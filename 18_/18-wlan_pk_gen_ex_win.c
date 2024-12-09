// UDP packet generation through Wlan interface
//

#ifdef _MSC_VER
/*
 * we do not want the warnings about the old deprecated and unsecure CRT functions
 * since these examples can be compiled under *nix as well
 */
#define _CRT_SECURE_NO_WARNINGS
#endif

#include	<pcap.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include 	"18-myheader.h"

#ifdef _WIN32
#include <tchar.h>
BOOL LoadNpcapDlls()
{
	_TCHAR npcap_dir[512];
	UINT len;
	len = GetSystemDirectory(npcap_dir, 480);
	if (!len) {
		fprintf(stderr, "Error in GetSystemDirectory: %x", GetLastError());
		return FALSE;
	}
	_tcscat_s(npcap_dir, 512, _T("\\Npcap"));
	if (SetDllDirectory(npcap_dir) == 0) {
		fprintf(stderr, "Error in SetDllDirectory: %x", GetLastError());
		return FALSE;
	}
	return TRUE;
}
#endif

#define		WLAN_FC_TYPE_DATA		2
#define		WLAN_FC_SUBTYPE_DATA	0

// A bogus addresses just to show that it can be done
const uint8_t	src_mac[6] 	= { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab };
const uint8_t	dst_mac[6] 	= { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
const uint8_t	ssid_mac[6]	= { 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6 };

// Ethernet header fields example
uint16_t	etype = 0x0800;

// IP hdaerd fields example
char        *sip = "169.254.1.1";
char        *dip = "255.255.255.255";
uint8_t		protocol = IPPROTO_UDP;

// Application data
char        *app_data = "Hello World, I am the Hero !!!";

// logical link control (LLC) header should
const uint8_t ipllc[8] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00 };

//=======================================================================================
// main function
//=======================================================================================
int main(void) {
	// packet to be transmitted
	uint8_t				packet[4096] = { 0 };
	int					pklen;

	// PCAP vars
	char 				errbuf[PCAP_ERRBUF_SIZE];
	pcap_t				*handle;
	pcap_if_t 			*alldevs, *d;
	pcap_addr_t			*a;
    int 				i = 0, dnum=0, snum=0;
    const char			*filename = "80211_frames.pcap";  	// PCAP file name
    pcap_dumper_t		*dumpfile;       					// PCAP file handler

    // get all available network interfaces
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

	// Scan the list printing every entry
	ifprint0(alldevs, &dnum);

	printf("Enter the interface number (1-%d):", dnum);
	scanf("%d", &snum);

	if (snum < 1 || snum > dnum)
	{
		printf("\nInterface number out of range.\n");
		pcap_freealldevs(alldevs); /* Free the device list */
		return -1;
	}

	// Jump to the selected adapter
	for (d = alldevs, i = 0; i < snum - 1; d = d->next, i++);

	// name and IP addresses of the selected adapter
	for (a = d->addresses; a; a = a->next)
		if (a->addr != NULL && a->addr->sa_family == AF_INET) {
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)a->addr;
#ifdef _WIN32
			char ip[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET, &ipv4->sin_addr, ip, INET_ADDRSTRLEN);
			printf("\n\tSelected interface: %s, IP address: %s\n\n", d->name, ip);
#else
			printf("\n\tSelected interface: %s, IP address: %s\n\n", d->name, inet_ntoa(ipv4->sin_addr));
#endif
	}

	// open the selected interface
    handle = pcap_open_live(d->name, 65536, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", d->name, errbuf);
        pcap_freealldevs(alldevs);
        return 0;
    }

    // open the selected device
	handle = pcap_create(d->name, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "pcap_create 실패: %s\n", errbuf);
		return 1;
	}

	// set monitor mode to the device
	if (pcap_set_rfmon(handle, 1) != 0) {
		fprintf(stderr, "Monitor mode setup failure: %s\n", pcap_geterr(handle));
	}
	else {
		printf("Monitor mode setup success....\n");
	}

	// activate the device
	if (pcap_activate(handle) != 0) {
		fprintf(stderr, "pcap_activate failure: %s\n", pcap_geterr(handle));
		pcap_close(handle);
		return 2;
	}

	// generate UDP packet for 802.11 wlan
	generate_packet_wlan_udp(packet, &pklen);
			
	// send UDP packet over 802.11 wlan
	if (pcap_sendpacket(handle, packet, pklen) == 0) {
		pcap_close(handle);
		return 0;
	}

	// close all
	pcap_perror(handle, "Failed to inject packet");
	pcap_close(handle);
	return 1;
}

