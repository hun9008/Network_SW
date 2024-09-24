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
	
	// server address
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// peer address
	struct sockaddr_in peeraddr;
	socklen_t addrlen;
	char buf[BUFSIZE+1];
	int len;

	// communication loop
	while(1){
		printf("\n[Input Message] ");
		if(fgets(buf, BUFSIZE+1, stdin) == NULL)
			break;

		len = strlen(buf);
		if(buf[len-1] == '\n')
			buf[len-1] = '\0';
		if(strlen(buf) == 0)
			break;

		// send message
		retval = sendto(sock, buf, strlen(buf), 0, 
			(struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if(retval == -1){
			err_display("sendto()");
			continue;
		}
		printf("[UDP Client] Sent %d bytes.\n", retval);

		// receive message
		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0, 
			(struct sockaddr *)&peeraddr, &addrlen);
		if(retval == -1){
			err_display("recvfrom()");
			continue;
		}

		// verify sender's IP address
		if(memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))){
			printf("[Warning] Received message from unknown sender!\n");
			continue;
		}

		buf[retval] = '\0';
		printf("[UDP Client] Received %d bytes.\n", retval);
		printf("[Received Message] %s\n", buf);
	}

	// close socket
	close(sock);

	return 0;
}