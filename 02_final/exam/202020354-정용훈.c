
#ifdef _MSC_VER
/*
 * we do not want the warnings about the old deprecated and unsecure CRT functions
 * since these examples can be compiled under *nix as well
 */
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/time.h>
#else
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#endif

#include "20myheader.h"

char* iptos(u_long in);

#define     SNAPLEN	    68       // captured packet size
#define     MAXPKT      1000	    // max number of stored pkts
pcap_t*     adhandle;
int         tot_cap_num = 0;
void online_dump(u_char* dumpfile, const struct pcap_pkthdr* pkt_hdr, const u_char* pkt_data);


// Function prototypes
void		ifprint0(pcap_if_t* alldevs, int* dnum);
void 		get_header_ethernet(struct eth_header *eth);
void 		get_header_ip(struct ip_header* iph, int ttl);
void		get_header_icmp(struct icmp_header* icmph);
uint16_t	calculate_checksum(uint8_t* buffer, int length);
void		generate_packet_icmp(u_char *packet, int *pklen, int ttl);
void 		packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

// Global variables
uint8_t		dst_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };	// ***** ethernet: gateway MAC address
uint8_t		src_mac[6] = { 0x14, 0x18, 0xc3, 0x7e, 0xdc, 0x0e };		// ***** ethernet: source MAC
uint16_t	etype = 0x0800;					// ethernet: type field

int			TTL;							// ip datagram: TTL value
uint16_t 	pkid = 2114;					// ip datagram: packet id
uint8_t 	protocol = IPPROTO_ICMP;		// ip datagram: protocol field
char        *sip 	 = "172.21.47.255";			// ***** ip datagram: your ip address
char        *dip 	 = "8.8.8.8";			// ***** ip datagram: default target ip address
char hostIP[12];



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

