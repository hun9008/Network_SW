#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/ip.h>
#include <sys/time.h>

#define BUFSIZE 1500
#define ICMP_ECHOREPLY 0
#define ICMP_ECHOREQUEST 8
#define MAX_TTL 30    // Maximum TTL value to trace
#define TIMEOUT 1     // Timeout in seconds
#define PROBES 3

void err_quit(char *msg)
{
    perror(msg);
    exit(-1);
}

void err_display(char *msg)
{
    perror(msg);
}

typedef struct _ICMPMESSAGE {
    u_char icmp_type;
    u_char icmp_code;
    u_short icmp_cksum;
    u_short icmp_id;
    u_short icmp_seq;
} ICMPMESSAGE;

typedef struct iphdr {
    unsigned int ihl:4;
    unsigned int version:4;
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
} IPHEADER;

void DecodeICMPMessage(char *buf, int bytes, struct sockaddr_in *from);
u_short checksum(u_short *buffer, int size);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <IP Address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sk, ttl, retval;
    char *destIP = argv[1];
    ICMPMESSAGE icmpmsg;
    char buf[BUFSIZE + 1];
    struct sockaddr_in destaddr, peeraddr;
    socklen_t addrlen = sizeof(peeraddr);
    struct timeval timeout = {TIMEOUT, 0};  // Timeout setting
    struct timeval start_time, current_time;

    sk = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sk < 0) err_quit("socket()");

    // Set socket options for receive and send timeouts
    setsockopt(sk, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sk, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    memset(&destaddr, 0, sizeof(destaddr));
    destaddr.sin_family = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr(destIP);

    printf("Traceroute to %s\n", destIP);

    // Iterate through increasing TTL values
    for (ttl = 1; ttl <= MAX_TTL; ttl++) {
        printf("\n%2d  ", ttl);
        setsockopt(sk, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));  // Set TTL

        memset(&icmpmsg, 0, sizeof(icmpmsg));
        icmpmsg.icmp_type = ICMP_ECHOREQUEST;
        icmpmsg.icmp_code = 0;
        icmpmsg.icmp_id = (u_short)getpid();
        icmpmsg.icmp_seq = ttl;
        icmpmsg.icmp_cksum = checksum((u_short *)&icmpmsg, sizeof(icmpmsg));

        for (int i = 0; i < PROBES; i++)
        {
            gettimeofday(&start_time, NULL);

            if (sendto(sk, &icmpmsg, sizeof(icmpmsg), 0, (struct sockaddr *)&destaddr, sizeof(destaddr)) == -1) {
                perror("sendto()");
                continue;
            }

            // Receive response
            retval = recvfrom(sk, buf, BUFSIZE, 0, (struct sockaddr *)&peeraddr, &addrlen);
            if (retval < 0) {
                printf("\t*");  // Timeout or no response
                continue;
            }
            gettimeofday(&current_time, NULL);
            printf("\t%ld ms", (current_time.tv_sec - start_time.tv_sec) * 1000 + (current_time.tv_usec - start_time.tv_usec) / 1000);
        }
        // Decode and print ICMP response
        printf("\t%s", inet_ntoa(peeraddr.sin_addr));
        if (peeraddr.sin_addr.s_addr == destaddr.sin_addr.s_addr) {
            printf("\nReached destination!\n");
            break;
        }
    }

    close(sk);
    return 0;
}

// ICMP checksum calculation
u_short checksum(u_short *buf, int len) {
    u_long sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(u_char *)buf;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (u_short)(~sum);
}