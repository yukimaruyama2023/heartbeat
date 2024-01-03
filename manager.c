#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "monitor.h"

#define BUF_SIZE 256
#define DEST_ADDR "192.168.99.37"
#define DEST_PORT 22222
#define RECV_ADDR "192.168.99.38"
#define RECV_PORT 22224
#define INTERVAL 0.01 // modified. The unit is second

uint64_t metric[NSTATS];
uint64_t old_metric[NSTATS];

uint64_t metric_diff(enum stat_id id) {
    return metric[id] - old_metric[id];
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("usage: ./manager result_filename");
        return 0;
    }
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
    struct timespec send_time, recv_time, interval = {.tv_sec = 0, .tv_nsec = INTERVAL * 1000000000}; // modified
    struct tm *time;
    FILE *fp = fopen(argv[1], "w");
    if (send(send_sd, "Hello", 6, 0) < 0) {
        perror("send");
        exit(1);
    }

    if (recv(recv_sd, old_metric, NSTATS * sizeof(uint64_t), 0) < 0) {
        perror("recv");
        exit(1);
    }
    for (int i = 0; i < 12; ++i) {
        nanosleep(&interval, NULL);
        timespec_get(&send_time, TIME_UTC);
        if (send(send_sd, "Hello", 6, 0) < 0) {
            perror("send");
            exit(1);
        }

        if (recv(recv_sd, metric, NSTATS * sizeof(uint64_t), 0) < 0) {
            perror("recv");
            exit(1);
        }
        timespec_get(&recv_time, TIME_UTC);
        time = localtime(&recv_time.tv_sec);
        // printf("%ld.%ld,%09ld: ", send_time.tv_sec, send_time.tv_nsec, (recv_time.tv_sec - send_time.tv_sec) * 1000000000 + recv_time.tv_nsec - send_time.tv_nsec);
        // for (int i = 0; i < NSTATS; ++i) {
        //     printf("%ld ", metric[i]);
        // }
        // printf("\n");
        fprintf(fp, "%d/%02d/%02d-%02d:%02d:%02d.%ld,", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, recv_time.tv_nsec / 1000);
        fprintf(fp, "%ld.%09ld,%ld,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n", send_time.tv_sec, send_time.tv_nsec,
                (recv_time.tv_sec - send_time.tv_sec) * 1000000L + (recv_time.tv_nsec - send_time.tv_nsec) / 1000L,
                /* metric_diff(DISK_WRITE_NSEC), */
                metric_diff(CPUTIME_USER),
                metric_diff(CPUTIME_NICE),
                metric_diff(CPUTIME_SYSTEM),
                metric_diff(CPUTIME_IDLE),
                metric_diff(CPUTIME_IOWAIT),
                metric_diff(CPUTIME_IRQ),
                metric_diff(CPUTIME_SOFTIRQ),
                metric_diff(CPUTIME_STEAL),
                metric_diff(CPUTIME_GUEST),
                metric_diff(CPUTIME_GUEST_NICE));
                /* metric[MEM_FREE_PAGE]); */
        memcpy(old_metric, metric, sizeof(uint64_t) * NSTATS);
    }

    close(send_sd);
    close(recv_sd);
    fclose(fp);

    return 0;
}
