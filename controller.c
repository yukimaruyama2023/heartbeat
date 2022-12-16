#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 256
#define MSG_LEN 8
#define DEST_ADDR "10.0.1.3"
#define DEST_PORT 22223
#define RECV_ADDR "10.0.1.4"
#define RECV_PORT 22225
#define NCPUS 12

void read_addr(const char *filename, off_t *addr) {
    FILE *fp = fopen(filename, "r");
    for (int i = 0; i < NCPUS; ++i) {
        fscanf(fp, "%lx", &addr[i]);
    }
    fclose(fp);
}

uint64_t read_cpuinfo(int kmem_fd, off_t *addr) {
    uint64_t write_nsec = 0;
    for (int i = 0; i < NCPUS; ++i) {
        if (lseek(kmem_fd, addr[i], SEEK_SET) == (off_t)-1) {
            perror("lseek");
            exit(1);
        }
        uint64_t val;
        ssize_t n;
        n = read(kmem_fd, &val, 8);
        if (n == (ssize_t)-1) {
            perror("read");
            exit(1);
        }
        if (n != 8) {
            fprintf(stderr, "n is %ld, not 8 (i = %d)\n", n, i);
            exit(1);
        }
        printf("val[%d] = %lu\n", i, val);
        write_nsec += val;
    }
    return write_nsec;
}

void message_gen(char *msg, int kmem_fd, off_t *addr) {
    static uint64_t old_write_nsec = 0;
    static struct timespec old_time;
    struct timespec now_time;

    if (old_write_nsec == 0) {
        timespec_get(&old_time, TIME_UTC);
        old_write_nsec = read_cpuinfo(kmem_fd, addr);
    }

    timespec_get(&now_time, TIME_UTC);
    uint64_t write_nsec = read_cpuinfo(kmem_fd, addr);
    long diff_time = (now_time.tv_sec - old_time.tv_nsec) * 1000000 + now_time.tv_nsec - old_time.tv_nsec;
    long diff_write_nsec = write_nsec - old_write_nsec;

    // double ret = diff_write_nsec / diff_time

    old_time = now_time;
    old_write_nsec = write_nsec;

    memcpy(msg, &write_nsec, MSG_LEN);
}

int main() {
    struct sockaddr_in send_addr, recv_addr;
    int send_sd, recv_sd, kmem_fd;
    char msg[BUF_SIZE];
    off_t cpuinfo_addr[NCPUS];

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

    kmem_fd = open("/dev/kmem", O_RDONLY);
    if (kmem_fd < 0) {
        perror("open /dev/kmem");
        exit(1);
    }
    read_addr("address.txt", cpuinfo_addr);

    int val = 1;
    ioctl(recv_sd, FIONBIO, &val);

    while (1) {
        message_gen(msg, kmem_fd, cpuinfo_addr);
        // wait for the request
        if (recv(recv_sd, msg, BUF_SIZE, 0) < 0) {
            if (errno == EAGAIN) continue;
            perror("recv");
            exit(1);
        }
        printf("received: %s\n", msg);
        if (send(send_sd, msg, MSG_LEN, 0) < 0) {
            perror("send");
            exit(1);
        }
    }

    close(recv_sd);
    close(send_sd);
    close(kmem_fd);

    return 0;
}
