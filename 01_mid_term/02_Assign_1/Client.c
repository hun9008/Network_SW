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

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
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
                (struct sockaddr *)&serveraddr, sizeof(serveraddr));

            // clear buffer
            memset(buf, 0, sizeof(buf));

            if (retval == -1) {
                err_display("sendto()");
                continue;
            }
            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (struct sockaddr *)&clientaddr, &addrlen);
            if (retval == -1) {
                err_display("recvfrom()");
                continue;
            }
            
            if (clientaddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr) {
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
                (struct sockaddr *)&serveraddr, sizeof(serveraddr));

            // clear buffer
            memset(buf, 0, sizeof(buf));
            memset(temp, 0, sizeof(temp));

            if (retval == -1) {
                err_display("sendto()");
                continue;
            }
            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (struct sockaddr *)&clientaddr, &addrlen);
            if (retval == -1) {
                err_display("recvfrom()");
                continue;
            }
            
            if (clientaddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr) {
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
                (struct sockaddr *)&serveraddr, sizeof(serveraddr));
            
            if (retval == -1) {
                err_display("sendto()");
                continue;
            }

            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (struct sockaddr *)&clientaddr, &addrlen);
            
            if (retval == -1) {
                err_display("recvfrom()");
                continue;
            }

            if (clientaddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr) {
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
                (struct sockaddr *)&serveraddr, sizeof(serveraddr));
            
            if (retval == -1) {
                err_display("sendto()");
                continue;
            }

            printf("[UDP Client] Sent %d bytes.\n", retval);

            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, 
                (struct sockaddr *)&clientaddr, &addrlen);
            
            if (retval == -1) {
                err_display("recvfrom()");
                continue;
            }

            if (clientaddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr) {
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

    close(sock);

    return 0;
}