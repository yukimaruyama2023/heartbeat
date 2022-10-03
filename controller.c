#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 256
#define DEST_ADDR "192.168.100.2"
#define DEST_PORT 22223
#define RECV_ADDR "192.168.100.1"
#define RECV_PORT 22225

void message_gen(char *msg) {
    strcpy(msg, "Hello");
}

int main() {
    struct sockaddr_in send_addr, recv_addr;
    int send_sd, recv_sd;
    char msg[BUF_SIZE];

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    inet_aton(DEST_ADDR, &send_addr.sin_addr);
    send_addr.sin_port = htons(DEST_PORT);

    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    inet_aton(RECV_ADDR, &recv_addr.sin_addr);
    recv_addr.sin_port = htons(RECV_PORT);

    if ((send_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
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

    while (1) {
        // wait for the request
        if (recv(recv_sd, msg, BUF_SIZE, 0) < 0) {
            perror("recv");
            exit(1);
        }
        printf("received: %s\n", msg);
        // send information
        if (send(send_sd, msg, strlen(msg), 0) < 0) {
            perror("send");
            exit(1);
        }
    }

    close(recv_sd);
    close(send_sd);

    return 0;
}
