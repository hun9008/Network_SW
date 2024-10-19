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
int running_time = 0;
int server_running = 1; // 서버 실행 상태 플래그

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void err_display(const char *msg) {
    perror(msg);
}

typedef struct {
    struct sockaddr_in addr;
    int active;
    char nickname[20];
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];

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

void add_client(struct sockaddr_in *clientaddr, const char *nickname) {
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

void broadcast_message(int sock, const char *buf, int len, struct sockaddr_in *senderaddr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active &&
            (clients[i].addr.sin_addr.s_addr != senderaddr->sin_addr.s_addr ||
             clients[i].addr.sin_port != senderaddr->sin_port)) {
            if (sendto(sock, buf, len, 0, 
                       (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr)) < 0) {
                perror("sendto()");
            }
        }
    }
}

void *process_client(void *arg) {
    int sock = *(int *)arg;
    struct sockaddr_in clientaddr;
    char buf[BUFSIZE + 1];
    socklen_t addrlen;
    int retval;

    while (server_running) {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0, 
                          (struct sockaddr *)&clientaddr, &addrlen);
        if (retval < 0) {
            perror("recvfrom()");
            continue;
        }

        buf[retval] = '\0';
        printf("Message from client (%s:%d): %s\n",
               inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);
        sum_messages++;
        sum_bytes += retval;

        char nickname[20];
        int i, j = 0;
        for (i = 1; i < retval; i++) {
            if (buf[i] == ']') break;
            nickname[j++] = buf[i];
        }
        nickname[j] = '\0';

        if (strlen(nickname) > 0) {
            add_client(&clientaddr, nickname);
        }

        broadcast_message(sock, buf, retval, &clientaddr);
    }

    return NULL;
}


void printClientInfo() {
    int active_clients_num = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            active_clients_num++;
        }
    }

    printf("****************************************\n");
    printf("*              CLIENT INFO             *\n");
    printf("****************************************\n");
    printf("*           Client Number : %d          *\n", active_clients_num);

    printf("* * * * * * * * * * * * * * * * * * * **\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            char temp_buf[41];
            snprintf(temp_buf, 38, " %-19s\t%s:%d", 
             clients[i].nickname, inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));

            printf("*%s*\n", temp_buf);
        }
    }
    printf("****************************************\n");
}

void printChatStatistics() {
    // running_time을 갱신
    current_time = time(NULL);
    running_time = (int)difftime(current_time, start_time);
    int minute = running_time / 60.0;

    if (minute == 0) minute = 1;

    printf("****************************************\n");
    printf("*           CHAT STATISTICS            *\n");
    printf("****************************************\n");
    printf("* Msg/min : %-26f *\n", (float)sum_messages / minute);
    printf("* Bytes/min : %-24f *\n", (float)sum_bytes / minute);
    printf("* Total Msgs : %-23d *\n", sum_messages);
    printf("* Total Bytes : %-22d *\n", sum_bytes);
    printf("* Total Time : %-23d *\n", running_time);
    printf("****************************************\n");
}


void printQuit() {
    printf("****************************************\n");
    printf("*              I'm Quit!               *\n");
    printf("****************************************\n");
}

void *process_commands(void *arg) {
    char command[2];

    while(1) {
        if (fgets(command, 2, stdin) == NULL) 
            return 0;
        
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

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int sock;
    struct sockaddr_in serveraddr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        err_quit("bind()");
    }

    start_time = time(NULL);

    printCommandMenu();
   
   
    pthread_t client_thread, command_thread;
    pthread_create(&client_thread, NULL, process_client, &sock);
    pthread_create(&command_thread, NULL, process_commands, NULL);

    pthread_join(client_thread, NULL);
    pthread_join(command_thread, NULL);

    close(sock);
    return 0;
}