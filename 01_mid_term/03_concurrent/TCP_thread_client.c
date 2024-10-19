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

// 메시지 송신을 담당하는 함수
void *SendMessageThread(void *arg) {
    int sock = *(int *)arg;
    char buf[BUFSIZE + 1];
    char temp[BUFSIZE + 1];
    char nickname[20];
    int retval;

    printf("\n[SET NICKNAME] : ");
    
    if (fgets(nickname, 20, stdin) == NULL)
        return NULL;

    int nlen = strlen(nickname);
    if (nickname[nlen - 1] == '\n')
        nickname[nlen - 1] = '\0';

    retval = send(sock, nickname, strlen(nickname), 0);
    if (retval == -1) {
        err_display("send()");
    }
    printf("[TCP Client] Sent nickname.\n");

    while (1) {
        printf("\n[Input Message] ");
        if (fgets(temp, BUFSIZE + 1, stdin) == NULL)
            break;

        sprintf(buf, "[%s] %s", nickname, temp);
        int len = strlen(buf);

        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (strlen(buf) == 0) {
            printf("Message is empty\n");
            continue;
        }

        retval = send(sock, buf, strlen(buf), 0);
        if (retval == -1) {
            err_display("send()");
            continue;
        }

        printf("[TCP Client] Sent %d bytes.\n", retval);
    }

    return NULL;
}

// 메시지 수신을 담당하는 함수
void *ReceiveMessageThread(void *arg) {
    int sock = *(int *)arg;
    char buf[BUFSIZE + 1];
    int retval;

    while (1) {
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == -1) {
            err_display("recv()");
            exit(EXIT_FAILURE);
        } else if (retval == 0) {
            printf("Server closed the connection.\n");
            break;
        }

        buf[retval] = '\0';
        printf("[TCP Client] Received %d bytes from server.\n", retval);
        printf("[Received Message] %s\n", buf);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    char ip[20] = "";
    int port;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    } else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

    int sock;
    struct sockaddr_in serveraddr;
    int retval;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == -1) err_quit("connect()");

    printf("[TCP Client] Connected to the server.\n");

    pthread_t sendThread, recvThread;

    if (pthread_create(&sendThread, NULL, SendMessageThread, &sock) != 0)
        err_quit("pthread_create() for send");

    if (pthread_create(&recvThread, NULL, ReceiveMessageThread, &sock) != 0)
        err_quit("pthread_create() for recv");

    pthread_join(recvThread, NULL);
    close(sock);

    return 0;
}