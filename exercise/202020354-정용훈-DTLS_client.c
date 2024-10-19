#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 1500

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

    method = DTLS_client_method(); 
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in addr;
    SSL_CTX *ctx;
    SSL *ssl;

    initialize_openssl();
    ctx = create_context();
    configure_context(ctx);  // 인증서 및 키 설정

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to connect");
        exit(EXIT_FAILURE);
    }

    // Create DTLS session
    BIO *bio = BIO_new_dgram(sockfd, BIO_NOCLOSE);
    ssl = SSL_new(ctx);
    SSL_set_bio(ssl, bio, bio);


    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);  
        printf("SSL_connect failed\n"); 
        exit(EXIT_FAILURE);
    } else {
        char buffer[BUFFER_SIZE];

        while (1) {

            printf("Enter message to send to server (type 'exit' to quit): ");
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                break;
            }

            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            }

            SSL_write(ssl, buffer, strlen(buffer));

            memset(buffer, 0, sizeof(buffer));
            int bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
            if (bytes_received <= 0) {
                printf("Connection closed or error occurred.\n");
                break;
            }

            buffer[bytes_received] = '\0';
            printf("Received from server: %s\n", buffer);
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    return 0;
}