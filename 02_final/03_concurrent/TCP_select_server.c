#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#define BUFSIZE 1500
#define MAX_CLIENTS 100

int sum_messages = 0;
int sum_bytes = 0;
time_t start_time;
time_t current_time;
volatile int running_time = 0;
int server_running = 1;  // 서버 실행 상태 플래그

typedef struct {
    int socket;
    struct sockaddr_in addr;
    int active;
    char nickname[20];
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];  // 클라이언트 정보 저장 배열

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void add_client(int client_sock, struct sockaddr_in *clientaddr, const char *nickname) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].socket = client_sock;
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

void remove_client(int client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_sock) {
            clients[i].active = 0;
            close(clients[i].socket);
            printf("Client disconnected: %s:%d\n",
                   inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
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

void printClientInfo() {
    printf("****************************************\n");
    printf("*              CLIENT INFO             *\n");
    printf("****************************************\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            printf("* %-11s %-8s %s:%d *\n",
                   clients[i].nickname,
                   "active",
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
    printf("* Msg/min : %-27d *\n", sum_messages / (running_time > 0 ? running_time : 1));
    printf("* Bytes/min : %-25d *\n", sum_bytes / (running_time > 0 ? running_time : 1));
    printf("* Total Msgs : %-24d *\n", sum_messages);
    printf("* Total Bytes : %-23d *\n", sum_bytes);
    printf("* Total Time : %-24d *\n", running_time);
    printf("****************************************\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int listen_sock, client_sock, port = atoi(argv[1]);
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen;
    fd_set readfds, allfds;
    char buf[BUFSIZE + 1];

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        err_quit("bind()");

    if (listen(listen_sock, SOMAXCONN) < 0)
        err_quit("listen()");

    printf("[SERVER] Listening on port %d...\n", port);

    FD_ZERO(&allfds);
    FD_SET(listen_sock, &allfds);
    int maxfd = listen_sock;

    while (server_running) {
        readfds = allfds;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select()");
            break;
        }

        if (FD_ISSET(listen_sock, &readfds)) {
            addrlen = sizeof(clientaddr);
            client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
            if (client_sock < 0) {
                perror("accept()");
                continue;
            }

            printf("New client connected: %s:%d\n",
                   inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

            FD_SET(client_sock, &allfds);
            if (client_sock > maxfd) maxfd = client_sock;

            char nickname[20];
            int retval = recv(client_sock, nickname, sizeof(nickname) - 1, 0);
            if (retval <= 0) {
                close(client_sock);
                FD_CLR(client_sock, &allfds);
            } else {
                nickname[retval] = '\0';
                add_client(client_sock, &clientaddr, nickname);
            }
        }

        for (int i = 0; i <= maxfd; i++) {
            if (i != listen_sock && FD_ISSET(i, &readfds)) {
                int retval = recv(i, buf, BUFSIZE, 0);
                if (retval <= 0) {
                    if (retval < 0) perror("recv()");
                    close(i);
                    FD_CLR(i, &allfds);
                    remove_client(i);
                } else {
                    buf[retval] = '\0';
                    printf("Message from client: %s\n", buf);
                    sum_messages++;
                    sum_bytes += retval;
                    broadcast_message(i, buf, retval);
                }
            }
        }
    }

    close(listen_sock);
    return 0;
}