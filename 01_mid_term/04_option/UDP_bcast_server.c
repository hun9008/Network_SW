#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 512

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

    if(argc != 3) {
        printf("Parameter Error\n");
        return -1;
    } else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

    int retval;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == -1) err_quit("socket()");

    struct sockaddr_in bcastAddr;
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bcastAddr.sin_port = htons(port);

    retval = bind(sock, (struct sockaddr *)&bcastAddr, sizeof(bcastAddr));
    if(retval == -1) err_quit("bind()");
    printf("\n[INFO] BIND SUCCESS\n");

    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];

    while(1) {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
        if(retval == -1) {
            err_display("recvfrom()");
            continue;
        }

        buf[retval] = '\0';
        printf("[UDP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);
    }

    close(sock);

    return 0;
}