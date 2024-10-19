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

typedef struct stats {
    int packets_sent;
    int packets_recv;
    int bytes_sent;
    int bytes_recv;
    int interval_time[100];
    int interval_cnt;
} STATS;

void DecodeICMPMessage(char *buf, int bytes, struct sockaddr_in *from);

u_short checksum(u_short *buffer, int size);

void StatsPrint(STATS *stats);

int main(int argc, char* argv[])
{
    int sk, retval;
    char *destIP;
    ICMPMESSAGE icmpmsg;
    char buf[BUFSIZE + 1];
    struct sockaddr_in destaddr, peeraddr;
    socklen_t addrlen, optlen;
    struct timeval timeo = {1, 0};
    STATS stats;
    memset(&stats, 0, sizeof(stats));
    struct timeval start_time, current_time;

    destIP = argv[1];

    sk = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    setsockopt(sk, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
    setsockopt(sk, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));

    memset(&destaddr, 0, sizeof(destaddr));
    destaddr.sin_family = AF_INET;
    destaddr.sin_addr.s_addr = inet_addr(destIP);


    for(int i = 0; i < 4; i++)
    {
        memset(&icmpmsg, 0, sizeof(icmpmsg));
        icmpmsg.icmp_type = ICMP_ECHOREQUEST;
        icmpmsg.icmp_code = 0;
        icmpmsg.icmp_id = (u_short)getpid();
        icmpmsg.icmp_seq = i;
        icmpmsg.icmp_cksum = checksum((u_short *)&icmpmsg, sizeof(icmpmsg));

        gettimeofday(&start_time, NULL);
        if (sendto(sk, (char *)&icmpmsg, sizeof(icmpmsg), 0, (struct sockaddr *)&destaddr, sizeof(destaddr)) == -1)
        {
            if (errno == EWOULDBLOCK)
            {
                printf("Send Timeout\n");
                continue;
            }
            printf("sendto() error\n");
            break;
        } else {
            stats.packets_sent++;
            stats.bytes_sent += sizeof(icmpmsg);
        }
        
        addrlen = sizeof(peeraddr);
        retval = recvfrom(sk, buf, BUFSIZE, 0, (struct sockaddr *)&peeraddr, &addrlen);
        if (retval < 0) {
            if (errno == EWOULDBLOCK)
            {
                printf("Recv Timeout\n");
                continue;
            }
            printf("recvfrom() error\n");
            break;
        } else {
            stats.packets_recv++;
            stats.bytes_recv += retval;

            gettimeofday(&current_time, NULL);
            long elapsed_time = (current_time.tv_sec - start_time.tv_sec) * 1000
                + (current_time.tv_usec - start_time.tv_usec) / 1000;
            // printf("Elapsed Time: %ld ms\n", elapsed_time);
            stats.interval_time[stats.interval_cnt++] = elapsed_time;
        }

        DecodeICMPMessage(buf, retval, &peeraddr);

        sleep(1);

    }

    StatsPrint(&stats);

    close(sk);
    return 0;

}

void DecodeICMPMessage (char *buf, int len, struct sockaddr_in *from)
{
    IPHEADER *iphdr = (IPHEADER *)buf;
    int iphdrlen = iphdr->ihl << 2;
    ICMPMESSAGE *icmpmsg = (ICMPMESSAGE *)(buf + iphdrlen);

    if ( (len - iphdrlen) < 8)
    {
        printf("[error] ICMP packet is too short!\n");
        return;
    }

    if (icmpmsg->icmp_id != (u_short)getpid())
    {
        printf("[error] Not for our echo request!\n");
        return;
    }

    if (icmpmsg->icmp_type != ICMP_ECHOREPLY)
    {
        printf("[error] Not an echo packet!\n");
        return;
    }

    printf("Reply from %s: total bytes = %d, seq = %d\n", inet_ntoa(from->sin_addr), len, icmpmsg->icmp_seq);
    return;

}

u_short checksum(u_short *buf, int len)
{
    u_long cksum = 0;
    ushort *ptr = buf;
    int left = len;

    while(left > 1)
    {
        cksum += *ptr++;
        left -= sizeof(u_short);
    }

    if(left == 1)
    {
        cksum += *(u_char *)ptr;
    }

    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);

    return (u_short)(~cksum);
}

void StatsPrint(STATS *stats)
{
    int i;
    int min_interval = 1000;
    int max_interval = 0;
    int avg_interval = 0;

    printf("\n\t==================== ==================== ====================\n\n");
    printf("\tSent Packets : %d \t Recv Packets : %d \t Loss Packets : %d\n", stats->packets_sent, stats->packets_recv, stats->packets_sent - stats->packets_recv);
    printf("\t\t\t\tLoss Rate : %.2f%%\n", (float)(stats->packets_sent - stats->packets_recv) / stats->packets_sent * 100);
    printf("\tSent Bytes : %d \t Recv Bytes : %d\n", stats->bytes_sent, stats->bytes_recv);
    printf("\n\t==================== ==================== ====================\n\n");
    
    for (int i = 0; i < stats->interval_cnt; i++)
    {
        if (stats->interval_time[i] < min_interval)
        {
            min_interval = stats->interval_time[i];
        }
        if (stats->interval_time[i] > max_interval)
        {
            max_interval = stats->interval_time[i];
        }
        avg_interval += stats->interval_time[i];
    }
    avg_interval /= stats->interval_cnt;

    printf("\tMin Interval : %d ms\t", min_interval);
    printf("Max Interval : %d ms\t", max_interval);
    printf("Avg Interval : %d ms\n", avg_interval);
    printf("\n\t==================== ==================== ====================\n\n");


}