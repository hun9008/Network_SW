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
volatile int running_time = 0;
int server_running = 1; // 서버 실행 상태 플래그

int contents_db_len = 3;

void err_quit(char *msg) {
    perror(msg);
    exit(-1);
}

void err_display(char *msg) {
    perror(msg);
}

typedef struct {
    char channel_number[3];
    char contents_title[BUFSIZE];
    struct sockaddr_in addr;
} Contents_DB;

Contents_DB contents_db[3];

void *process_client(void *arg) {
    int client_sock = *(int *)arg;
    char buf[BUFSIZE + 1];
    int retval;

    while(1) {
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval <= 0) {
            if (retval < 0) perror("recv()");
            return NULL;
        }  
        buf[retval] = '\0';
        printf("Message from client: %s\n", buf);

        if (buf[0] == '1') {

            strncpy(contents_db[0].channel_number, &buf[1], 2);
            char ip[20];
            strncpy(ip, &buf[3], 12);
            char port[6];
            strncpy(port, &buf[15], 5);
            char title[50];
            strncpy(title, &buf[20], 50);

            // printf("[DEBUG] %s\n%s\n%s\n", ip, port, title);
            contents_db[0].addr.sin_addr.s_addr = inet_addr(ip);
            contents_db[0].addr.sin_port = htons(atoi(port));
            strcpy(contents_db[0].contents_title, title);

            
            
            printf("[%s:%d] channel %s, title : %s\n", inet_ntoa(contents_db[0].addr.sin_addr),
            ntohs(contents_db[0].addr.sin_port), contents_db[0].channel_number,
            contents_db[0].contents_title);


            // broadcast_message(client_sock, buf, retval);
            printf("sent %s\n", buf);
            retval = send(client_sock, buf, BUFSIZE, 0);

        } else if (buf[0] == '2') {
            
            char idx = '4';
            strcpy(buf, &idx);
            strcat(buf, "3");

            for(int i = 0; i < contents_db_len; i++)
            {
                strcat(buf, contents_db[i].channel_number);
                strcat(buf, inet_ntoa(contents_db[i].addr.sin_addr));
                strcat(buf, "10100");
                strcat(buf, contents_db[i].contents_title);
            }

            retval = send(client_sock, buf, BUFSIZE, 0);

        } 
    }   

    close(client_sock);
    return NULL;
}

int main(int argc, char *argv[]) {

    int retval;
    int listen_sock, client_sock;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen;
    pthread_t stocastic_tid, message_tid;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval < 0) err_quit("bind()");

    retval = listen(listen_sock, SOMAXCONN);
    if (retval < 0) err_quit("listen()");

    printf("[SERVER] Listening on port %d...\n", port);

    while (server_running) {
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        if (client_sock < 0) {
            perror("accept()");
            continue;
        }

        pthread_create(&message_tid, NULL, process_client, &client_sock);
    }

    close(listen_sock);
    return 0;
}