int main()
{
    // ***** set internal varialbes
	// ***** COMPLETE your program

    pcap_if_t*          alldevs;
    pcap_if_t*          d;
    char                errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr* pkt_hdr;    // captured packet header
    const u_char*       pkt_data;   // caputred packet data
    time_t              local_tv_sec;
    struct tm*          ltime;
    char                timestr[16];

    int		i, ret;			// for general use
    int		ndNum = 0;	// number of network devices
    int		devNum;		// device Id used for online packet capture
    pcap_dumper_t*          dumpfile;

    //printf("default device: %s\n", pcap_lookupdev(errbuf));

	// ***** 1. List the device list
    /* Retrieve the device list */
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

    printf("\n");
    pcap_addr_t* a;
    for (d = alldevs; d; d = d->next)
    {
        // device name
        printf(" [%d] %s", ++ndNum, d->name);

        // description
        if (d->description)
            printf(" (%s) ", d->description);

        // loopback address
        // printf("\tLoopback: %s\n", (d->flags & PCAP_IF_LOOPBACK) ? "yes" : "no");

        // IP addresses
        for (a = d->addresses; a; a = a->next) {
            if (a->addr->sa_family == AF_INET) {
                if (a->addr)
                    printf("[%s]", iptos(((struct sockaddr_in*)a->addr)->sin_addr.s_addr));
                //if (a->netmask)
                //    printf("\tNetmask: %s\n", iptos(((struct sockaddr_in*)a->netmask)->sin_addr.s_addr));
                //if (a->broadaddr)
                //    printf("\tBroadcast Address: %s\n", iptos(((struct sockaddr_in*)a->broadaddr)->sin_addr.s_addr));
                //if (a->dstaddr)
                //    printf("\tDestination Address: %s\n", iptos(((struct sockaddr_in*)a->dstaddr)->sin_addr.s_addr));
                break;
            }
        }
        printf(" flag=%d\n", (int)d->flags);
    }
    printf("\n");
    /* error ? */
    if (ndNum == 0)
    {
        printf("\nNo interfaces found! Make sure Npcap is installed.\n");
        return -1;
    }

    /* select device for online packet capture application */
    printf(" Enter the interface number (1-%d):", ndNum);
    scanf("%d", &devNum);

	
	// ***** 2. Select athe device to be used,
	// *****    print the information on your selected device,
	// *****    and open the adaptor (using promiscusos mode)	
    /* Jump to the selected adapter */
    for (d = alldevs, i = 0; i < devNum - 1; d = d->next, i++);

    /* Open the adapter */
    if ((adhandle = pcap_open_live( d->name, // name of the device
                                    65536,     // portion of the packet to capture. 
                                                // 65536 grants that the whole packet will be captured on all the MACs.
                                    1,         // promiscuous mode
                                    1000,      // read timeout
                                    errbuf)     // error buffer
        ) == NULL)
    {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap\n", d->name);
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return -1;
    }

    printf("\n Selected device %s is available\n\n", d->description);
    // pcap_freealldevs(alldevs);

	// ***** 3. Get target host ip

	printf("\n Enter the host IP : ");
	scanf("%s", hostIP);

	printf("\n Host IP : %s", hostIP);

	// ***** 4. Traceroute main
	// ***** COMPLETE your program
	// ***** ...
	
	struct eth_header 	*reth;			// ethernet header from the captured packet
	struct ip_header 	*riph;			// ip header from the captured packet
	struct icmp_header 	*ricmph;		// icmp header from the captured packet
	int					max_ttl = 30;	// maximum number of hops for traceroute 
	int					max_seq = 1;	// number of echo requests for each TTL (here, simply 1)
    for (TTL = 1; TTL <= max_ttl; TTL++) {
		for (int j=1; j<=max_seq; j++) {
			printf("\n TTL : %d, j : %d", TTL, j);
			
			// ***** 4.1. Generate ICMP packet
			// ***** COMPLETE the internal of pcap_sendpacket	

			u_char packet[4096] = { 0 };
			int pklen;
			pcap_t *adhandle;

			generate_packet_icmp(packet, &pklen, TTL); 

			printf("\n Generate Done");
			
			// ***** 4.2. Send the packet
			// ***** COMPLETE the internal of pcap_sendpacket
			//
			int res = pcap_sendpacket(adhandle, packet, pklen);
			
			// ***** 4.3. Check the correct reply
			// ***** COMPLETE your program internal while()	
			printf("Hop %d : ", TTL);



			// while (1) {
			// 	// packet capture (you can use it)
			// 	res = pcap_next_ex(adhandle, &header, &response);
				
			// 	// **** 4.4 check the correct replied packet 
			// 	// **** if etype==0x0800 && ip->protocol = IPPROTO_ICMP, else contiue while loop
			// 	// **** refer to the following pseudo code
			// 	if (){
			// 		if (icmp_type == ICMP_TIME_EXCEEDED ) {
			// 			// do correct action on this event
			// 			// go to next action and break;
			// 		}
			// 		else if (icmp_type == ICMP_ECHOREPLY ) {
			// 			// compare the addresses
			// 			// if match, here the traceroute has completed
			// 			//    do correct action, 
			// 			//    give terminate routine 
			// 			//    and break
			// 		}
			// 	}
				
			// 	//***** 4.5 check the timeout condition
			// 	//          for this, you can use the following example to get times and check the timeout condition
			// 	// long				time_diff;
			// 	// #ifdef _WIN32
			// 	//		time_t start_time, end_time;
			// 	//		double time_difference;
			// 	//		time(&start_time);
			// 	//		...
			// 	//		time(&end_time);
			// 	//		time_diff = difftime(end_time, start_time);			
			// 	// #else
			// 	//		struct timeval		start, now;
			// 	//		gettimeofday(&start, NULL);
			// 	//		...
			// 	//		gettimeofday(&now, NULL);
			// 	//		time_diff = now.tv_sec - start.tv_sec;			
			// 	// #endif
			// 	// ***** COMPLETE the internal of pcap_sendpacket
			// 	// ***** ...
				
			// 	if (time_diff > 5) {
			// 		printf(" timeout ...");
			// 		break;
			// 	}
			// }
			// packet interval

			usleep(100); // 100ms ���
		}

		// ***** 4.6 check terminate condition when traceroute has completed
		// *****	this is related to else if (icmp_type == ICMP_ECHOREPLY ) of the 4.4 
		// ***** COMPLETE the internal of pcap_sendpacket
		// ***** ...

	}

	/* Free the device list */
	pcap_freealldevs(alldevs);

	return 0;
}


//============================================================================
// List of the available devices (you can use it)
//============================================================================
void ifprint0(pcap_if_t* alldevs, int* dnum)
{
	pcap_if_t*		d;
	pcap_addr_t*	a;
	int				i = 0;

	printf("Available devices:\n");
	for (d = alldevs; d; d = d->next)
	{
		/* Name */
		printf("%2d. %s - ", ++i, d->name);

		/* Description */
		if (d->description)
			printf("%s", d->description);
		else {
			char desc[100];
			sprintf(desc,"No description available");
			printf("%s ", desc);
		}
		/* IP addresses */
		for (a = d->addresses; a; a = a->next) {
			if (a->addr->sa_family == AF_INET)
				if (a->addr > 0) {
					struct sockaddr_in* ipv4 = (struct sockaddr_in*)a->addr;
#ifdef _WIN32
					char ip[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET, &ipv4->sin_addr, ip, INET_ADDRSTRLEN);
					printf(" %s ", ip);
#else
					printf(" %s ", inet_ntoa(ipv4->sin_addr));
#endif
				}
		}
		printf("\n");
	}
	*dnum = i;
}

