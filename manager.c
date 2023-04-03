#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 256
#define DEST_ADDR "192.168.23.99"
#define DEST_PORT 22222
#define RECV_ADDR "192.168.23.99"
#define RECV_PORT 22224

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
    struct timespec send_time, recv_time, interval = {.tv_sec = 0, .tv_nsec = 5000000};
    FILE *fp = fopen(argv[1], "w");
    uint64_t val;
    for (int i = 0; i < 10000; ++i) {
        nanosleep(&interval, NULL);
        timespec_get(&send_time, TIME_UTC);
        if (send(send_sd, "Hello", 6, 0) < 0) {
            perror("send");
            exit(1);
        }

        if (recv(recv_sd, &val, BUF_SIZE, 0) < 0) {
            perror("recv");
            exit(1);
        }
        timespec_get(&recv_time, TIME_UTC);
        printf("%ld.%ld,%09ld,%lu\n", send_time.tv_sec, send_time.tv_nsec, (recv_time.tv_sec - send_time.tv_sec) * 1000000000 + recv_time.tv_nsec - send_time.tv_nsec, val);
        fprintf(fp, "%ld.%09ld,%ld,%lu\n", send_time.tv_sec, send_time.tv_nsec, (recv_time.tv_sec - send_time.tv_sec) * 1000000 + recv_time.tv_nsec - send_time.tv_nsec, val);
    }

    close(send_sd);
    close(recv_sd);
    fclose(fp);

    return 0;
}
