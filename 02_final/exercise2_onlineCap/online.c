#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/ethernet.h>  
#include <netinet/ip.h>    
#include <netinet/tcp.h>   
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


void online_dump(u_char* dumpfile, const struct pcap_pkthdr* pkt_hdr, const u_char* pkt_data);
void print_statistics();

int main(void)
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

    printf("\n[Enter the file name(ex : capdump.cap)]: ");
    scanf("%s", file_name);

    adhandle = pcap_open_live(d->name, SNAPLEN, 1, 1000, errbuf);
    if (adhandle == NULL) {
        fprintf(stderr, "Unable to open the adapter: %s\n", d->name);
        pcap_freealldevs(alldevs);
        return -1;
    }

    printf("Capturing on %s...\n", d->description ? d->description : d->name);

    pcap_dumper_t* dumpfile = pcap_dump_open(adhandle, file_name);
    if (dumpfile == NULL) {
        fprintf(stderr, "Error opening dump file: %s\n", pcap_geterr(adhandle));
        return -1;
    }

    capture_start_time = (double)time(NULL);

    pcap_loop(adhandle, MAXPKT, online_dump, (u_char*)dumpfile);

    print_statistics();

    pcap_dump_close(dumpfile);
    pcap_freealldevs(alldevs);
    pcap_close(adhandle);

    // ====================================

	struct pcap_file_header	pcap_global_hdr;		
	struct pcap_pkthdr_packed pcap_pk_hdr;			
	unsigned char			pcap_pk_data[2000];		
	FILE*					fin;
	int						pk_no, res, offset=0;
	double					init_time, curr_time;	
	unsigned long			net_ip_count=0, net_etc_count=0;
	unsigned long			trans_tcp_count=0, trans_udp_count=0, trans_etc_count=0;

    fin = fopen(file_name, "rb");

	fread((char*)&pcap_global_hdr, sizeof(pcap_global_hdr), 1, fin);
	if (pcap_global_hdr.magic != 0xA1B2C3D4) {
		printf("파일 오류: 지원되지 않는 PCAP 파일 형식 (0x%x)\n", pcap_global_hdr.magic);
		exit(0);
	}

	pk_no = 0;
	int input = 0;
	int guard = 1;
	while (1) {

		if (guard == 1) {
			printf("[Enter Command (1: next, 0: show all, <num>: specific packet)] : ");
			scanf("%d", &input);
		}
		
		if (input == 0) {
			guard = 0;
		} if (input > 1) {
            pk_no = input - 1;
        }

		if (fread((char*)&pcap_pk_hdr, sizeof(pcap_pk_hdr), 1, fin) == 0)
			break;
		

		curr_time = pcap_pk_hdr.ts_sec + pcap_pk_hdr.ts_usec * 0.000001;
		if (pk_no == 0)
			init_time = curr_time;

		fread(pcap_pk_data, sizeof(unsigned char), pcap_pk_hdr.caplen, fin);

		offset = 0;
		res = parse_ethernet_header(&pcap_pk_data[offset], &eth_hdr);

		if (eth_hdr.eth_type == ETHERTYPE_IP) {

			offset += ETH_II_HSIZE;
			res = parse_ip_header(&pcap_pk_data[offset], &ip_hdr);
			net_ip_count++;

			offset += IP_HSIZE;
			if (ip_hdr.protocol == IP_PROTO_TCP) {
				res = parse_tcp_header(&pcap_pk_data[offset], &tcp_hdr);
				trans_tcp_count++;
			}
			else if (ip_hdr.protocol == IP_PROTO_UDP) {
				res = parse_udp_header(&pcap_pk_data[offset], &udp_hdr);
				trans_udp_count++;
			}
			else
				trans_etc_count++;
		}
		else
			net_etc_count++;

		pk_no++;

		printf("=====================================================================================================\n");

		printf("%d | %f | %d.%d.%d.%d | %d.%d.%d.%d | %d | %d | %s\n", pk_no, curr_time - init_time,
			(ip_hdr.saddr >> 24) & 0xFF, (ip_hdr.saddr >> 16) & 0xFF, (ip_hdr.saddr >> 8) & 0xFF, ip_hdr.saddr & 0xFF,
			(ip_hdr.daddr >> 24) & 0xFF, (ip_hdr.daddr >> 16) & 0xFF, (ip_hdr.daddr >> 8) & 0xFF, ip_hdr.daddr & 0xFF,
			ip_hdr.protocol, pcap_pk_hdr.caplen, "Info");

		printf("Internet Protocol Version %d, Src: %d.%d.%d.%d, Dst: %d.%d.%d.%d\n",
			ip_hdr.version, (ip_hdr.saddr >> 24) & 0xFF, (ip_hdr.saddr >> 16) & 0xFF, (ip_hdr.saddr >> 8) & 0xFF, ip_hdr.saddr & 0xFF,
			(ip_hdr.daddr >> 24) & 0xFF, (ip_hdr.daddr >> 16) & 0xFF, (ip_hdr.daddr >> 8) & 0xFF, ip_hdr.daddr & 0xFF);
		
        printf("\t%x .... = Version: %d\n", ip_hdr.version, ip_hdr.version); // decimal
		printf("\t.... %x = Header Length: %d bytes\n", ip_hdr.ihl, ip_hdr.ihl * 4); // decimal

		printf("Differentiated Services Field: 0x%02x (DSCP: 0x%02x, ECN: 0x%02x)\n", ip_hdr.tos, ip_hdr.tos >> 2, ip_hdr.tos & 0x03);
		printf("Total Length: %d\n", ip_hdr.tot_len);
		printf("Identification: 0x%04x\n", ip_hdr.id);

		printf("%3x. .... = Flags\n", ip_hdr.frag_off >> 13);
		printf("...%x .... = Fragment offset: %d\n", (ip_hdr.frag_off >> 8) & 0x1F, ip_hdr.frag_off & 0x1F);
		printf("Time to live: %d\n", ip_hdr.ttl);
		printf("Protocol: %d\n", ip_hdr.protocol);
		printf("Header checksum: 0x%04x\n", ip_hdr.checksum);
		printf("[Header checksum status: Unverified]\n");
		printf("Source: %d.%d.%d.%d\n", (ip_hdr.saddr >> 24) & 0xFF, (ip_hdr.saddr >> 16) & 0xFF, (ip_hdr.saddr >> 8) & 0xFF, ip_hdr.saddr & 0xFF);
		printf("Destination: %d.%d.%d.%d\n", (ip_hdr.daddr >> 24) & 0xFF, (ip_hdr.daddr >> 16) & 0xFF, (ip_hdr.daddr >> 8) & 0xFF, ip_hdr.daddr & 0xFF);
		printf("[Stream index: %d]\n", ip_hdr.saddr ^ ip_hdr.daddr);

		if (ip_hdr.protocol == IP_PROTO_TCP) {
			printf("Transmission Control Protocol, Src Port: %d, Dst Port: %d, Seq: %d, Ack: %d\n",
				tcp_hdr.src_port, tcp_hdr.dst_port, tcp_hdr.seq_num, tcp_hdr.ack_num);
			printf("Header Length: %d bytes\n", (tcp_hdr.hlen_flags >> 12) * 4);
			printf("Flags: 0x%04x\n", tcp_hdr.hlen_flags & 0x0FFF);
			printf("Window size value: %d\n", tcp_hdr.window);
			printf("Checksum: 0x%04x\n", tcp_hdr.checksum);
			printf("[Checksum status: Unverified]\n");
			printf("Urgent pointer: %d\n", tcp_hdr.urg_ptr);
			printf("[SEQ/ACK analysis]\n");
			printf("TCP segment data: %d bytes\n", pcap_pk_hdr.caplen - offset);
		}
		else if (ip_hdr.protocol == IP_PROTO_UDP) {
			printf("User Datagram Protocol, Src Port: %d, Dst Port: %d, Length: %d\n",
				udp_hdr.src_port, udp_hdr.dst_port, udp_hdr.length);
			printf("Checksum: 0x%04x\n", udp_hdr.checksum);
			printf("[Checksum status: Unverified]\n");
			printf("Data: %d bytes\n", pcap_pk_hdr.caplen - offset);
		}

	}
	fclose(fin);

    return 0;
}

