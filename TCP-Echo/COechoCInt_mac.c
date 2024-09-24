// COechoClnt.c
// TCP echo client for macOS/Linux
//
// �����: ��Ʈ��ũ����Ʈ�����
// ���ִ��б� ����Ʈ�����а�
// ����� ��Ƽ�̵�� ��� ��Ʈ��ũ ������ (mmcn.ajou.ac.kr)
//

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>     // for close()
#include <stdlib.h>
#include <stdio.h>
#include <string.h>     // for memset()

#define BUFSIZE 1500

// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
    printf("Error [%s] ... program terminated \n", msg);
    exit(-1);
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
    printf("socket function error [%s]\n", msg);
}

int main(int argc, char* argv[])
{
    int retval;
    int sock;                       
    struct sockaddr_in serveraddr;   
    char buf[BUFSIZE + 1];
    int len;

    // socket()
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");

    // server address
    memset(&serveraddr, 0, sizeof(serveraddr)); 
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9000);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // connect()
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));  
    if (retval == -1) err_quit("connect()");

    while (1) {

        memset(buf, 0, sizeof(buf));  
        printf("\n[���� ������] ");
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
        printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� ���½��ϴ�.\n", retval);

        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == -1) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;

        buf[retval] = '\0';
        printf("[TCP Ŭ���̾�Ʈ] %d����Ʈ�� �޾ҽ��ϴ�.\n", retval);
        printf("[���� ������] %s\n", buf);
    }

    close(sock);

    return 0;
}