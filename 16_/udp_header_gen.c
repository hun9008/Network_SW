#include <stdlib.h>
#include <stdio.h>

#include <pcap.h>
#include <time.h>

#define SNAPLEN 68     
#define MAXPKT 1000     

#pragma warning(disable:4996)

#define LINE_LEN 16

#define ETHERTYPE_IP		0x0800
#define ETH_II_HSIZE		14		
#define IP_HSIZE			20		
#define IP_PROTO_TCP		6		
#define IP_PROTO_UDP		17	
#define ORIG_PACKET_LEN 64

#define pntoh16(p)  ((unsigned short)                       \
                    ((unsigned short)*((unsigned char *)(p)+0)<<8|  \
                     (unsigned short)*((unsigned char *)(p)+1)<<0))

#define pntoh32(p)  ((unsigned short)*((unsigned char *)(p)+0)<<24|  \
                    (unsigned short)*((unsigned char *)(p)+1)<<16|  \
                    (unsigned short)*((unsigned char *)(p)+2)<<8|   \
                    (unsigned short)*((unsigned char *)(p)+3)<<0)

typedef int BOOL;

#define TRUE 1
#define FALSE 0

pcap_t* adhandle;
int tot_cap_num = 0;       
int total_packets = 0;   
int ip_count = 0;         
int tcp_count = 0;       
int udp_count = 0;        
double capture_start_time; 

struct ethernet_header {
	uint8_t dest_mac[6];   
	uint8_t src_mac[6];    
	uint16_t eth_type;     
};

struct ip_header {
	uint8_t  ihl : 4;      // IP 헤더 길이
	uint8_t  version : 4;  // IP 버전
	uint8_t  tos;          // 서비스 타입
	uint16_t tot_len;      // 전체 길이
	uint16_t id;           // 식별자
	uint16_t frag_off;     // 플래그 + 오프셋
	uint8_t  ttl;          // TTL
	uint8_t  protocol;     // 프로토콜 (TCP, UDP 등)
	uint16_t checksum;     // 체크섬
	uint32_t saddr;        // 소스 IP 주소
	uint32_t daddr;        // 목적지 IP 주소
};

struct tcp_header {
	uint16_t src_port;       // 소스 포트 번호
	uint16_t dst_port;       // 목적지 포트 번호
	uint32_t seq_num;        // 시퀀스 번호
	uint32_t ack_num;        // 응답 번호
	uint16_t hlen_flags;     // 헤더길이 (4비트) + Unused (6비트) + 플래그(6비트)
	uint16_t window;         // 윈도우 크기
	uint16_t checksum;       // 체크섬
	uint16_t urg_ptr;        // 긴급포인터
};

struct udp_header {
	uint16_t src_port;       // 소스 포트 번호
	uint16_t dst_port;       // 목적지 포트 번호
	uint16_t length;         // UDP 데이터그램 크기
	uint16_t checksum;       // 체크섬
};

struct ethernet_header	eth_hdr;
struct ip_header		ip_hdr;
struct tcp_header		tcp_hdr;
struct udp_header		udp_hdr;

int parse_ip_header(unsigned char* data, struct ip_header *ip_hdr)
{
	ip_hdr->version		= data[0] >> 4;		// IP version
	ip_hdr->ihl			= data[0] & 0x0f;	// IP header length
	ip_hdr->protocol	= data[9];		    // protocol above IP

	ip_hdr->tot_len		= pntoh16(&data[2]);
	ip_hdr->id			= pntoh16(&data[4]);
	ip_hdr->frag_off	= pntoh16(&data[6]);
	ip_hdr->ttl			= data[8];
	ip_hdr->checksum	= pntoh16(&data[10]);
	ip_hdr->saddr		= pntoh32(&data[12]);
	ip_hdr->daddr		= pntoh32(&data[16]);

	return 0;
}

int parse_ethernet_header(unsigned char* data, struct ethernet_header *eth_hdr)
{
	int i;

	for (i = 0; i < 6; i++) {
		eth_hdr->dest_mac[i] = data[i];
	}

	for (i = 0; i < 6; i++) {
		eth_hdr->src_mac[i] = data[i+6];
	}

	eth_hdr->eth_type = pntoh16(&data[12]);

	return 0;
}

int parse_tcp_header(unsigned char* data, struct tcp_header* tcp_hdr)
{
	tcp_hdr->src_port	= pntoh16(&data[0]);
	tcp_hdr->dst_port	= pntoh16(&data[2]);
	tcp_hdr->seq_num	= pntoh32(&data[4]);
	tcp_hdr->ack_num	= pntoh32(&data[8]);
	tcp_hdr->hlen_flags = pntoh16(&data[12]);
	tcp_hdr->window		= pntoh16(&data[14]);
	tcp_hdr->checksum	= pntoh16(&data[16]);
	tcp_hdr->urg_ptr	= pntoh16(&data[18]);

	return 0;
}

int parse_udp_header(unsigned char* data, struct udp_header* udp_hdr)
{
	udp_hdr->src_port	= pntoh16(&data[0]);
	udp_hdr->dst_port	= pntoh16(&data[2]);
	udp_hdr->length		= pntoh16(&data[4]);
	udp_hdr->checksum	= pntoh16(&data[6]);

	return 0;
}

