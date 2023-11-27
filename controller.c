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
#define DEST_ADDR "192.168.23.210"
#define DEST_PORT 22224
#define RECV_ADDR "192.168.23.99"
#define RECV_PORT 22222
#define NCPUS 6
#define PERCPU_OFFSET 0x40000
#define PAGE_SIZE 0x1000
#define PAGE_MASK ~(PAGE_SIZE - 1)

#define DEBUG(x) printf(#x ": %lx\n", x)

uint64_t statistics[NSTATS];
uint64_t addr[NSTATS][NCPUS];
void *mapped_addr[NSTATS][NCPUS];
uint64_t metric[NSTATS];
bool is_per_cpu[NSTATS] = {
    [DISK_WRITE_NSEC] = true,
    [CPUTIME_USER] = true,
    [CPUTIME_NICE] = true,
    [CPUTIME_SYSTEM] = true,
    [CPUTIME_SOFTIRQ] = true,
    [CPUTIME_IRQ] = true,
    [CPUTIME_IDLE] = true,
    [CPUTIME_IOWAIT] = true,
    [CPUTIME_STEAL] = true,
    [CPUTIME_GUEST] = true,
    [CPUTIME_GUEST_NICE] = true,
    [NET_IP_IN_RECV] = true,
    [NET_IP_IN_HDR_ERRORS] = true,
    [NET_IP_ADDR_ERRORS] = true,
};

void read_metric(enum stat_id id) {
    if (!is_per_cpu[id]) {
        metric[id] = *(uint64_t *)mapped_addr[id][0];
        return;
    }

    metric[id] = 0;
    for (int i = 0; i < NCPUS; ++i) {
        metric[id] += *(uint64_t *)mapped_addr[id][i];
    }
}

void message_gen(void) {
    for (int i = 0; i < NSTATS; ++i) {
        read_metric(i);
        // printf("%ld\n", metric[i]);
    }
    // printf("\n");
}

void metric_mmap(int mem_fd, enum stat_id id) {
    for (int i = 0; i < (is_per_cpu[id] ? NCPUS : 1); ++i) {
        mapped_addr[id][i] = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_PRIVATE, mem_fd, addr[id][i] & PAGE_MASK);
        if (mapped_addr[id][i] == (void *)-1) {
            perror("mmap");
            exit(1);
        }
        mapped_addr[id][i] += addr[id][i] & ~PAGE_MASK;
    }
}

void metric_unmap(enum stat_id id) {
    for (int i = 0; i < (is_per_cpu[id] ? NCPUS : 1); ++i) {
        munmap((uint64_t)mapped_addr[id][i] & PAGE_MASK, PAGE_SIZE);
    }
}

void read_addr_from_file(char *filename, enum stat_id first_id, enum stat_id last_id) {
    FILE *fp = fopen(filename, "r");
    for (int id = first_id; id <= last_id; ++id) {
        if (!is_per_cpu[id]) {
            fscanf(fp, "%lx", &addr[id][0]);
        } else {
            for (int cpu = 0; cpu < NCPUS; ++cpu) {
                fscanf(fp, "%lx", &addr[id][cpu]);
            }
        }
    }
    fclose(fp);
}

void init_metrics(int mem_fd) {
    read_addr_from_file("addr_diskstat.txt", DISK_WRITE_NSEC, DISK_WRITE_NSEC);
    read_addr_from_file("addr_cpustat.txt", CPUTIME_USER, CPUTIME_GUEST_NICE);
    read_addr_from_file("addr_memstat.txt", MEM_FREE_PAGE, MEM_FREE_PAGE);
    read_addr_from_file("addr_netstat.txt", NET_IP_IN_RECV, NET_IP_ADDR_ERRORS);
    for (int i = 0; i < NSTATS; ++i) {
        metric_mmap(mem_fd, i);
    }
}

void unmap_metrics() {
    for (int i = 0; i < NSTATS; ++i) {
        metric_unmap(i);
    }
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

    int mem_fd = open(MEM_FILE, O_RDONLY);
    if (mem_fd < 0) {
        perror("open" MEM_FILE);
        exit(1);
    }

    int val = 1;
    ioctl(recv_sd, FIONBIO, &val);

    init_metrics(mem_fd);

    while (1) {
        message_gen();
        // wait for the request
        if (recv(recv_sd, buf, BUF_SIZE, 0) < 0) {
            if (errno == EAGAIN) continue;
            perror("recv");
            exit(1);
        }
        // double val;
        // memcpy(&val, msg, 8);
        //  printf("%lf\n", val);
        //  printf("received: %s\n", msg);
        if (send(send_sd, metric, sizeof(uint64_t) * NSTATS, 0) < 0) {
            perror("send");
            exit(1);
        }
    }
    unmap_metrics();
    close(recv_sd);
    close(send_sd);
    close(mem_fd);

    return 0;
}
