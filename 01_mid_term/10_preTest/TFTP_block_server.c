#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define BUFSIZE 512
#define SERVER_DIR "./server_files/"

enum TFTP_OPCODE {
    RRQ = 1,
    DATA = 3,
    ACK = 4,
    ERROR = 5
};

volatile int server_running = 1;

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// int set_nonblocking(int sock) {
//     int flags = fcntl(sock, F_GETFL, 0);
//     if (flags < 0) return -1;
//     return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
// }

// 파일을 512바이트 단위로 전송하는 TFTP RRQ 요청 처리
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
        int sent = send(client_sock, buffer, bytes_read + 4, 0);
        if (sent < 0) {
            perror("send()");
            break;
        }

        // ACK 수신 대기
        // handle_rrq 함수 내에서 ACK 수신 부분 수정
        char ack[4];
        int received = recv(client_sock, ack, sizeof(ack), 0);
        if (received == 0) {
            printf("[TFTP Server] Client closed the connection.\n");
            break;
        } else if (received < 0) {
            perror("[TFTP Server] recv()");
            break;
        }

        if (received < 4 || ack[1] != ACK || ((ack[2] << 8) | ack[3]) != block) {
            printf("[TFTP Server] ACK error or mismatch: block %d\n", block);
            perror("[TFTP Server] recv()");
            break;
        } else {
            printf("[TFTP Server] Received ACK for block %d\n", block);
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

// 클라이언트 요청을 처리
void handle_client(int client_sock) {
    char buffer[BUFSIZE];
    int received = recv(client_sock, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        perror("recv()");
        close(client_sock);
        return;
    }

    char nickname[20];
    snprintf(nickname, sizeof(nickname), "Client %d", client_sock);

    received = recv(client_sock, buffer, sizeof(buffer), 0);
    if (received < 0) {
        perror("recv()");
        close(client_sock);
        return;
    }

    // RRQ 요청 처리
    if (buffer[1] == RRQ) {
        char *filename = buffer + 2;  // 요청된 파일 이름 추출
        printf("[TFTP Server] Client requested: %s\n", filename);
        handle_rrq(client_sock, filename);
    } else {
        printf("[TFTP Server] Unsupported request.\n");
    }
    close(client_sock);  // 클라이언트 연결 종료
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) err_quit("socket()");

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // if (set_nonblocking(listen_sock) < 0) err_quit("set_nonblocking()");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        err_quit("bind()");
    if (listen(listen_sock, SOMAXCONN) < 0)
        err_quit("listen()");

    printf("[TFTP Server] Listening on port %d...\n", port);

    fd_set main_set, read_fds;
    FD_ZERO(&main_set);
    FD_SET(listen_sock, &main_set);
    int max_fd = listen_sock;

    while (server_running) {
        read_fds = main_set;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select()");
            break;
        }

        if (FD_ISSET(listen_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
            if (client_sock < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    perror("accept()");
                continue;
            }

            printf("[TFTP Server] New connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            handle_client(client_sock);
        }
    }

    close(listen_sock);
    return 0;
}