//------------------------------------------------------------------------
// Checksum Calculation
//------------------------------------------------------------------------
uint16_t calculate_checksum(uint8_t* buffer, int length) {
	uint32_t	checksum	= 0;
	uint16_t*	buffer_16	= (uint16_t*)buffer;

	//printf("in calc_checksum: length=%d\n", length);

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

//------------------------------------------------------------------------
// list simple information of all devices
//------------------------------------------------------------------------
void ifprint0(pcap_if_t* alldevs, int* dnum)
{
	pcap_if_t*		d;
	pcap_addr_t*	a;
	int				i = 0;

	printf("Available devices:");
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

//------------------------------------------------------------------------
//  IP header generation
//------------------------------------------------------------------------
void get_header_ip(struct ip_header* iph)
{
	uint32_t 			src_ip, dst_ip;
	uint16_t			upp_len, checksum = 0;
	uint8_t 			buffer[4096] = { 0 };

	// IP header fields example
#ifndef _WIN32
	src_ip = inet_addr(sip);			// source IP address
	dst_ip = inet_addr(dip);			// destination IP address
#else
	inet_pton(AF_INET, sip, &src_ip);	// source IP address
	inet_pton(AF_INET, dip, &dst_ip);	// destination IP address
#endif

	if (protocol == IPPROTO_TCP)
		upp_len = 20 + sizeof(struct tcp_header) + strlen(app_data);
	else
		upp_len = 20 + sizeof(struct udp_header) + strlen(app_data);

	iph->version			= 4;				// IPv4
	iph->ihl				= 5;				// Header Length (5 words = 20 bytes), no option
	iph->tos				= 0;				// Default TOS
	iph->total_length		= htons(upp_len);	// no option
	iph->id					= htons(54321);		// Example Identification
	iph->fragment_offset	= htons(0x4000); 	// Don't fragmentation
	iph->ttl				= 64;				// Default TTL
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

//------------------------------------------------------------------------
//  UDP Header generation
//------------------------------------------------------------------------
void	get_header_udp(struct udp_header* udph)
{
	struct pseudo_header	psh;
	uint32_t				checksum = 0;
	uint8_t 				buffer[4096] = { 0 };
	int						hlen;

	udph->src_port	= htons(src_port);			// Convert to network byte order
	udph->dst_port	= htons(dst_port);			// Convert to network byte order
	udph->length	= htons(sizeof(struct udp_header) + strlen(app_data));
	udph->checksum	= 0;						// Optional for UDP, can be left as 0

	// Get pseudo header
	get_header_pseudo(&psh);
	//printf("in udp(): udplen=%d pshlen=%d\n", udph->length, psh.length);
	
	memcpy(buffer, &psh, sizeof(struct pseudo_header));
	memcpy(buffer + sizeof(struct pseudo_header), udph, sizeof(struct udp_header));
	memcpy(buffer + sizeof(struct pseudo_header) + sizeof(struct udp_header), app_data, strlen(app_data));
	hlen = (int)sizeof(struct pseudo_header) + (int)sizeof(struct udp_header) + (int)strlen(app_data);
	udph->checksum = calculate_checksum(buffer, hlen);

	// for validation
	memcpy(buffer + sizeof(struct pseudo_header), udph, sizeof(struct udp_header));
	int h1 = (int)sizeof(struct pseudo_header) + (int)sizeof(struct udp_header);
	printf("in gen_udp(): psheln=%d d[0]=%c d[1]= %c checksum1 = %d, checksum2 = %d\n", hlen, buffer[h1], buffer[h1+1], udph->checksum, calculate_checksum(buffer, hlen));
}

//------------------------------------------------------------------------
//  pseudo header generation for calculating UDP and TCP checksums
//------------------------------------------------------------------------
void	get_header_pseudo(struct pseudo_header *psh)
{
	uint32_t 			src_ip, dst_ip;

	// IP header fields example
#ifndef _WIN32
	src_ip = inet_addr(sip);			// source IP address
	dst_ip = inet_addr(dip);			// destination IP address
#else
	inet_pton(AF_INET, sip, &src_ip);	// source IP address
	inet_pton(AF_INET, dip, &dst_ip);	// destination IP address
#endif

	// pseudo header 
	psh->src_addr	= src_ip;
	psh->dst_addr	= dst_ip;
	psh->zeros		= 0;
	psh->protocol	= protocol;
	if (protocol == IPPROTO_TCP)
		psh->length = htons(sizeof(struct tcp_header) + strlen(app_data));
	else
		psh->length = htons(sizeof(struct udp_header) + strlen(app_data));
}

//------------------------------------------------------------------------
//   UDP packet generation: radiotap + wlan + ip + udp + data
//------------------------------------------------------------------------
void	generate_packet_wlan_udp(u_char* packet, int *pklen)
{
	char          			sendbuf[4096] = { 0 };
	uint8_t 				*radioh; 	// radiotap header
	struct ieee80211_hdr	*wlanh;		// 802.11 header
	uint8_t					fcchunk[2]; // frame control in 802.11 header
	uint8_t					*llc;		// LLC part
	struct ip_header		*iph;		// IP header
	struct udp_header		*udph;		// TCP header
	char					*data;		// Application Data

	// Total buffer size (note the 0 bytes of data and the 4 bytes of FCS 
	*pklen = sizeof(u8aRadiotapHeader) + sizeof(struct ieee80211_hdr) + sizeof(ipllc) 
				+ sizeof(struct ip_header) + sizeof(struct udp_header) + strlen(app_data)
				+ 4; // FCS

	// Put our pointers in the right place
	radioh		= (uint8_t *) sendbuf;
	wlanh		= (struct ieee80211_hdr *) (radioh+sizeof(u8aRadiotapHeader));
	llc			= (uint8_t *) (wlanh+1);
	iph			= (struct ip_header *) (llc+sizeof(ipllc));
	udph		= (struct udp_header *) (iph+1);
	data		= (char *) (uint8_t *) (udph+1);

	// The radiotap header has been explained already */
	memcpy(radioh, u8aRadiotapHeader, sizeof(u8aRadiotapHeader));

  // construct the 802.11 header
	fcchunk[0] = ((WLAN_FC_TYPE_DATA << 2) | (WLAN_FC_SUBTYPE_DATA << 4));
	fcchunk[1] = 0x02;
	memcpy(&wlanh->frame_control, &fcchunk[0], 2*sizeof(uint8_t));
	wlanh->duration_id = 0xffff;
	memcpy(&wlanh->addr1[0], src_mac, 6*sizeof(uint8_t));
	memcpy(&wlanh->addr2[0], dst_mac, 6*sizeof(uint8_t));
	memcpy(&wlanh->addr3[0], ssid_mac, 6*sizeof(uint8_t));
	wlanh->seq_ctrl = 0;
	//wlanh->addr4;

	/* The LLC+SNAP header has already been explained above */
	memcpy(llc, ipllc, 8*sizeof(uint8_t));

	// IP header
	get_header_ip(iph);

	// TCP/UDP header
	if ( protocol == IPPROTO_UDP )
		get_header_udp(udph);

	// Application data
	strcpy(data, app_data);
	//memcpy(data, app_data, sizeof(app_data));
	

	//void get_header_udp(struct udp_header* udph, struct ip_header* iph);
	memcpy(packet, sendbuf, sizeof(sendbuf));
}
