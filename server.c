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
#define DEST_PORT 22222
#define RECV_ADDR "192.168.23.50"
#define RECV_PORT 22222
#define NCPUS 6
#define PERCPU_OFFSET 0x40000
#define PAGE_SIZE 0x1000
#define PAGE_MASK ~(PAGE_SIZE - 1)
#define READ_BUF_SIZE 3000
#define MAXLINE 1024

#define DEBUG(x) printf(#x ": %lx\n", x)

int fd_stat;

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

    char *hello = "Hello from server";
    long buffer[10];
    int n;
    if (n = recv(recv_sd, buffer, sizeof(buffer), 0) < 0) {
        perror("recv");
        exit(1);
    }
    puts("Message recieved");
    send(send_sd, buffer, sizeof(buffer), 0);
    puts("Hello message sent");
    close(recv_sd);
    close(send_sd);

    return 0;
}
