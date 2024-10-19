#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define BUFSIZE 512

int sock;  // 소켓 전역 변수

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void err_display(const char *msg) {
    perror(msg);
}

// SIGIO 신호 처리기 함수
void sigio_handler(int signo) {
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    char buf[BUFSIZE + 1];
    int retval;

    // 데이터 수신
    retval = recvfrom(sock, buf, BUFSIZE, 0, 
                      (struct sockaddr *)&clientaddr, &addrlen);
    if (retval == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            err_display("recvfrom()");
        }
        return;
    }

    buf[retval] = '\0';  // 문자열 종료
    printf("[UDP/%s:%d] %s\n", 
           inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);

    // 에코 메시지 전송
    retval = sendto(sock, buf, retval, 0, 
                    (struct sockaddr *)&clientaddr, addrlen);
    if (retval == -1) {
        err_display("sendto()");
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serveraddr;
    struct sigaction sa;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    // 소켓 생성
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) err_quit("socket()");

    // 서버 주소 설정
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    // 소켓 바인딩
    if (bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
        err_quit("bind()");

    printf("[INFO] BIND SUCCESS\n");

    // SIGIO 신호 처리기 등록
    sa.sa_handler = sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGIO, &sa, NULL) == -1) err_quit("sigaction()");

    // 소켓 비동기 설정
    fcntl(sock, F_SETOWN, getpid());  // 현재 프로세스에 소켓 소유권 부여
    fcntl(sock, F_SETFL, O_NONBLOCK | O_ASYNC);  // 논블로킹 및 비동기 모드 설정

    // 서버가 종료될 때까지 대기
    while (1) {
        pause();  // SIGIO 신호 대기
    }

    close(sock);
    return 0;
}