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

    int z, s;
    struct sockaddr_in mAddr;
    char mMsg[256];
    unsigned char mTTL = 1;
    unsigned int mcastLen;

    s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    z = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&mTTL, sizeof(mTTL));

    memset(&mAddr, 0, sizeof(mAddr));
    mAddr.sin_family = AF_INET;
    mAddr.sin_addr.s_addr = inet_addr("224.10.0.3");
    mAddr.sin_port = htons(9099);

    int count = 0;
    while(1) {
        sprintf(mMsg, "Multicast Message %d", count++);
        mcastLen = strlen(mMsg);
        z = sendto(s, mMsg, mcastLen, 0, (struct sockaddr *)&mAddr, sizeof(mAddr));
        if(z == -1) err_quit("sendto()");
        printf("[UDP/%s:%d] %s\n", inet_ntoa(mAddr.sin_addr), ntohs(mAddr.sin_port), mMsg);
        sleep(1);
    }

    close(s);

    return 0;
}