#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 256
#define MSG_LEN 8
#define MANAGER_ADDR "10.0.1.1"
#define MANAGER_PORT 22224
#define MANAGER_RECV_ADDR "10.0.1.2"
#define MANAGER_RECV_PORT 22222
#define CONTROLLER_RECV_ADDR "10.0.1.3"
#define CONTROLLER_RECV_PORT 22223
#define CONTROLLER_ADDR "10.0.1.4"
#define CONTROLLER_PORT 22225

// controllerからの情報を基に、managerへのメッセージを生成する
void generate_msg(const char *controller_msg, char *manager_msg) {
    memcpy(manager_msg, controller_msg, MSG_LEN);
}

int main() {
    struct sockaddr_in manager_send_addr, manager_recv_addr, controller_send_addr, controller_recv_addr;
    int manager_send_sd, manager_recv_sd, controller_send_sd, controller_recv_sd;

    memset(&manager_send_addr, 0, sizeof(manager_send_addr));
    manager_send_addr.sin_family = AF_INET;
    inet_aton(MANAGER_ADDR, &manager_send_addr.sin_addr);
    manager_send_addr.sin_port = htons(MANAGER_PORT);

    memset(&manager_recv_addr, 0, sizeof(manager_recv_addr));
    manager_recv_addr.sin_family = AF_INET;
    inet_aton(MANAGER_RECV_ADDR, &manager_recv_addr.sin_addr);
    manager_recv_addr.sin_port = htons(MANAGER_RECV_PORT);

    memset(&controller_send_addr, 0, sizeof(controller_send_addr));
    controller_send_addr.sin_family = AF_INET;
    inet_aton(CONTROLLER_ADDR, &controller_send_addr.sin_addr);
    controller_send_addr.sin_port = htons(CONTROLLER_PORT);

    memset(&controller_recv_addr, 0, sizeof(controller_recv_addr));
    controller_recv_addr.sin_family = AF_INET;
    inet_aton(CONTROLLER_RECV_ADDR, &controller_recv_addr.sin_addr);
    controller_recv_addr.sin_port = htons(CONTROLLER_RECV_PORT);

    if ((manager_send_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket manager send");
        exit(1);
    }

    if ((manager_recv_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket manager recv");
        exit(1);
    }

    if ((controller_send_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket controller send");
        exit(1);
    }

    if ((controller_recv_sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket controller recv");
        exit(1);
    }

    if (bind(manager_recv_sd, (struct sockaddr *)&manager_recv_addr, sizeof(manager_recv_addr)) < 0) {
        perror("bind manager sd");
        exit(1);
    }

    if (bind(controller_recv_sd, (struct sockaddr *)&controller_recv_addr, sizeof(controller_recv_addr)) < 0) {
        perror("bind controller sd");
        exit(1);
    }

    if (connect(manager_send_sd, (struct sockaddr *)&manager_send_addr, sizeof(manager_send_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    if (connect(controller_send_sd, (struct sockaddr *)&controller_send_addr, sizeof(controller_send_addr)) < 0) {
        perror("connect controller");
        exit(1);
    }

    char controller_buf[BUF_SIZE], manager_buf[BUF_SIZE];
    int controller_msg_len, manager_msg_len;
    while (1) {
        // receive request from manager
        if (recv(manager_recv_sd, manager_buf, BUF_SIZE, 0) < 0) {
            perror("recv manager message");
            exit(1);
        }
        printf("manager: %s\n", manager_buf);
        // when received
        // send request to the controller
        if (send(controller_send_sd, "Hello", 6, 0) < 0) {
            perror("send controller");
            exit(1);
        }
        // receive infomation from controller
        if (recv(controller_recv_sd, controller_buf, BUF_SIZE, 0) < 0) {
            perror("recv controller message");
            exit(1);
        }
        printf("controller: %s\n", controller_buf);
        generate_msg(controller_buf, manager_buf);
        // send information to the manager
        if (send(manager_send_sd, manager_buf, MSG_LEN, 0) < 0) {
            perror("send manager message");
            exit(1);
        }
    }

    close(manager_send_sd);
    close(manager_recv_sd);
    close(controller_send_sd);
    close(controller_recv_sd);

    return 0;
}
