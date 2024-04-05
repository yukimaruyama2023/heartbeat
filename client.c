#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "monitor.h"

#define BUF_SIZE 256
#define MAXLINE 1024
/* #define DEST_ADDR "192.168.99.37" */
#define DEST_ADDR "192.168.99.37"
#define DEST_PORT 22222
#define RECV_ADDR "192.168.99.38"
#define RECV_PORT 22222
#define INTERVAL 0.01 // modified. The unit is second

uint64_t metric[NSTATS];
uint64_t old_metric[NSTATS];

uint64_t metric_diff(enum stat_id id) {
    return metric[id] - old_metric[id];
}

int main(int argc, char **argv) {
    struct sockaddr_in send_addr, recv_addr;
    int send_sd, recv_sd;
    char recv_buf[BUF_SIZE];

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

    long init_data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    if (send(send_sd, init_data, sizeof(init_data), 0) < 0) {
        perror("send");    
        exit(1);
    }
    puts("Hello message sent");

    int n;
    long metrics[10];
    if (( n = recv(recv_sd, metrics, sizeof(metrics), 0) < 0)) {
        perror("recv");
        exit(1);
    }
    puts("Recieved from server");
    for (int i = 0; i < 10; i++) {
	    printf("metrics[%d] : %ld\n", i, metrics[i]);
    }

    close(send_sd);
    close(recv_sd);

    return 0;
}
