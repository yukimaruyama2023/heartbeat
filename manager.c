#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 256
#define DEST_ADDR "192.168.23.124"
#define DEST_PORT 22223
#define RECV_PORT 22224

int main(){
	struct sockaddr_in send_addr, recv_addr;
	int send_sd, recv_sd;
	char recv_buf[BUF_SIZE];

	memset(&send_addr, 0, sizeof(send_addr));
	send_addr.sin_family = AF_INET;
	inet_aton(DEST_ADDR, &send_addr.sin_addr);
	send_addr.sin_port = htons(DEST_PORT);
	
	memset(&recv_addr, 0, sizeof(recv_addr));
	recv_addr.sin_family = AF_INET;
	inet_aton(INADDR_ANY, &recv_addr.sin_addr);
	recv_addr.sin_port = htons(RECV_PORT);

	if((send_sd = socket(AF_INET, SOCK_DGRAM,0)) < 0){
		perror("socket send");
		exit(1);
	}

	if((recv_sd = socket(AF_INET, SOCK_DGRAM,0)) < 0){
		perror("socket recv");
		exit(1);
	}

	if(bind(recv_sd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0){
		perror("bind");
		exit(1);
	}

	if(connect(send_sd, (struct sockaddr*) &recv_addr, sizeof(recv_addr)) < 0){
		perror("connect");
		exit(1);
	}
	while(1){
		if(send(send_sd, "Hello", 6, 0) < 0){
			perror("send");
			exit(1);
		}

		if(recv(send_sd, recv_buf, BUF_SIZE, 0) < 0){
			perror("recv");
			exit(1);
		}
		printf("received: %s\n", recv_buf);
		if(recv_buf[0] == 'x') break;
	}

	close(send_sd);
	close(recv_sd);
	
	return 0;
}

