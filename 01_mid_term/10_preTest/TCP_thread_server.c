#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define BUFSIZE 512
#define MAX_CLIENTS 100

int sum_messages = 0;
int sum_bytes = 0;
time_t start_time;
time_t current_time;
volatile int running_time = 0;
int server_running = 1;

typedef struct {
    int socket;
    struct sockaddr_in addr;
    int active;
    char nickname[20];
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
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

void add_client(int client_sock, struct sockaddr_in *clientaddr, char *nickname) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].socket = client_sock;
            clients[i].addr = *clientaddr;
            clients[i].active = 1;
            strcpy(clients[i].nickname, nickname);
            printf("New client added: [%s] %s:%d\n", nickname, inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port));
            return;
        }
    }
    printf("Client list is full!\n");
}

void remove_client(int client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_sock) {
            clients[i].active = 0;
            close(clients[i].socket);
            printf("Client disconnected: %s:%d\n", inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
            return;
        }
    }
}

void broadcast_message(int sender_sock, char *buf, int len) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket != sender_sock) {
            if (send(clients[i].socket, buf, len, 0) < 0) {
                perror("send()");
            }
        }
    }
}

void *process_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char buf[BUFSIZE + 1];
    int retval;

    while (server_running) {
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval <= 0) {
            if (retval < 0) perror("recv()");
            remove_client(client_sock);
            break;
        }
        buf[retval] = '\0';
        printf("Message from client: %s\n", buf);
        sum_messages++;
        sum_bytes += retval;

        broadcast_message(client_sock, buf, retval);
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
    printf("* Msg/min : %-27d*\n", sum_messages / (running_time ? running_time : 1) * 60);
    printf("* Bytes/min : %-25d*\n", sum_bytes / (running_time ? running_time : 1) * 60);
    printf("* Total Msgs : %-24d*\n", sum_messages);
    printf("* Total Bytes : %-23d*\n", sum_bytes);
    printf("* Total Time : %-24d*\n", running_time);
    printf("****************************************\n");
}

void printQuit() {
    printf("****************************************\n");
    printf("*              I'm Quit!               *\n");
    printf("****************************************\n");
    server_running = 0; // 서버 종료 플래그 설정
    exit(0);
}

void *process_stocastic(void *arg) {
    char command[2];
    while (server_running) {
        if (fgets(command, 2, stdin) == NULL) break;
        if (command[0] == 'i') {
            printClientInfo();
        } else if (command[0] == 's') {
            printChatStatistics();
        } else if (command[0] == 'q') {
            printQuit();
            break;
        } else {
            printf("*  i - info || s - static || q - quit  *\n");
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) err_quit("socket()");

    struct sockaddr_in serveraddr = {0};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        err_quit("bind()");
    
    if (listen(listen_sock, SOMAXCONN) < 0)
        err_quit("listen()");

    start_time = time(NULL);
    printCommandMenu();
    printf("[TCP SERVER READY] Listening on port %d...\n", port);

    pthread_t sThread;
    pthread_create(&sThread, NULL, process_stocastic, NULL);

    while (server_running) {
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        if (*client_sock < 0) {
            perror("accept()");
            free(client_sock);
            continue;
        }

        char nickname[20];
        int retval = recv(*client_sock, nickname, sizeof(nickname) - 1, 0);
        if (retval <= 0) {
            close(*client_sock);
            free(client_sock);
            continue;
        }
        nickname[retval] = '\0';
        add_client(*client_sock, &clientaddr, nickname);

        pthread_t mThread;
        pthread_create(&mThread, NULL, process_client, client_sock);
        pthread_detach(mThread);
    }

    pthread_join(sThread, NULL);
    close(listen_sock);
    return 0;
}