void online_dump(u_char* dumpfile, const struct pcap_pkthdr* pkt_hdr, const u_char* pkt_data)
{
    struct tm* ltime;
    char timestr[64];
    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    char info[128] = "No additional info";

    total_packets++;

    const struct ether_header* ethernet = (struct ether_header*)pkt_data;
    if (ntohs(ethernet->ether_type) != ETHERTYPE_IP) { 
        return;
    }

    ip_count++;

    time_t local_tv_sec = pkt_hdr->ts.tv_sec;
    ltime = localtime(&local_tv_sec);
    strftime(timestr, sizeof(timestr), "%H:%M:%S", ltime);

    const struct ip* ip_header = (struct ip*)(pkt_data + sizeof(struct ether_header));
    inet_ntop(AF_INET, &(ip_header->ip_src), src_ip, sizeof(src_ip));
    inet_ntop(AF_INET, &(ip_header->ip_dst), dst_ip, sizeof(dst_ip));

    char* protocol = "";
    switch (ip_header->ip_p) {
        case IPPROTO_TCP: {
            protocol = "TCP";
            tcp_count++; 
            const struct tcphdr* tcp_header = (struct tcphdr*)(pkt_data + sizeof(struct ether_header) + (ip_header->ip_hl * 4));
            snprintf(info, sizeof(info), "Src Port: %d, Dst Port: %d, Seq: %u",
                     ntohs(tcp_header->th_sport), ntohs(tcp_header->th_dport), ntohl(tcp_header->th_seq));
            break;
        }
        case IPPROTO_UDP:
            protocol = "UDP";
            udp_count++; 
            break;
        case IPPROTO_ICMP:
            protocol = "ICMP";
            break;
        default:
            protocol = "Other";
            break;
    }

    printf(" %4d %s %-16s %-16s %-8s %4d %s\n",
           ++tot_cap_num, timestr, src_ip, dst_ip, protocol, pkt_hdr->len, info);

    pcap_dump(dumpfile, pkt_hdr, pkt_data);

    if (tot_cap_num >= MAXPKT) {
        printf("Captured %d packets. Stopping capture...\n", tot_cap_num);
        pcap_breakloop(adhandle);
    }
}

void print_statistics()
{
    double capture_end_time = (double)time(NULL);
    double duration = capture_end_time - capture_start_time;

    double ip_ratio = (double)ip_count / total_packets * 100.0;
    double tcp_ratio = (ip_count > 0) ? (double)tcp_count / ip_count * 100.0 : 0.0;
    double udp_ratio = (ip_count > 0) ? (double)udp_count / ip_count * 100.0 : 0.0;
    double bit_rate = (double)(tot_cap_num * SNAPLEN * 8) / duration;
    double packet_rate = (double)tot_cap_num / duration;

    printf("\n=== Capture Statistics ===\n");
    printf("Total packets captured: %d\n", total_packets);
    printf("IP packet ratio: %.2f%%\n", ip_ratio);
    printf("TCP packet ratio: %.2f%%\n", tcp_ratio);
    printf("UDP packet ratio: %.2f%%\n", udp_ratio);
    printf("Bit rate: %.2f bits/sec\n", bit_rate);
    printf("Packet rate: %.2f packets/sec\n", packet_rate);
    printf("==========================\n");
}