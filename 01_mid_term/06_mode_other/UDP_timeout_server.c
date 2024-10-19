#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BUFSIZE 512

void err_quit(char *msg) {
    perror(msg);
    exit(-1);
}

void err_display(char *msg) {
    perror(msg);
}

int main(int argc, char* argv[]) {

    time_t start_time, current_time;

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

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1) err_quit("socket()");

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        err_quit("setsockopt() failed for SO_RCVTIMEO");
    } else {
        printf("\n[INFO] SETSOCKOPT SUCCESS\n");
    }
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        err_quit("setsockopt() failed for SO_SNDTIMEO");
    } else {
        printf("\n[INFO] SETSOCKOPT SUCCESS\n");
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(retval == -1) err_quit("bind()");
    printf("\n[INFO] BIND SUCCESS\n");

    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];

    while(1) {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
        if(retval == -1) {
            // timeout 일 경우
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("\n[INFO] TIMEOUT\n");
                timeout.tv_sec *= 2;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                continue;
            } else {
                err_display("recvfrom()");
                continue;
            }
        }

        buf[retval] = '\0';
        printf("[UDP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);

        retval = sendto(sock, buf, retval, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
        if(retval == -1) {
            err_display("sendto()");
            continue;
        }
    }

    close(sock);

    return 0;
}