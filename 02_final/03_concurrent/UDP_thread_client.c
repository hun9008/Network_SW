#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define BUFSIZE 1500

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

void* SendMessageThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int sock = args->sock;
    struct sockaddr_in* serveraddr = &args->serveraddr;
    char buf[BUFSIZE + 1];
    char temp[BUFSIZE + 1];
    char nickname[20];

    printf("\n[SET NICKNAME] : ");
    
    if (fgets(nickname, 20, stdin) == NULL)
        return NULL;

    int nlen = strlen(nickname);
    if (nickname[nlen - 1] == '\n')
        nickname[nlen - 1] = '\0';

    while (1) {
        printf("\n[Input Message] ");
        if (fgets(temp, BUFSIZE + 1, stdin) == NULL)
            break;

        snprintf(buf, BUFSIZE, "[%s] %s", nickname, temp);

        int len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (strlen(buf) == 0) {
            printf("Message is empty\n");
            continue;
        }

        // 서버로 메시지 전송
        ssize_t retval = sendto(sock, buf, strlen(buf), 0, 
                                (struct sockaddr*)serveraddr, sizeof(*serveraddr));
        if (retval == -1) {
            perror("sendto()");
            continue;
        }

        printf("[UDP Client] Sent %ld bytes.\n", retval);
    }
    
    return NULL;
}

// 메시지 수신을 담당하는 함수
void* ReceiveMessageThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int sock = args->sock;
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);
    char buf[BUFSIZE + 1];

    while (1) {
        // 메시지 수신
        ssize_t retval = recvfrom(sock, buf, BUFSIZE, 0, 
                                  (struct sockaddr*)&clientaddr, &addrlen);
        if (retval == -1) {
            perror("recvfrom()");
            continue;
        }

        buf[retval] = '\0';  // 수신된 메시지 처리
        printf("[UDP Client] Received %ld bytes from server.\n", retval);
        printf("[Received Message] %s\n", buf);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    char ip[20];
    int port;

    // 파라미터 확인
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    } else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

    // UDP 소켓 생성
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) err_quit("socket()");

    // 클라이언트 소켓을 바인딩할 로컬 주소 설정
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(0);  // 임의의 포트를 사용
    clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 로컬 주소

    // 클라이언트 소켓을 바인딩
    if (bind(sock, (struct sockaddr*)&clientaddr, sizeof(clientaddr)) == -1)
        err_quit("bind()");

    // 서버 주소 설정
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serveraddr.sin_addr) <= 0)
        err_quit("inet_pton()");

    // ThreadArgs 구조체 생성하여 소켓과 서버 주소 전달
    ThreadArgs args;
    args.sock = sock;
    args.serveraddr = serveraddr;

    // 스레드 생성
    pthread_t sendThread, recvThread;

    if (pthread_create(&sendThread, NULL, SendMessageThread, &args) != 0)
        err_quit("pthread_create() for send");

    if (pthread_create(&recvThread, NULL, ReceiveMessageThread, &args) != 0)
        err_quit("pthread_create() for recv");

    // 스레드 종료 대기
    pthread_join(sendThread, NULL);
    pthread_join(recvThread, NULL);

    close(sock);

    return 0;
}