#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 512

void err_quit(char *msg)
{
	perror(msg);
	exit(-1);
}

void err_display(char *msg)
{
	perror(msg);
}

int main(int argc, char* argv[])
{
	int retval;

	// socket()
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) err_quit("socket()");
	
	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if(retval == -1) err_quit("bind()");
	
	// client address
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char buf[BUFSIZE+1];

	// communication loop
	while(1){
		// receive message
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0, 
			(struct sockaddr *)&clientaddr, &addrlen);
		if(retval == -1){
			err_display("recvfrom()");
			continue;
		}

		// print received message
		buf[retval] = '\0';
		printf("[UDP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), 
			ntohs(clientaddr.sin_port), buf);

		// send message back
		retval = sendto(sock, buf, retval, 0, 
			(struct sockaddr *)&clientaddr, sizeof(clientaddr));
		if(retval == -1){
			err_display("sendto()");
			continue;
		}
	}

	// close socket
	close(sock);

	return 0;
}