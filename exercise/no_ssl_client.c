#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>     // for close()
#include <stdlib.h>
#include <stdio.h>
#include <string.h>     // for memset()

#define BUFSIZE 1500

void err_quit(const char *msg)
{
    perror(msg);
    exit(-1);
}

void err_display(const char *msg)
{
    perror(msg);
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    int retval;
    int sock;                       
    struct sockaddr_in serveraddr;   
    char buf[BUFSIZE + 1];
    int len;

    // 서버 IP와 포트번호
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    // socket() - TCP 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");

    // server address 설정
    memset(&serveraddr, 0, sizeof(serveraddr)); 
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    retval = inet_pton(AF_INET, server_ip, &serveraddr.sin_addr);
    if (retval <= 0) {
        if (retval == 0)
            printf("Invalid IP address format.\n");
        else
            err_quit("inet_pton()");
        return -1;
    }

    // connect()
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));  
    if (retval == -1) err_quit("connect()");

    while (1) {

        memset(buf, 0, sizeof(buf));  
        printf("\n[Input message] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        // send() - 서버로 메시지 전송
        retval = send(sock, buf, strlen(buf), 0);
        if (retval == -1) {
            err_display("send()");
            break;
        }
        printf("[TCP Client] %d bytes sent\n", retval);

        // recv() - 서버로부터 에코 메시지 수신
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == -1) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;

        buf[retval] = '\0';
        printf("[TCP Client] %d bytes received: %s\n", retval, buf);
    }

    close(sock);

    return 0;
}