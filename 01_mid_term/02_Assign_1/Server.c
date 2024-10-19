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
    int syntax;
    char ip[20] = "";
    int port;
    char recvformat[BUFSIZE + 1];
    int sum_recvByte = 0;
    int sum_recvMsgNum = 0;

    if(argc != 3) {
        printf("Parameter Error\n");
        return -1;
    }
    else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

	// socket()
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) err_quit("socket()");
	
	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if(retval == -1) err_quit("bind()");
	
	// client address
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char buf[BUFSIZE + 1];
    char temp[BUFSIZE + 1];

	// communication loop
	while(1){
		// receive message
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, temp, BUFSIZE, 0, 
			(struct sockaddr *)&clientaddr, &addrlen);
		if(retval == -1){
			err_display("recvfrom()");
			continue;
		} else {
            sum_recvByte += retval;
            sum_recvMsgNum++;
        }

		// print received message
		temp[retval] = '\0';
        char hex[3];
        strncpy(hex, temp, 2);
        hex[2] = '\0';
        syntax = strtol(hex, NULL, 16);
        strncpy(buf, temp + 2, retval - 2);

        printf("syntax : %x\n", syntax);
        printf("buf : %s\n", buf);

        if (syntax == 0x01) {

            printf("[UDP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), 
                ntohs(clientaddr.sin_port), buf);

            // recvformat은 [ip:port]<-[ip:port]:[n:recvmsg]::[sendmsg]
            snprintf(recvformat, BUFSIZE, "[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, buf);

            // send message back
            retval = sendto(sock, recvformat, strlen(recvformat), 0, 
                (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            if (retval == -1) {
                err_display("sendto()");
                continue;
            } 

        } else if (syntax == 0x02) {

            printf("[UDP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), 
                ntohs(clientaddr.sin_port), buf);

            char chat[BUFSIZE + 1];
            printf("[Input Message] ");
            
            if (fgets(chat, BUFSIZE + 1, stdin) == NULL)
                break;
            if (chat[strlen(chat) - 1] == '\n')
                chat[strlen(chat) - 1] = '\0';    
        
            if (strlen(chat) == 0)
            {
                printf("strlen error\n");
                break;
            }

            snprintf(recvformat, BUFSIZE, "[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, chat);

            retval = sendto(sock, recvformat, strlen(recvformat), 0, 
                (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            if (retval == -1) {
                err_display("sendto()");
                continue;
            } 

        } else if (syntax == 0x03) {
            
            printf("[UDP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), 
                ntohs(clientaddr.sin_port), buf);

            char info[BUFSIZE + 1];

            if (strcmp(buf, "bytes") == 0) {
                sprintf(info, "%d bytes", sum_recvByte);
            } else if (strcmp(buf, "number") == 0) {
                sprintf(info, "%d messages", sum_recvMsgNum);
            } else {
                sprintf(info, "%d bytes : %d messages", sum_recvByte, sum_recvMsgNum);
            }

            snprintf(recvformat, BUFSIZE, "[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, info);
            
            retval = sendto(sock, recvformat, strlen(recvformat), 0,
                (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            if (retval == -1) {
                err_display("sendto()");
                continue;
            }

        } else if (syntax == 0x04) {

            snprintf(recvformat, BUFSIZE,"[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, "server soon close");

            retval = sendto(sock, recvformat, strlen(recvformat), 0,
                (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            if (retval == -1) {
                err_display("sendto()");
                continue;
            }

            break;

        } else {
            printf("syntax error\n");
            break;
        }

        memset(buf, 0, sizeof(buf));
        memset(temp, 0, sizeof(temp));
        memset(recvformat, 0, sizeof(recvformat));
	}

	// close socket
	close(sock);

	return 0;
}