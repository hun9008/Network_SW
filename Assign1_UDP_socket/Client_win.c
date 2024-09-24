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
    int syntax;
    char ip[20] = "";
    int port;

    // fromIP:fromPort toIP:toPort byteNum:message
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

    int retval;

    WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];
    int len;

    while(1) {

        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;
        
        if (strcmp(buf, "echo\n") == 0) {
            syntax = 0x01;
        }
        else if (strcmp(buf, "chat\n") == 0) {
            syntax = 0x02;
        }
        else if (strcmp(buf, "stat\n") == 0) {
            syntax = 0x03;
        }
        else if (strcmp(buf, "quit\n") == 0) {
            syntax = 0x04;
        }
        else {
            printf("Syntax Error\n");
            break;
        }
        memset(buf, 0, sizeof(buf));
        
        if(syntax == 0x01) {
            printf("echo client\n");

            printf("\n[Input Message] ");
        
            // scanf("%s", buf);
            if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                break;
            
            len = strlen(buf);
            if (buf[len - 1] == '\n')
                buf[len - 1] = '\0';

            if (strlen(buf) == 0)
            {
                printf("strlen error\n");
                break;
            }

            // buf앞에 0x01을 붙여서 보내기
            char temp[BUFSIZE + 1];
            sprintf(temp, "%02x%s", syntax, buf);
            strcpy(buf, temp);


            retval = sendto(sock, buf, strlen(buf), 0, 
                (SOCKADDR *)&serveraddr, sizeof(serveraddr));
            if(retval == SOCKET_ERROR){
                err_display("sendto()");
                continue;
            }

            // clear buffer
            memset(buf, 0, sizeof(buf));

            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (SOCKADDR *)&clientaddr, &addrlen);
            if (retval == SOCKET_ERROR) {
                err_display("recvfrom()");
                continue;
            }
            
            if (memcmp(&clientaddr, &serveraddr, sizeof(clientaddr))) {
                printf("[Warning] Received message from unknown sender!\n");
                printf("clientaddr : %s\n", inet_ntoa(clientaddr.sin_addr));
                printf("serveraddr : %s\n", inet_ntoa(serveraddr.sin_addr));
                continue;
            }

            buf[retval] = '\0';
            printf("[UDP Client] Received %d bytes.\n", retval);

            char *recv_info, *recv_msg;

            recv_info = strtok(buf, "|");
            recv_msg = strtok(NULL, "|");

            printf("[Received Message] %s\t%s\n", recv_info, recv_msg);

        }
        else if(syntax == 0x02) {
            printf("chat client\n");


            printf("\n[Input Message] ");
        
            if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                break;
            len = strlen(buf);
            if (buf[len - 1] == '\n')
                buf[len - 1] = '\0';

            if (strlen(buf) == 0)
            {
                printf("strlen error\n");
                break;
            }

            // buf앞에 0x01을 붙여서 보내기
            char temp[BUFSIZE + 1];
            sprintf(temp, "%02x%s", syntax, buf);
            strcpy(buf, temp);


            retval = sendto(sock, buf, strlen(buf), 0, 
                (SOCKADDR *)&serveraddr, sizeof(serveraddr));

            // clear buffer
            memset(buf, 0, sizeof(buf));
            memset(temp, 0, sizeof(temp));

            if (retval == SOCKET_ERROR) {
                err_display("sendto()");
                continue;
            }
            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (SOCKADDR *)&clientaddr, &addrlen);
            if (retval == -1) {
                err_display("recvfrom()");
                continue;
            }
            
            if (memcmp(&clientaddr, &serveraddr, sizeof(clientaddr))) {
                printf("[Warning] Received message from unknown sender!\n");
                printf("clientaddr : %s\n", inet_ntoa(clientaddr.sin_addr));
                printf("serveraddr : %s\n", inet_ntoa(serveraddr.sin_addr));
                continue;
            }

            buf[retval] = '\0';
            printf("[UDP Client] Received %d bytes.\n", retval);

            char *recv_info, *recv_msg;

            recv_info = strtok(buf, "|");
            recv_msg = strtok(NULL, "|");

            printf("[Received Message] %s\t%s\n", recv_info, recv_msg);
        }
        else if(syntax == 0x03) {
            printf("stat client\n");

            printf("\n[Input Message] ");

            if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
                break;
            len = strlen(buf);
            if (buf[len - 1] == '\n')
                buf[len - 1] = '\0';
            
            if (strlen(buf) == 0)
            {
                printf("strlen error\n");
                break;
            }

            char temp[BUFSIZE + 1];
            sprintf(temp, "%02x%s", syntax, buf);
            strcpy(buf, temp);

            retval = sendto(sock, buf, strlen(buf), 0, 
                (SOCKADDR *)&serveraddr, sizeof(serveraddr));
            
            if (retval == -1) {
                err_display("sendto()");
                continue;
            }

            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (SOCKADDR *)&clientaddr, &addrlen);
            
            if (retval == SOCKET_ERROR) {
                err_display("recvfrom()");
                continue;
            }

            if (memcmp(&clientaddr, &serveraddr, sizeof(clientaddr))) {
                printf("[Warning] Received message from unknown sender!\n");
                printf("clientaddr : %s\n", inet_ntoa(clientaddr.sin_addr));
                printf("serveraddr : %s\n", inet_ntoa(serveraddr.sin_addr));
                continue;
            }

            buf[retval] = '\0';
            printf("[UDP Client] Received %d bytes.\n", retval);

            char *recv_info, *recv_msg;

            recv_info = strtok(buf, "|");
            recv_msg = strtok(NULL, "|");

            printf("[Received Message] %s\t%s\n", recv_info, recv_msg);
            
        }
        else if(syntax == 0x04) {
            printf("quit client\n");
            
            char temp[BUFSIZE + 1];
            sprintf(temp, "%02x", syntax);
            strcpy(buf, temp);

            retval = sendto(sock, buf, strlen(buf), 0, 
                (SOCKADDR *)&serveraddr, sizeof(serveraddr));
            
            if (retval == SOCKET_ERROR) {
                err_display("sendto()");
                continue;
            }

            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (SOCKADDR *)&clientaddr, &addrlen);
            
            if (retval == -1) {
                err_display("recvfrom()");
                continue;
            }

            if (memcmp(&clientaddr, &serveraddr, sizeof(clientaddr))) {
                printf("[Warning] Received message from unknown sender!\n");
                printf("clientaddr : %s\n", inet_ntoa(clientaddr.sin_addr));
                printf("serveraddr : %s\n", inet_ntoa(serveraddr.sin_addr));
                continue;
            }

            buf[retval] = '\0';
            printf("[UDP Client] Received %d bytes.\n", retval);
            
            char *recv_info, *recv_msg;

            recv_info = strtok(buf, "|");
            recv_msg = strtok(NULL, "|");

            printf("[Received Message] %s\t%s\n", recv_info, recv_msg);

            break;
        }
        else {
            printf("Syntax Error\n");
        }
    }

    closesocket(sock);

    WSACleanup();

    return 0;
}