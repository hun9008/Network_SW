#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE 1500

// just fun.
long long cnt = 0;

void err_quit(char *msg) {
    perror(msg);
    exit(-1);
}

void err_display(char *msg) {
    perror(msg);
}

int main(int argc, char* argv[]) {
    int syntax;
    char ip[20] = "";
    int port;

    if(argc != 3) {
        printf("Parameter Error\n");
        return -1;
    } else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

    int listen_sock;
    int client_sock;
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];
    int retval, msglen;

    listen_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_sock == -1) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 소켓 옵션 설정 (재사용 가능하도록 설정)
    int on = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // 논블로킹 소켓 설정 (fcntl 사용)
    int flags = fcntl(listen_sock, F_GETFL, 0);
    fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK);

    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(retval == -1) err_quit("bind()");
    printf("\n[INFO] BIND SUCCESS");

    retval = listen(listen_sock, SOMAXCONN);
    if(retval == -1) err_quit("listen()");

    while(1) {
        // printf("\n[INFO] LISTENING");
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        if(client_sock == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 소켓이 준비되지 않음. 잠시 대기 후 재시도
                usleep(1000);  // 1ms 대기
                continue;
            } else {
                // err_display("accept()");
                cnt++;
                for(int i = 0; i < (cnt / 20000) % 100; i++)
                {
                    printf("|");
                }
                printf("\n");
                continue;
            }
        }

        printf("\n[TCP Server] Client accepted : IP addr=%s, port=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        while(1) {
            msglen = recv(client_sock, buf, BUFSIZE, 0);
            if(msglen == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    // 소켓이 준비되지 않음. 잠시 대기 후 재시도
                    usleep(1000);  // 1ms 대기
                    continue;
                } else {
                    err_display("recv()");
                    break;
                }
            } else if(msglen == 0) {
                break;
            }

            buf[msglen] = '\0';
            printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
                ntohs(clientaddr.sin_port), buf);

            retval = send(client_sock, buf, msglen, 0);
            if(retval == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    // 소켓이 준비되지 않음. 잠시 대기 후 재시도
                    usleep(1000);  // 1ms 대기
                    continue;
                } else {
                    err_display("send()");
                    break;
                }
            }
        }

        close(client_sock);
        printf("[TCP Server] Client disconnected: IP addr=%s, port=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    }

    close(listen_sock);
    return 0;
}