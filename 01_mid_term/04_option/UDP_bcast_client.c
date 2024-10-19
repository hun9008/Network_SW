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

    char *bcastIP = "255.255.255.255";
    char *bcastMsg = "This is broadcast message !!";
    int so_broadcast = 1;
    int z, msglen;

    int retval;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) err_quit("socket()");

    z = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof so_broadcast);

    struct sockaddr_in bcastAddr;

    memset(&bcastAddr, 0, sizeof(bcastAddr));
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_addr.s_addr = inet_addr(bcastIP);
    bcastAddr.sin_port = htons(port);

    msglen = strlen(bcastMsg);

    struct sockaddr_in peeraddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];
    int len;

    while (1) {
        printf("\n[Input Message] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        retval = sendto(sock, buf, strlen(buf), 0,
            (struct sockaddr *)&bcastAddr, sizeof(bcastAddr));
        if (retval == -1) {
            err_display("sendto()");
            continue;
        }
        printf("[UDP Client] Sent %d bytes.\n", retval);

    }

    close(sock);
    return 0;
}