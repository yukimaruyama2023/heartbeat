#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "monitor.h"

#define MEM_FILE "/dev/mem"
#define DEST_ADDR "192.168.23.99"
#define DEST_PORT 22224
#define RECV_ADDR "192.168.23.50"
#define RECV_PORT 22222
#define NCPUS 6
#define PERCPU_OFFSET 0x40000
#define PAGE_SIZE 0x1000
#define PAGE_MASK ~(PAGE_SIZE - 1)
#define READ_BUF_SIZE 3000

#define DEBUG(x) printf(#x ": %lx\n", x)

/* int fd_diskstat, fd_stat, fd_meminfo, fd_net; */
int fd_stat;

uint64_t statistics[NSTATS];
uint64_t addr[NSTATS][NCPUS];
void *mapped_addr[NSTATS][NCPUS];
uint64_t metric[NSTATS];
bool is_per_cpu[NSTATS] = {
    /* [DISK_WRITE_NSEC] = true, */
    [CPUTIME_USER] = true,
    [CPUTIME_NICE] = true,
    [CPUTIME_SYSTEM] = true,
    [CPUTIME_IDLE] = true,
    [CPUTIME_IOWAIT] = true,
    [CPUTIME_IRQ] = true,
    [CPUTIME_SOFTIRQ] = true,
    [CPUTIME_STEAL] = true,
    [CPUTIME_GUEST] = true,
    [CPUTIME_GUEST_NICE] = true,
    /* [NET_IP_IN_RECV] = true, */
    /* [NET_IP_IN_HDR_ERRORS] = true, */
    /* [NET_IP_ADDR_ERRORS] = true, */
};

void open_procfiles() {
    /* fd_diskstat = open("/proc/diskstats", O_RDONLY); */
    /* // fd_diskstat = open("dkstat.txt", O_RDONLY); */
    /* if (fd_diskstat < 0) { */
    /*     perror("open diskstat:"); */
    /*     exit(1); */
    /* } */
    fd_stat = open("/proc/stat", O_RDONLY);
    if (fd_stat < 0) {
        perror("open stat:");
        exit(1);
    }
    /* fd_meminfo = open("/proc/meminfo", O_RDONLY); */
    /* if (fd_meminfo < 0) { */
    /*     perror("open meminfo:"); */
    /*     exit(1); */
    /* } */
    /* fd_net = open("/proc/net/snmp", O_RDONLY); */
    /* if (fd_net < 0) { */
    /*     perror("open net/snmp:"); */
    /*     exit(1); */
    /* } */
}

/* void read_diskstat() { */
/*     lseek(fd_diskstat, 0, SEEK_SET); */
/*     char buf[READ_BUF_SIZE]; */
/*     read(fd_diskstat, buf, READ_BUF_SIZE); */
/*     int line = 1, pos = 0; */
/*     // seek to sda metrics line */
/*     while (line != 19) { */
/*         if (buf[pos++] == '\n') line++; */
/*         // printf("%d\n", line); */
/*     } */
/*     // seek to line 19:   8  0   sda */
/*     //                              ^ */
/*     while (buf[pos++] != 'a') */
/*         ; */
/*     // putchar(buf[pos]); */

/*     int word_count = 0; */
/*     // skip 8 - 1 words (= metrics) */
/*     while (word_count != 8) { */
/*         if (buf[pos++] == ' ') word_count++; */
/*     } */
/*     // read metric with atol */
/*     char *begin_write_nsec = &buf[pos]; */
/*     while (buf[++pos] != ' ') */
/*         ; */
/*     buf[pos] = '\0'; */
/*     metric[DISK_WRITE_NSEC] = atol(begin_write_nsec); */
/* } */

void read_stat() {
    lseek(fd_stat, 5, SEEK_SET);
    char buf[READ_BUF_SIZE];
    read(fd_stat, buf, READ_BUF_SIZE);
    int pos = 0;
    for (int i = CPUTIME_USER; i <= CPUTIME_GUEST_NICE; ++i) {
        char *begin_metric = &buf[pos];
        while (buf[++pos] != ' ' && buf[pos] != '\n')
            ;
        buf[pos++] = '\0';
        metric[i] = atol(begin_metric);
    }
}

/* void read_meminfo() { */
/*     lseek(fd_meminfo, 9, SEEK_SET); */
/*     char buf[READ_BUF_SIZE]; */
/*     read(fd_meminfo, buf, READ_BUF_SIZE); */
/*     int pos = 0; */
/*     // skip 1st line */
/*     while (buf[pos++] != '\n') */
/*         ; */
/*     // skip Memfree: */
/*     pos += 7; */
/*     while (buf[++pos] == ' ') */
/*         ; */
/*     char *met = &buf[pos]; */
/*     while (buf[++pos] != ' ') */
/*         ; */
/*     buf[pos] = '\0'; */
/*     metric[MEM_FREE_PAGE] = atol(met); */
/* } */

/* void read_net() { */
/*     lseek(fd_net, 232, SEEK_SET); */
/*     char buf[READ_BUF_SIZE]; */
/*     read(fd_net, buf, READ_BUF_SIZE); */
/*     int pos = 0; */
/*     for (int i = NET_IP_IN_RECV; i <= NET_IP_ADDR_ERRORS; ++i) { */
/*         char *begin_metric = &buf[pos]; */
/*         while (buf[++pos] != ' ' && buf[pos] != '\n') */
/*             ; */
/*         buf[pos++] = '\0'; */
/*         metric[i] = atol(begin_metric); */
/*     } */
/* } */

void read_metric() {
    /* read_diskstat(); */
    read_stat();
    /* read_meminfo(); */
    /* read_net(); */
}

void message_gen(void) {
    read_metric();
}

int main() {
    struct sockaddr_in send_addr, recv_addr;
    int send_sd, recv_sd;
    char msg[BUF_SIZE], buf[BUF_SIZE];

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    inet_aton(DEST_ADDR, &send_addr.sin_addr);
    send_addr.sin_port = htons(DEST_PORT);

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    inet_aton(RECV_ADDR, &recv_addr.sin_addr);
    recv_addr.sin_port = htons(RECV_PORT);

    if ((send_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket send");
        exit(1);
    }

    if ((recv_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket recv");
        exit(1);
    }

    if (bind(recv_sd, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (connect(send_sd, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    int val = 1;
    ioctl(recv_sd, FIONBIO, &val);
    open_procfiles();

    while (1) {
        message_gen();
        // wait for the request
        if (recv(recv_sd, buf, BUF_SIZE, 0) < 0) {
            if (errno == EAGAIN) continue;
            perror("recv");
            exit(1);
        }
        if (send(send_sd, metric, sizeof(uint64_t) * NSTATS, 0) < 0) {
            perror("send");
            exit(1);
        }
        puts("recv and send cycled");
    }
    close(recv_sd);
    close(send_sd);

    return 0;
}
