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

#define pntoh16(p)  ((unsigned short)                       \
                    ((unsigned short)*((unsigned char *)(p)+0)<<8|  \
                     (unsigned short)*((unsigned char *)(p)+1)<<0))

#define pntoh32(p)  ((unsigned short)*((unsigned char *)(p)+0)<<24|  \
                    (unsigned short)*((unsigned char *)(p)+1)<<16|  \
                    (unsigned short)*((unsigned char *)(p)+2)<<8|   \
                    (unsigned short)*((unsigned char *)(p)+3)<<0)

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

#define ORIG_PACKET_LEN 64
int main(int argc, char **argv)
{
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

	pcap_t *fp;
	// errbuf[PCAP_ERRBUF_SIZE] = {0};
	u_char packet[ORIG_PACKET_LEN] = 
		/* Ethernet frame header */
		"\xff\xff\xff\xff\xff\xfa" /* dst mac */
		"\x02\x02\x02\x02\x02\x02" /* src mac */
		"\x08\x00" /* ethertype IPv4 */
		/* IPv4 packet header */
		"\x45\x00\x00\x00" /* IPv4, minimal header, length TBD */
		"\x12\x34\x00\x00" /* IPID 0x1234, no fragmentation */
		"\x10\x11\x00\x00" /* TTL 0x10, UDP, checksum (not required) */
		"\x00\x00\x00\x00" /* src IP (TBD) */
		"\xff\xff\xff\xff" /* dst IP (broadcast) */
		/* UDP header */
		"\x00\x07\x00\x07" /* src port 7, dst port 7 (echo) */
		"\x00\x00\x00\x00" /* length TBD, cksum 0 (unset) */
	;
	u_char *sendme = packet;
	size_t packet_len = ORIG_PACKET_LEN;
	pcap_if_t *ifaces = NULL;
	pcap_if_t *dev = NULL;
	pcap_addr_t *addr = NULL;

	/* Check the validity of the command line */
	if (argc != 2)
	{
		printf("usage: %s interface", argv[0]);
		return 1;
	}
    
	if (0 != pcap_init(PCAP_CHAR_ENC_LOCAL, errbuf)) {
		fprintf(stderr, "Failed to initialize pcap lib: %s\n", errbuf);
		return 2;
	}

	/* Find the IPv4 address of the device */
	if (0 != pcap_findalldevs(&ifaces, errbuf)) {
		fprintf(stderr, "Failed to get list of devices: %s\n", errbuf);
		return 2;
	}

	for (dev = ifaces; dev != NULL; dev = dev->next)
	{
		if (close_enough(dev->name, argv[1]))
		{
			break;
		}
	}
	if (dev == NULL) {
		fprintf(stderr, "Could not find %s in the list of devices\n", argv[1]);
		return 3;
	}

	for (addr = dev->addresses; addr != NULL; addr = addr->next)
	{
		if (addr->addr->sa_family == AF_INET)
		{
			break;
		}
	}
	if (addr == NULL) {
		fprintf(stderr, "Could not find IPv4 address for %s\n", argv[1]);
		return 3;
	}

	/* Fill in the length and source addr and calculate checksum */
	packet[14 + 2] = 0xff & ((ORIG_PACKET_LEN - 14) >> 8);
	packet[14 + 3] = 0xff & (ORIG_PACKET_LEN - 14);
	/* UDP length */
	packet[14 + 20 + 4] = 0xff & ((ORIG_PACKET_LEN - 14 - 20) >> 8);
	packet[14 + 20 + 5] = 0xff & (ORIG_PACKET_LEN - 14 - 20);
#ifdef _WIN32
	*(u_long *)(packet + 14 + 12) = ((struct sockaddr_in *)(addr->addr))->sin_addr.S_un.S_addr;
#else
	*(u_long *)(packet + 14 + 12) = ((struct sockaddr_in *)(addr->addr))->sin_addr.s_addr;;	
#endif
	uint32_t cksum = 0;
	for (int i=14; i < 14 + 4 * (packet[14] & 0xf); i += 2)
	{
		cksum += *(uint16_t *)(packet + i);
	}
	while (cksum>>16)
		cksum = (cksum & 0xffff) + (cksum >> 16);
	cksum = ~cksum;
	*(uint16_t *)(packet + 14 + 10) = cksum;

	/* Open the adapter */
	if ((fp = pcap_open_live(d->name,		// name of the device
							 0, // portion of the packet to capture. 0 == no capture.
							 0, // non-promiscuous mode
							 1000,			// read timeout
							 errbuf			// error buffer
							 )) == NULL)
	{
		fprintf(stderr,"\nUnable to open the adapter. %s is not supported by Npcap\n", argv[1]);
		return 2;
	}
	
	switch(pcap_datalink(fp))
	{
		case DLT_NULL:
			/* Skip Ethernet header, retreat NULL header length */
#define NULL_VS_ETH_DIFF (14 - 4)
			sendme = packet + NULL_VS_ETH_DIFF;
			packet_len -= NULL_VS_ETH_DIFF;
			// Pretend IPv4
			sendme[0] = 2;
			sendme[1] = 0;
			sendme[2] = 0;
			sendme[3] = 0;
			break;
		case DLT_EN10MB:
			/* Already set up */
			sendme = packet;
			break;
		default:
			fprintf(stderr, "\nError, unknown data-link type %u\n", pcap_datalink(fp));
			return 4;
	}
	
	/* Send down the packet */
	if (pcap_sendpacket(fp,	// Adapter
		sendme, // buffer with the packet
		packet_len // size
		) != 0)
	{
		fprintf(stderr,"\nError sending the packet: %s\n", pcap_geterr(fp));
		return 3;
	}

	pcap_close(fp);	
	return 0;
}
