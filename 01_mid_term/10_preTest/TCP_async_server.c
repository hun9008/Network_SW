#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <fcntl.h>
#include <errno.h>

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

int set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
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

ClientInfo* find_client_by_socket(int sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == sock && clients[i].active) {
            return &clients[i];
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

    int listen_sock, max_fd;
    fd_set main_set, read_fds; 
    
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) err_quit("socket()");

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (set_nonblocking(listen_sock) < 0) err_quit("set_nonblocking()");

    struct sockaddr_in serveraddr = {0};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        err_quit("bind()");
    
    if (listen(listen_sock, SOMAXCONN) < 0)
        err_quit("listen()");

    FD_ZERO(&main_set);
    FD_SET(listen_sock, &main_set);
    max_fd = listen_sock;
    printf("[DEBUG] Added listen_sock: %d\n", listen_sock);

    start_time = time(NULL);
    printCommandMenu();
    printf("[TCP SERVER READY] Listening on port %d...\n", port);

    pthread_t sThread;
    pthread_create(&sThread, NULL, process_stocastic, NULL);

    while (server_running) {

        struct timeval timeout;
        timeout.tv_sec = 5;  // 5초 동안 대기
        timeout.tv_usec = 0;

        read_fds = main_set;
        int selected = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        printf("[INFO] Select() returns : %d\n", selected);
        if (selected < 0) {
            perror("select()");
            break;
        }

        if (selected == 0) {
            printf("[INFO] Select() timeout\n");
            continue;
        }

        printf("[INFO] Select() returns\n");

        for (int i = 0; i <= max_fd; i++)
        {
            // printf("socket : %d\n", i);
            if (FD_ISSET(i, &read_fds)) {
                printf("[DEBUG] pass FD_ISSET\n");
                if (i == listen_sock) {

                    printf("[DEBUF] find listen_sock\n");

                    struct sockaddr_in clientaddr;
                    socklen_t addrlen = sizeof(clientaddr);
                    int *client_sock = malloc(sizeof(int));
                    *client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
                    if (*client_sock < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 소켓이 준비되지 않음
                            printf("accept() would block, retrying...\n");
                            free(client_sock);
                            continue;
                        } else {
                            perror("accept()");
                            free(client_sock);
                            continue;
                        }
                    }

                    if (set_nonblocking(*client_sock) < 0) {
                        perror("set_nonblocking()");
                        close(*client_sock);
                        free(client_sock);
                        continue;
                    }

                    printf("New connection from %s:%d\n",
                           inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

                    // char nickname[20] = "undefined";

                    add_client(*client_sock, &clientaddr, "undefined");

                    FD_SET(*client_sock, &main_set);
                    if (*client_sock > max_fd) max_fd = *client_sock;

                } else {
                    printf("[DEBUG] not find listen socket\n");
                    char buf[BUFSIZE + 1];
                    int retval = recv(i, buf, BUFSIZE, 0);
                    if (retval < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            printf("recv() would block, retrying...\n");
                            continue;
                        } else {
                            perror("recv()");
                            close(i);
                            FD_CLR(i, &main_set);
                            continue;
                        }
                    }

                    if (retval == 0) {
                        printf("Client disconnected.\n");
                        close(i);
                        FD_CLR(i, &main_set);
                    } else {
                        
                        ClientInfo *client = find_client_by_socket(i);
                        if (client == NULL) {
                            printf("Unknown client socket: %d\n", i);
                            continue;
                        }

                        // 클라이언트의 닉네임이 설정되지 않은 경우
                        if (strcmp(client->nickname, "undefined") == 0) {
                            printf("Received nickname: %s\n", buf);
                            strcpy(client->nickname, buf);
                        } else {
                            printf("Message from client (%s): %s\n", client->nickname, buf);
                            broadcast_message(i, buf, retval);
                        }
                    }
                }
            }
        }

    }

    pthread_join(sThread, NULL);
    close(listen_sock);
    return 0;
}