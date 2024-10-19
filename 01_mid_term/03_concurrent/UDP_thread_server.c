#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define BUFSIZE 1500
#define MAX_CLIENTS 100

int sum_messages = 0;
int sum_bytes = 0;
time_t start_time;
time_t current_time;
int running_time = 0;

void err_quit(char *msg) {
    perror(msg);
    exit(-1);
}

void err_display(char *msg) {
    perror(msg);
}

void printCommandMenu() {
    printf("****************************************\n");
    printf("*              COMMAND MENU            *\n");
    printf("****************************************\n");
    printf("*      ___                             *\n");
    printf("*     |   |      Press 'i' to get      *\n");
    printf("*     | i |   -> Client Info           *\n");
    printf("*     |___|                            *\n");
    printf("*                                      *\n");
    printf("*      ___                             *\n");
    printf("*     |   |      Press 's' to get      *\n");
    printf("*     | s |   -> Chat Statistics       *\n");
    printf("*     |___|                            *\n");
    printf("*                                      *\n");
    printf("*      ___                             *\n");
    printf("*     |   |      Press 'q' to Quit     *\n");
    printf("*     | q |                            *\n");
    printf("*     |___|                            *\n");
    printf("*                                      *\n");
    printf("****************************************\n");
}

typedef struct {
    struct sockaddr_in addr;  
    int active;      
    char nickname[20]; 
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];  // 클라이언트 정보 저장 배열

void add_client(struct sockaddr_in *clientaddr, char *nickname) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].addr = *clientaddr;
            clients[i].active = 1;
            strcpy(clients[i].nickname, nickname);
            printf("New client added: [%s] %s:%d\n", nickname,
                   inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port));
            return;
        }
    }
    printf("Client list is full!\n");
}

int is_known_client(struct sockaddr_in *clientaddr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active &&
            clients[i].addr.sin_addr.s_addr == clientaddr->sin_addr.s_addr &&
            clients[i].addr.sin_port == clientaddr->sin_port) {
            return 1;  // 이미 등록된 클라이언트
        }
    }
    return 0;  // 새로운 클라이언트
}

void broadcast_message(int sender_sock, char *buf, int len, struct sockaddr_in *senderaddr) {    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active &&
            (clients[i].addr.sin_addr.s_addr != senderaddr->sin_addr.s_addr ||
             clients[i].addr.sin_port != senderaddr->sin_port)) {
            // 메시지를 보낸 클라이언트를 제외한 나머지에게 메시지를 전송
            int retval = sendto(sender_sock, buf, len, 0, (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr));
            if (retval < 0) {
                perror("send()");
            }
        }
    }
}

void *process_client(void *arg) {
    int client_sock = *(int *)arg;
    struct sockaddr_in clientaddr;
    char buf[BUFSIZE + 1];
    int retval;
    socklen_t addrlen;

    while (1) {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(client_sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
        if (retval <= 0) {
            if (retval < 0) perror("recv()");
            break;
        }

        buf[retval] = '\0';
        printf("Message from client (%s:%d): %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);
        sum_messages++;
        sum_bytes += retval;

        // buf는 [nickname] message 형태로 전송됨. 이 중 nickname을 추출
        char nickname[20];
        int i, j = 0;
        for (i = 1; i < retval; i++) {  
            if (buf[i] == ']') {
                break;
            }
            nickname[j++] = buf[i];
        }
        nickname[j] = '\0';

        // 새로운 클라이언트라면 추가
        if (!is_known_client(&clientaddr)) {
            add_client(&clientaddr, nickname);
        }

        // 에코 메시지: 보낸 클라이언트를 제외한 모든 클라이언트에게 전송
        broadcast_message(client_sock, buf, retval, &clientaddr);
    }

    close(client_sock);
    return NULL;
}

void printClientInfo() {
    int active_clients_num = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].nickname[0] != '\0') {  // 닉네임이 설정된 클라이언트만 카운트
            if (clients[i].active)
                active_clients_num++;
        }
    }

    printf("****************************************\n");
    printf("*              CLIENT INFO             *\n");
    printf("****************************************\n");
    printf("*           Client Number : %d          *\n", active_clients_num);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].nickname[0] != '\0') {  // 닉네임이 설정된 클라이언트만 출력
            printf("* %-11s %-8s %s:%d *\n",
                   clients[i].nickname,
                   clients[i].active ? "active" : "inactive",
                   inet_ntoa(clients[i].addr.sin_addr),
                   ntohs(clients[i].addr.sin_port));
        }
    }
    printf("****************************************\n");
}

void printChatStatistics() {
    current_time = time(NULL);
    running_time = (int)difftime(current_time, start_time);

    printf("****************************************\n");
    printf("*           CHAT STATISTICS            *\n");
    printf("****************************************\n");
    printf("* Msg/min : %-27d*\n", sum_messages / running_time * 60);
    printf("* Bytes/min : %-25d*\n", sum_bytes / running_time * 60);
    printf("* Total Msgs : %-24d*\n", sum_messages);
    printf("* Total Bytes : %-23d*\n", sum_bytes);
    printf("* Total Time : %-24d*\n", running_time);
    printf("****************************************\n");
}

void printQuit() {
    printf("****************************************\n");
    printf("*              I'm Quit!               *\n");
    printf("****************************************\n");
    exit(0);
}

void *process_stocastic() {
    char command[2];

    while (1) {
        if (fgets(command, 2, stdin) == NULL)
            return 0;

        if (command[0] == 'i') {
            printClientInfo();
        } else if (command[0] == 's') {
            printChatStatistics();
        } else if (command[0] == 'q') {
            printQuit();
        } else {
            printf("*  i - info || s - static || q - quit  *\n");
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {

    int retval;
    int sock;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen;
    pthread_t stocastic_tid, message_tid;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval < 0) err_quit("bind()");

    start_time = time(NULL);

    printCommandMenu();
    printf("[UDP SERVER READY]\n");

    retval = pthread_create(&stocastic_tid, NULL, process_stocastic, NULL);
    if (retval != 0) err_quit("pthread_create()");

    retval = pthread_create(&message_tid, NULL, process_client, &sock);
    if (retval != 0) err_quit("pthread_create()");

    // wait stocastic thread
    pthread_join(stocastic_tid, NULL);
    close(sock);

    return 0;
}