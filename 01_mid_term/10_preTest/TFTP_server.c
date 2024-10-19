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

#define SERVER_DIR "./server_files/"

enum TFTP_OPCODE {
    RRQ = 1,
    WRQ = 2,
    DATA = 3,
    ACK = 4,
    ERROR = 5
};

int sum_messages = 0;
int sum_bytes = 0;
time_t start_time;
time_t current_time;
volatile int running_time = 0;
int server_running = 1;

fd_set main_set, read_fds; 

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

// 클라이언트 요청 처리 함수
void handle_rrq(int client_sock, const char *filename) {
    char filepath[BUFSIZE];
    snprintf(filepath, sizeof(filepath), "%s%s", SERVER_DIR, filename);

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("File not found");
        return;
    }

    char buffer[BUFSIZE + 4];  // TFTP 패킷 헤더 포함
    int block = 1;

    printf("[TFTP Server] Sending file: %s\n", filename);

    while (1) {
        int bytes_read = fread(buffer + 4, 1, BUFSIZE, file);
        if (bytes_read < 0) {
            perror("File read error");
            break;
        }

        // TFTP DATA 패킷 생성
        buffer[0] = 0;
        buffer[1] = DATA;
        buffer[2] = (block >> 8) & 0xFF;
        buffer[3] = block & 0xFF;

        // 데이터 전송
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(client_sock, &write_fds);

        struct timeval timeout = {1, 0};  // 1초 대기
        int ready = select(client_sock + 1, NULL, &write_fds, NULL, &timeout);
        if (ready > 0 && FD_ISSET(client_sock, &write_fds)) {
            int sent = send(client_sock, buffer, bytes_read + 4, 0);
            if (sent < 0) {
                perror("send()");
                break;
            }
        } else {
            printf("[TFTP Server] Client socket not ready for writing.\n");
            break;
        }

        // ACK 수신 대기
        char ack[4];
        FD_ZERO(&write_fds);
        FD_SET(client_sock, &write_fds);
        ready = select(client_sock + 1, &write_fds, NULL, NULL, &timeout);

        if (ready > 0 && FD_ISSET(client_sock, &write_fds)) {
            int received = recv(client_sock, ack, sizeof(ack), 0);
            if (received == 0) {
                printf("[TFTP Server] Client closed the connection.\n");
                break;
            } else if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("[TFTP Server] recv() would block, retrying...\n");
                    usleep(1000);  // 1ms 대기 후 재시도
                    continue;
                } else {
                    perror("[TFTP Server] recv()");
                    break;
                }
            }

            if (received < 4 || ack[1] != ACK || ((ack[2] << 8) | ack[3]) != block) {
                printf("[TFTP Server] ACK error or mismatch: block %d\n", block);
                break;
            } else {
                printf("[TFTP Server] Received ACK for block %d\n", block);
            }
        } else {
            printf("[TFTP Server] Client socket not ready for reading.\n");
            break;
        }

        // 마지막 블록인지 확인
        if (bytes_read < BUFSIZE) {
            printf("[TFTP Server] File transfer complete.\n");
            break;
        }
        block++;
    }

    fclose(file);
}

// // 클라이언트로부터의 요청을 처리하는 함수
// void handle_client(int client_sock, fd_set main_set) {

// }

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

// void broadcast_message(int sender_sock, char *buf, int len) {
//     for (int i = 0; i < MAX_CLIENTS; i++) {
//         if (clients[i].active) {
//             handle_client_request(clients[i].socket); 
//         }
//     }
// }

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
        printf("[INFO] max_fd : %d\n", max_fd);
        if (selected < 0) {
            perror("select()");   
            break;
        }

        if (selected == 0) {
            printf("[INFO] Select() timeout\n");
            continue;
        }

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

                    ClientInfo *client = find_client_by_socket(i);
                    if (client == NULL || !client->active) {
                        printf("Invalid or inactive socket: %d\n", i);
                        close(i);
                        FD_CLR(i, &main_set);  // FD_SET에서 제거
                        continue;
                    }

                    int client_sock = i;

                    char buffer[BUFSIZE];
                    char nickname[20];
                    int total_received = 0;
                    int expected_size = BUFSIZE;

                    // 1. 클라이언트의 닉네임 수신
                    int received = recv(client_sock, nickname, sizeof(nickname) - 1, 0);
                    if (received <= 0) {
                        perror("recv() for nickname");
                        close(client_sock);
                        FD_CLR(client_sock, &main_set);
                        break;
                    }
                    nickname[received] = '\0';  // 닉네임 문자열 종료

                    strcpy(client->nickname, nickname);

                    printf("[SET NICKNAME] : %s\n", nickname);

                    // memset(buffer, 0, sizeof(buffer));  // 버퍼 초기화

                    // 1. TFTP 요청 전체 수신
                    while (total_received < expected_size) {
                        int received = recv(client_sock, buffer + total_received, BUFSIZE - total_received, 0);
                        
                        if (received < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                usleep(1000);  // 1ms 대기 후 재시도
                                continue;
                            } else {
                                perror("recv() for TFTP request");
                                close(client_sock);
                                FD_CLR(client_sock, &main_set);
                                break;
                            }
                        } else if (received == 0) {
                            // printf("[TFTP Server] Client disconnected.\n");
                            remove_client(client_sock);
                            close(client_sock);
                            FD_CLR(client_sock, &main_set); 
                            break;
                        }
                        total_received += received;

                        // 요청이 끝났는지 확인 (마지막 바이트가 '\0'인지 확인)
                        if (buffer[total_received - 1] == '\0') {
                            break;
                        }
                    }

                    // 3. TFTP RRQ 요청인지 확인
                    if (buffer[1] == RRQ) {
                        char *filename = buffer + 2;  // 요청된 파일 이름 추출
                        printf("[TFTP Server] Client requested: %s\n", filename);
                        handle_rrq(client_sock, filename);
                    } else {
                        printf("[TFTP Server] Unsupported request.\n");
                    }

                    // 4. 클라이언트 소켓 종료
                    close(client_sock);
                    FD_CLR(client_sock, &main_set);
                }
            }
        }

    }

    // pthread_join(sThread, NULL);
    close(listen_sock);
    
    // sThread kill
    pthread_cancel(sThread);

    return 0;
}