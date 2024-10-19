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
    struct sockaddr_in mcastAddr;
    char mcastMsg[4096];
    struct ip_mreq mRequest;

    s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&mcastAddr, 0, sizeof(mcastAddr)); 
    mcastAddr.sin_family = AF_INET;
    mcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mcastAddr.sin_port = htons(9099);

    z = bind(s, (struct sockaddr *)&mcastAddr, sizeof(mcastAddr));

    mRequest.imr_multiaddr.s_addr = inet_addr("224.10.0.3");
    mRequest.imr_interface.s_addr = htonl(INADDR_ANY);

    z = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mRequest, sizeof(mRequest));

    while(1) {
        socklen_t addrlen = sizeof(mcastAddr);  // socklen_t 변수를 선언
        z = recvfrom(s, mcastMsg, sizeof(mcastMsg), 0, 
                    (struct sockaddr *)&mcastAddr, &addrlen);  // &addrlen 전달
        mcastMsg[z] = '\0';
        printf("[UDP/%s:%d] %s\n", inet_ntoa(mcastAddr.sin_addr), ntohs(mcastAddr.sin_port), mcastMsg);

    }

    z = setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *)&mRequest, sizeof(mRequest));
    close(s);

    return 0;
}