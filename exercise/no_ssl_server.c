#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("Invalid port number. Please provide a valid port number between 1 and 65535.\n");
        return 1;
    }

    int sockfd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);
    int bytes_received;

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err_quit("socket() failed");
    }

    // 서버 주소 구조체 초기화
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 바인딩
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err_quit("bind() failed");
    }

    // 연결 대기
    if (listen(sockfd, 1) < 0) {
        err_quit("listen() failed");
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        // 클라이언트 연결 수락
        client_sock = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("accept() failed");
            continue;
        }

        printf("Client connected: IP %s, Port %d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 클라이언트로부터 데이터 수신 및 에코
        while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';  // 문자열 끝
            printf("Received message: %s\n", buffer);

            // 클라이언트에게 다시 전송 (에코)
            send(client_sock, buffer, bytes_received, 0);
        }

        if (bytes_received < 0) {
            perror("recv() failed");
        }

        printf("Client disconnected: IP %s, Port %d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        close(client_sock);  // 클라이언트 소켓 닫기
    }

    close(sockfd);  // 서버 소켓 닫기
    return 0;
}