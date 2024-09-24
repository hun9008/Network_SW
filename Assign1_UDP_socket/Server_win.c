#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 512

void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
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

    WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;
	
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");
	
	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	retval = bind(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");
	
	// client address
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];
    char temp[BUFSIZE + 1];

	// communication loop
	while(1){
		// receive message
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, temp, BUFSIZE, 0, 
			(SOCKADDR *)&clientaddr, &addrlen);
		if(retval == SOCKET_ERROR){
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
            sprintf(recvformat, "[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, buf);

            // send message back
            retval = sendto(sock, recvformat, strlen(recvformat), 0, 
                (SOCKADDR *)&clientaddr, sizeof(clientaddr));
            if (retval == SOCKET_ERROR) {
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

            sprintf(recvformat, "[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, chat);

            retval = sendto(sock, recvformat, strlen(recvformat), 0, 
                (SOCKADDR *)&clientaddr, sizeof(clientaddr));
            if (retval == SOCKET_ERROR) {
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

            sprintf(recvformat, "[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, info);
            
            retval = sendto(sock, recvformat, strlen(recvformat), 0,
                (SOCKADDR *)&clientaddr, sizeof(clientaddr));
            if (retval == SOCKET_ERROR) {
                err_display("sendto()");
                continue;
            }

        } else if (syntax == 0x04) {

            sprintf(recvformat, "[%s:%d]<-[%s:%d]:[%lu:%s]|%s", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), 
                inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), 
                strlen(buf), buf, "server soon close");

            retval = sendto(sock, recvformat, strlen(recvformat), 0,
                (SOCKADDR *)&clientaddr, sizeof(clientaddr));
            if (retval == SOCKET_ERROR) {
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
	closesocket(sock);
    WSACleanup();

	return 0;
}