#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define BUFSIZE 1500

void err_quit(char *msg) {
    perror(msg);
    exit(-1);
}

void err_display(char *msg) {
    perror(msg);
}

int main(int argc, char *argv[]) {
    int listen_sock, client_sock, max_fd;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];
    int retval, msglen;
    fd_set reads, temps;

    if (argc != 3) {
        printf("Parameter Error\n");
        return -1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    printf("IP : %s\n", ip);
    printf("Port : %d\n", port);

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) err_quit("socket()");

    // 소켓 설정 및 바인딩
    int on = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    fcntl(listen_sock, F_SETFL, O_NONBLOCK);

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == -1) err_quit("bind()");
    printf("\n[INFO] BIND SUCCESS");

    retval = listen(listen_sock, SOMAXCONN);
    if (retval == -1) err_quit("listen()");

    // fd_set 초기화
    FD_ZERO(&reads);
    FD_SET(listen_sock, &reads);
    max_fd = listen_sock;

    while (1) {
        temps = reads;

        // select() 호출로 I/O 이벤트 감지
        retval = select(max_fd + 1, &temps, NULL, NULL, NULL);
        if (retval == -1) err_quit("select()");

        // 수신 가능한 소켓 확인
        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &temps)) {
                if (i == listen_sock) {
                    // 클라이언트 연결 수락
                    addrlen = sizeof(clientaddr);
                    client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
                    if (client_sock == -1) {
                        if (errno != EWOULDBLOCK) err_display("accept()");
                        continue;
                    }
                    printf("\n[TCP Server] Client accepted: IP addr=%s, port=%d\n",
                           inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

                    FD_SET(client_sock, &reads);
                    if (client_sock > max_fd) max_fd = client_sock;
                } else {
                    // 클라이언트로부터 메시지 수신
                    msglen = recv(i, buf, BUFSIZE, 0);
                    if (msglen <= 0) {
                        if (msglen == 0) {
                            printf("[TCP Server] Client disconnected: fd=%d\n", i);
                        } else {
                            err_display("recv()");
                        }
                        close(i);
                        FD_CLR(i, &reads);
                    } else {
                        buf[msglen] = '\0';
                        printf("[TCP/%d] %s\n", i, buf);

                        // 클라이언트에 메시지 에코
                        retval = send(i, buf, msglen, 0);
                        if (retval == -1) {
                            err_display("send()");
                            close(i);
                            FD_CLR(i, &reads);
                        }
                    }
                }
            }
        }
    }

    close(listen_sock);
    return 0;
}