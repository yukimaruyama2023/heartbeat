#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 256
#define MANAGER_ADDR "192.168.23.99"
#define MANAGER_PORT 22224
#define MANAGER_RECV_ADDR "192.168.23.124"
#define MANAGER_RECV_PORT 22222
#define CONTROLLER_RECV_ADDR "192.168.100.2"
#define CONTROLLER_RECV_PORT 22223
#define CONTROLLER_ADDR "192.168.100.1"
#define CONTROLLER_PORT 22225

// controllerからの情報を基に、managerへのメッセージを生成する
void generate_msg(const char *controller_msg, char *manager_msg) {
    strcpy(manager_msg, controller_msg);
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
    // non-blocking connection
    // do not wait for receive heartbeat requests
    int val = 1;
    ioctl(manager_recv_sd, FIONBIO, &val);

    if (bind(controller_recv_sd, (struct sockaddr *)&controller_recv_addr, sizeof(controller_recv_addr)) < 0) {
        perror("bind controller sd");
        exit(1);
    }

    if (connect(manager_send_sd, (struct sockaddr *)&manager_recv_addr, sizeof(manager_recv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    char controller_buf[BUF_SIZE], manager_buf[BUF_SIZE];
    int controller_msg_len, manager_msg_len;
    while (1) {
        // receive request from manager
        if ((manager_msg_len = recv(manager_recv_sd, manager_buf, BUF_SIZE, 0)) < 0 && errno != EAGAIN) {
            perror("recv manager message");
            exit(1);
        }
        // request not coming
        else if (manager_msg_len < 0) {
            continue;
        }
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
        generate_msg(controller_buf, manager_buf);
        // send information to the manager
        if (send(manager_send_sd, manager_buf, strlen(manager_buf), 0) < 0) {
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
