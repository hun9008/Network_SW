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

    method = DTLS_server_method(); 
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match the certificate public key\n");
        exit(EXIT_FAILURE);
    }
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

    int sockfd;
    struct sockaddr_in addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};

    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;

    initialize_openssl();
    ctx = create_context();
    configure_context(ctx);

    // UDP 소켓 생성
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    printf("DTLS Echo server listening on port %d\n", port);

    while (1) {
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
        if (len < 0) {
            perror("recvfrom failed");
            continue;
        }

        printf("Received message from client, performing DTLS handshake...\n");

        bio = BIO_new_dgram(sockfd, BIO_NOCLOSE);
        SSL *ssl = SSL_new(ctx);
        SSL_set_bio(ssl, bio, bio);

        BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &client_addr);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            printf("SSL_accept failed\n");
            SSL_free(ssl);
            continue;
        }

        printf("DTLS handshake successful!\n");

        while (1) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
            if (bytes <= 0) {
                ERR_print_errors_fp(stderr);
                break;
            }

            buffer[bytes] = '\0';
            printf("Received message: %s\n", buffer);

            SSL_write(ssl, buffer, bytes);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
    }

    close(sockfd);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    return 0;
}