#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

void initialize_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in addr;
    SSL_CTX *ctx;
    SSL *ssl;

    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        printf("Invalid port number. Please provide a valid port number between 1 and 65535.\n");
        return 1;
    }

    initialize_openssl();
    ctx = create_context();

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to connect");
        exit(EXIT_FAILURE);
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        char buffer[BUFFER_SIZE];

        while (1) {
            // 사용자 입력 받기
            printf("Enter message to send to server (type 'exit' to quit): ");
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                break;
            }

            // 'exit' 입력시 종료
            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            }

            // 서버로 메시지 전송
            SSL_write(ssl, buffer, strlen(buffer));

            // 서버로부터 에코 메시지 수신
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
            if (bytes_received <= 0) {
                printf("Connection closed or error occurred.\n");
                break;
            }

            printf("Received from server: %s\n", buffer);
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    cleanup_openssl();
}