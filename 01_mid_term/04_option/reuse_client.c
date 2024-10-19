#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 1500

void err_quit(char *msg) {
    perror(msg);
    exit(-1);
}

void err_display(char *msg) {
    perror(msg);
}

int main(int argc, char* argv[]) {
    int syntax;
    char ip[20] = "";
    int port;

    if (argc != 3) {
        printf("Parameter Error\n");
        return -1;
    } else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

    int retval;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];
    int len;

    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == -1) err_quit("connect()");

    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("\n[Input Message] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        retval = send(sock, buf, strlen(buf), 0);
        if (retval == -1) {
            err_display("send()");
            break;
        }
        printf("[TCP client] Sent %d bytes.\n", retval);

        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == -1) {
            err_display("recv()");
            break;
        } else if (retval == 0)
            break;

        buf[retval] = '\0';
        printf("[TCP client] Sent %d bytes.\n", retval);
        printf("[Receive message] %s\n", buf);
    }

    close(sock);
    return 0;
}