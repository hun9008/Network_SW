#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>     
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 1500

void err_quit(char *msg)
{
    perror(msg);
    exit(-1);
}

void err_display(char *msg)
{
    perror(msg);
}

int main(int argc, char* argv[])
{
    int listen_sock;
    int client_sock;
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE+1];
    int retval, msglen;

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock == -1) err_quit("socket()");
    
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9000);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(retval == -1) err_quit("bind()");
    
    retval = listen(listen_sock, SOMAXCONN);
    if(retval == -1) err_quit("listen()");

    while(1){
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        if(client_sock == -1) {
            err_display("accept()");
            continue;
        }

        printf("\n[TCP Server] Client accepted : IP addr=%s, port=%d\n", 
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        while(1){
            msglen = recv(client_sock, buf, BUFSIZE, 0);
            if(msglen == -1){
                err_display("recv()");
                break;
            }
            else if(msglen == 0)
                break;

            buf[msglen] = '\0';
            printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
                ntohs(clientaddr.sin_port), buf);

            retval = send(client_sock, buf, msglen, 0);
            if(retval == -1){
                err_display("send()");
                break;
            }
        }

        close(client_sock);
        printf("[TCP Server] Client disconnected: IP addr=%s, port=%d\n", 
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    }

    close(listen_sock);

    return 0;
}