//============================================================================
// 5. ICMP packet  (Complete the program)
// ***** COMPLETE the internal of the subroutine
//============================================================================
void	generate_packet_icmp(u_char *packet, int *pklen, int ttl)
{
    // complete the following
	char sendbuf[4096] = { 0 };
	struct eth_header *eth;
	struct ip_header *iph;
	struct icmp_header *icmph;
	char					*udata;
	size_t					dlen;
	struct pseudo_header	psh;

	/* Prepare headers*/
	eth		= (struct eth_header*)sendbuf;
	iph		= (struct ip_header*)(sendbuf + sizeof(struct eth_header));
	icmph   = (struct icmp_header*)(sendbuf + sizeof(struct icmp_header));
	udata = (char*)(sendbuf + sizeof(struct eth_header) + sizeof(struct ip_header) + sizeof(struct icmp_header));
	// ICMP header
	get_header_icmp(icmph);
	dlen = strlen(udata);

	printf("\nICMP Done");

	// IP header
	get_header_ip(iph, ttl);

	printf("\nIP Done");

	// Ethernet header
	get_header_ethernet(eth);
	
	printf("\nETH Done");
    // make them into one packet: complete the following
	memcpy(packet, sendbuf, sizeof(sendbuf));

}

//============================================================================
// 5-1. Ethernet header (Complete the program)
// ***** COMPLETE the internal of the subroutine
//============================================================================
void get_header_ethernet(struct eth_header *eth) {
    // complete the following

	// copy destination mac to ethernet header
	memcpy(eth->dst_mac, dst_mac, 6);

	// copy source mac to ethernet header
	memcpy(eth->src_mac, src_mac, 6);
	eth->ethertype = htons(etype);
}

//============================================================================
// 5-2. IP header (Complete the program)
// ***** COMPLETE the internal of the subroutine
//============================================================================
void get_header_ip(struct ip_header* iph, int ttl)
{
    // complete the following
	uint32_t 			src_ip, dst_ip;
	uint16_t			upp_len, checksum = 0;
	uint8_t 			buffer[4096] = { 0 };

	protocol = IPPROTO_ICMP;

	iph->version			= 4;				// IPv4
	iph->ihl				= 5;				// Header Length (5 words = 20 bytes), no option
	iph->tos				= 0;	
	iph->total_length		= htons(upp_len);	// no option
	iph->id					= htons(54321);		// Example Identification
	iph->fragment_offset	= 0;				// No fragmentation
	iph->ttl				= ttl;				// Default TTL
	iph->protocol			= protocol;			// Protocol (TCP, UDP, ICMP, etc)
	iph->checksum			= checksum;			// Will be calculated later
	iph->src_ip				= src_ip;	// Convert to network byte order
	iph->dst_ip				= dst_ip;	// Convert to network byte order

	memcpy(buffer, iph, sizeof(struct ip_header));
	
	// checksum calculation
	iph->checksum			= calculate_checksum(buffer, sizeof(struct ip_header));

	//memcpy(buffer, iph, sizeof(struct ip_header));
	//checksum = calculate_checksum(buffer, sizeof(struct ip_header));
	//printf("in get_header_ip: checksum2 = %u (%u) iphlen=%d \n", checksum, iph->checksum, sizeof(struct ip_header));
}

//============================================================================
// 5-3. ICMP header  (Complete the program)
// ***** COMPLETE the internal of the subroutine
//============================================================================
void	get_header_icmp(struct icmp_header	*icmph)
{
	uint16_t			upp_len, checksum = 0;
	uint8_t 			buffer[4096] = { 0 };
    // complete the following
	icmph->type = 0x08;
	icmph->code = 0x00;
	icmph->id = 0x00;
	icmph->seq = 0x00;
	icmph->checksum = calculate_checksum(buffer, sizeof(struct icmp_header));

}

//============================================================================
// 5-4. Checksum Calculation (Complete the program)
// ***** COMPLETE the internal of the subroutine
//============================================================================
uint16_t calculate_checksum(uint8_t* buffer, int length) 
{
	uint32_t	checksum	= 0;
	uint16_t*	buffer_16	= (uint16_t*)buffer;

	printf("in calc_checksum: length=%d\n", length);

	// Padding zero for odd number buffer
	if (length % 2 == 1) {
		buffer[length] = 0;
		length += 1;
	}

	while (length > 1) {
		checksum += *buffer_16++;  // 2bytes
		length -= 2;
	}
	// Wrap around for carry over 16-bits length
	checksum = (checksum >> 16) + (checksum & 0xFFFF);

	// one's complement
	checksum = ~checksum;

	return checksum;
}



/* From tcptraceroute, convert a numeric IP address to a string : source Npcap SDK */
#define IPTOSBUFFERS	12
char* iptos(u_long in)
{
    static char output[IPTOSBUFFERS][3 * 4 + 3 + 1];
    static short which;
    u_char* p;

    p = (u_char*)&in;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return output[which];
}

// Callback function for online dump
void online_dump(u_char* dumpfile, const struct pcap_pkthdr* pkt_hdr, const u_char* pkt_data)
{
    printf("(%4d) clen=%3d, len=%4d \r", tot_cap_num++, pkt_hdr->caplen, pkt_hdr->len);

    // save the packet on the dump file
    pcap_dump(dumpfile, pkt_hdr, pkt_data);

    if (tot_cap_num > MAXPKT) {
        printf("\n\n %d-packets were captured ...\n", tot_cap_num);
        // close all devices and files
        pcap_close(adhandle);
        pcap_dump_close((pcap_dumper_t *)dumpfile);
        exit(0);
    }

}
