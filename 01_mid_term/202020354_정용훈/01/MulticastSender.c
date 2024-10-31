#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define BUFSIZE 1500

typedef struct {
    char channel_number[3];
    char contents_title[BUFSIZE];
    struct sockaddr_in addr;
} Channel_Info;

Channel_Info channel_info[3];

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void err_display(const char *msg) {
    perror(msg);
}

// 메시지 송신을 담당하는 함수
void *Thread_m1(void *arg) {

    while(1) {
        int sock = *(int *)arg;
        char buf[BUFSIZE + 1];
        char temp[BUFSIZE + 1];
        int retval;

        char idx = '1';
        strcpy(buf, &idx);
        strcat(buf, channel_info[0].channel_number);
        strcat(buf, inet_ntoa(channel_info[0].addr.sin_addr));
        strcat(buf, "10100");
        strcat(buf, channel_info[0].contents_title);

        printf("\n[DEBUG] INFO_REG : %s", buf);
        
        retval = send(sock, buf, BUFSIZE, 0);
        if (retval == -1) {
            err_display("send()");
            exit(1);
        }
        printf("\n[TCP client] Sent %d bytes.\n", retval);


        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval <= 0) {
            err_display("recv()");
            exit(1);
        }
            
        buf[retval] = '\0';
        printf("\n[REG_RES] %s", buf);

        while(1) {
            printf("\n Would you change contents title? : ");
            if (fgets(temp, BUFSIZE + 1, stdin) == NULL)
                break;
            
            if (temp[0] == 'Y')
            {
                printf("\n[Enter the Title] ");
                if (fgets(temp, BUFSIZE + 1, stdin) == NULL)
                    break;
                strcpy(channel_info[0].contents_title, "");
                strcpy(channel_info[0].contents_title, temp);
                break;
            } else {
                continue;
            }
        }
    }

    return NULL;
}

void set_channel_info() 
{
    strcpy(channel_info[0].channel_number,"01");
    strcpy(channel_info[0].contents_title, "Star Wars");
    channel_info[0].addr.sin_addr.s_addr = inet_addr("224.10.0.110");
    channel_info[0].addr.sin_port = htons(10100);

    strcpy(channel_info[1].channel_number,"02");
    strcpy(channel_info[1].contents_title, "Terminator 3");
    channel_info[1].addr.sin_addr.s_addr =inet_addr("224.10.0.120");
    channel_info[1].addr.sin_port = htons(10200);

    strcpy(channel_info[2].channel_number,"03");
    strcpy(channel_info[2].contents_title, "Frozen");
    channel_info[2].addr.sin_addr.s_addr = inet_addr("224.10.0.130");
    channel_info[2].addr.sin_port = htons(10300);
}

void get_channel_info()
{
    for(int i = 0; i < 3; i++)
    {
        printf("%s %d %s %s\n", inet_ntoa(channel_info[i].addr.sin_addr),
        ntohs(channel_info[i].addr.sin_port), channel_info[i].channel_number,
        channel_info[i].contents_title);
    }
}

int main(int argc, char *argv[]) {
    char ip[20] = "";
    int port;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    } else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

    set_channel_info();
    get_channel_info();

    int sock;
    struct sockaddr_in serveraddr;
    int retval;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == -1) err_quit("connect()");

    printf("[TCP Client] Connected to the server.\n");

    pthread_t sendThread;

    if (pthread_create(&sendThread, NULL, Thread_m1, &sock) != 0)
        err_quit("pthread_create() for send");

    pthread_join(sendThread, NULL);
    close(sock);

    return 0;
}