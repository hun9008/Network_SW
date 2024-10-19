#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define BUFSIZE 512

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[BUFSIZE];

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) err_quit("socket()");

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 바인딩
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err_quit("bind()");
    }
    printf("[INFO] UDP Server started on %s:%d\n", ip, port);

    // select를 위한 fd_set 설정
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    while (1) {
        fd_set temps = readfds;

        // select를 사용하여 대기
        int retval = select(sockfd + 1, &temps, NULL, NULL, NULL);
        if (retval < 0) err_quit("select()");

        if (FD_ISSET(sockfd, &temps)) {
            // 데이터 수신
            int n = recvfrom(sockfd, buffer, BUFSIZE, 0, 
                             (struct sockaddr *)&client_addr, &addrlen);
            if (n < 0) {
                perror("recvfrom()");
                continue;
            }

            buffer[n] = '\0';
            printf("[UDP/%s:%d] %s\n", inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port), buffer);

            // 에코 응답
            if (sendto(sockfd, buffer, n, 0, 
                       (struct sockaddr *)&client_addr, addrlen) < 0) {
                perror("sendto()");
            }
        }
    }

    close(sockfd);
    return 0;
}