struct pcap_pkthdr_packed {
    unsigned int ts_sec;
    unsigned int ts_usec;
    unsigned int caplen;
    unsigned int len;
} __attribute__((packed));

/* case-insensitive string comparison that may mix up special characters and numbers */
int close_enough(char *one, char *two)
{
	while (*one && *two)
	{
		if ( *one != *two && !(
			(*one >= 'a' && *one - *two == 0x20) ||
			(*two >= 'a' && *two - *one == 0x20)
			))
		{
			return 0;
		}
		one++;
		two++;
	}
	if (*one || *two)
	{
		return 0;
	}
	return 1;
}

uint16_t udp_sum_calc (uint16_t len_udp, uint16_t src_addr[], uint16_t dest_addr[], BOOL padding, uint16_t buff[])
{
    uint16_t prot_udp = 17;
    uint16_t padd = 0;
    uint16_t word16;
    uint32_t sum;

    if (padding == TRUE) {
        padd = 1;
        buff[len_udp] = 0;
    }

    sum = 0;

    for (int i = 0; i < len_udp + padd; i += 2) {
        word16 = ((buff[i] << 8) & 0xFF00) + (buff[i + 1] & 0xFF);
        sum += (unsigned long)word16;
    }

    for (int i = 0; i < 4; i += 2) {
        word16 = ((src_addr[i] << 8) & 0xFF00) + (src_addr[i + 1] & 0xFF);
        sum += word16;
    }

    sum = sum + prot_udp + len_udp;

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    sum = ~sum;

    return ((uint16_t)sum);
}

#define ORIG_PACKET_LEN 64
int main(int argc, char **argv)
{
    char sendbuf[4096];
    struct ethernet_header *eh;
    struct ip_header *iph;
    struct udp_header *udph;
    char *udata;
    int pk_len;


    pcap_if_t* alldevs;
    pcap_if_t* d;
    char errbuf[PCAP_ERRBUF_SIZE];
    int ndNum = 0; 
    int devNum;    
    char file_name[256];

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

    for (d = alldevs; d; d = d->next) {
        printf("[%d] %s (%s)\n", ++ndNum, d->name, d->description ? d->description : "No description available");
    }

    if (ndNum == 0) {
        printf("No interfaces found! Make sure Npcap is installed.\n");
        return -1;
    }

    printf("Enter the interface number (1-%d): ", ndNum);
    scanf("%d", &devNum);

    if (devNum < 1 || devNum > ndNum) {
        printf("Interface number out of range.\n");
        pcap_freealldevs(alldevs);
        return -1;
    }

	for (d = alldevs, ndNum = 1; ndNum < devNum; d = d->next, ndNum++);

	printf("select interface : %s\n", d->name);

    adhandle = pcap_open_live(d->name, SNAPLEN, 1, 1000, errbuf);

    eh = (struct ethernet_header *)sendbuf;
    iph = (struct ip_header *)(sendbuf + sizeof(struct ethernet_header));
    udph = (struct udp_header *)(sendbuf + sizeof(struct ethernet_header) + sizeof(struct ip_header));
    udata = (char *) (sendbuf + sizeof(struct ethernet_header) + sizeof(struct ip_header) + sizeof(struct udp_header));

    eh->dest_mac[0] = 0xff;
    eh->dest_mac[1] = 0xff;
    eh->dest_mac[2] = 0xff;
    eh->dest_mac[3] = 0xff;
    eh->dest_mac[4] = 0xff;
    eh->dest_mac[5] = 0xff;
    
    eh->src_mac[0] = 0x00;
    eh->src_mac[1] = 0x0c;
    eh->src_mac[2] = 0x29;
    eh->src_mac[3] = 0x2e;
    eh->src_mac[4] = 0x8e;
    eh->src_mac[5] = 0x3b;

    eh->eth_type = htons(0x0800);

    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->id = htons(9999);
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = 17;
    iph->saddr = inet_addr("");
    
    udph->src_port = htons(12345);
    udph->dst_port = htons(12345);

    udata[0] = 0xde;
    udata[1] = 0xad;
    udata[2] = 0xbe;
    udata[3] = 0xef;

    uint16_t src_addr[2];
    src_addr[0] = iph->saddr >> 16;
    src_addr[1] = iph->saddr & 0xFFFF;

    uint16_t dest_addr[2];
    dest_addr[0] = iph->daddr >> 16;
    dest_addr[1] = iph->daddr & 0xFFFF;

    iph->tot_len = htons(sizeof(struct ip_header) + sizeof(struct udp_header) + 4);
    iph->checksum = udp_sum_calc(sizeof(struct udp_header) + 4, src_addr, dest_addr, TRUE, (uint16_t *)udph);
    udph->length = htons(sizeof(struct udp_header) + 4);
    udph->checksum = udp_sum_calc(sizeof(struct udp_header) + 4, src_addr, dest_addr, TRUE, (uint16_t *)udph);

    pk_len = sizeof(struct ethernet_header) + sizeof(struct ip_header) + sizeof(struct udp_header) + 4;
    pcap_inject(adhandle, sendbuf, pk_len);

    pcap_close(adhandle);

    return 0;
}
