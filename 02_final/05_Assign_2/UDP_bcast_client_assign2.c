#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define BUFSIZE 512

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void err_display(const char *msg) {
    perror(msg);
}

typedef struct {
    int sock;
    struct sockaddr_in serveraddr;
} ThreadArgs;

// 메시지 송신을 담당하는 함수
void *SendMessageThread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int sock = args->sock;
    struct sockaddr_in *serveraddr = &args->serveraddr;
    char buf[BUFSIZE + 1];
    char temp[BUFSIZE + 1];
    char nickname[20];

    printf("\n[SET NICKNAME]: ");
    if (fgets(nickname, 20, stdin) == NULL)
        return NULL;

    int nlen = strlen(nickname);
    if (nickname[nlen - 1] == '\n')
        nickname[nlen - 1] = '\0';

    while (1) {
        printf("\n[Input Message]: ");
        if (fgets(temp, BUFSIZE + 1, stdin) == NULL)
            break;

        snprintf(buf, sizeof(buf), "[%s] %s", nickname, temp);
        int len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (sendto(sock, buf, strlen(buf), 0, 
                   (struct sockaddr *)serveraddr, sizeof(*serveraddr)) < 0) {
            err_display("sendto()");
            continue;
        }

        printf("[UDP Client] Sent %d bytes.\n", (int)strlen(buf));
    }

    return NULL;
}

// 메시지 수신을 담당하는 함수
void *ReceiveMessageThread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int sock = args->sock;
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    char buf[BUFSIZE + 1];

    while (1) {
        int retval = recvfrom(sock, buf, BUFSIZE, 0, 
                              (struct sockaddr *)&clientaddr, &addrlen);
        if (retval < 0) {
            err_display("recvfrom()");
            continue;
        }

        buf[retval] = '\0';
        printf("[UDP Client] Received %d bytes from server.\n", retval);
        printf("[Received Message]: %s\n", buf);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) err_quit("socket()");

    // 클라이언트 소켓 바인딩
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(0);  // 임의의 포트 사용
    clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
        err_quit("bind()");
    }

    // 서버 주소 설정
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    // 스레드 생성에 필요한 구조체 초기화
    ThreadArgs args;
    args.sock = sock;
    args.serveraddr = serveraddr;

    pthread_t sendThread, recvThread;

    // 송신 및 수신 스레드 생성
    if (pthread_create(&sendThread, NULL, SendMessageThread, &args) != 0) {
        err_quit("pthread_create() for send");
    }

    if (pthread_create(&recvThread, NULL, ReceiveMessageThread, &args) != 0) {
        err_quit("pthread_create() for recv");
    }

    // 스레드가 종료될 때까지 대기
    pthread_join(sendThread, NULL);
    pthread_join(recvThread, NULL);

    close(sock);
    return 0;
}