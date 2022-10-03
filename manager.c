#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 256
#define DEST_ADDR "192.168.23.124"
#define DEST_PORT 22222
#define RECV_ADDR "192.168.23.99"
#define RECV_PORT 22224

int main() {
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

    if (bind(recv_sd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (connect(send_sd, (struct sockaddr*)&send_addr, sizeof(send_addr)) < 0) {
        perror("connect");
        exit(1);
    }
    while (1) {
        sleep(1);
        if (send(send_sd, "Hello", 6, 0) < 0) {
            perror("send");
            exit(1);
        }

        if (recv(recv_sd, recv_buf, BUF_SIZE, 0) < 0) {
            perror("recv");
            exit(1);
        }
        printf("received: %s\n", recv_buf);
    }

    close(send_sd);
    close(recv_sd);

    return 0;
}
