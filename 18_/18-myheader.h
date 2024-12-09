// myheader.h 
// includes useful headers

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#ifdef _WIN32
#pragma pack(push, 1)
#endif

#define		MAC_ADDR_LEN	6
#define		ARPOP_REQUEST	1		// ARP request
#define     ARPOP_REPLY     2
#define		ARPHRD_ETHER	1 		// Ethernet 10/100Mbps
#define		ETHERTYPE_IP	0x0800	// IP protocol
#define		ETHERTYPE_ARP	0x0806	// ARP protocol
#define     ORIG_PACKET_LEN 64

// Ethernet Header Structure
struct eth_header {
    uint8_t  dst_mac[6];  // Destination MAC Address
    uint8_t  src_mac[6];  // Source MAC Address
    uint16_t ethertype;   // EtherType (e.g., IPv4: 0x0800)
};

// ARP packet
#ifdef _WIN32
#pragma pack(push, 1)
struct arp_message
{
    uint16_t	ar_hrd;		// Format of hardware address
    uint16_t	ar_pro;		// Format of protocol address
    uint8_t		ar_hln;		// Length of hardware address
    uint8_t		ar_pln;		// Length of protocol address
    uint16_t	ar_op;		// ARP opcode (command)
	uint8_t		ar_sha[6];	// Sender hardware address
	uint32_t	ar_sip;		// Sender IP address
	uint8_t		ar_tha[6];	// Target hardware address
	uint32_t	ar_tip;		// Target IP address	
};
#pragma pack(pop)
#else
struct arp_message
{
    uint16_t	ar_hrd;		// Format of hardware address
    uint16_t	ar_pro;		// Format of protocol address
    uint8_t		ar_hln;		// Length of hardware address
    uint8_t		ar_pln;		// Length of protocol address
    uint16_t	ar_op;		// ARP opcode (command)
	uint8_t		ar_sha[6];	// Sender hardware address
	uint32_t	ar_sip;		// Sender IP address
	uint8_t		ar_tha[6];	// Target hardware address
	uint32_t	ar_tip;		// Target IP address	
}__attribute__((packed));
#endif
  
// IP header sturcute (IPv4)
struct ip_header {
    uint8_t  ihl : 4;			// Header Length
    uint8_t  version : 4;		// IP Version
    uint8_t  tos;				// Type of Service
    uint16_t total_length;		// Total Length
    uint16_t id;				// Identification
    uint16_t fragment_offset;	// Fragment Offset
    uint8_t  ttl;				// Time to Live
    uint8_t  protocol;			// Protocol (TCP: 6, UDP: 17)
    uint16_t checksum;			// Header Checksum
    uint32_t src_ip;			// Source IP Address
    uint32_t dst_ip;			// Destination IP Address
};


// UDP header sturcute
struct udp_header {
    uint16_t src_port;		// Source Port
    uint16_t dst_port;		// Destination Port
    uint16_t length;		// UDP Length
    uint16_t checksum;		// UDP Checksum
};

// TCP header structure
struct tcp_header {
    uint16_t src_port;			// Source Port
    uint16_t dst_port;			// Destination Port
    uint32_t seq_num;			// Sequence Number
    uint32_t ack_num;			// Acknowledgment Number
    uint8_t  offset : 4;		// Data Offset (Header Length)
    uint8_t  reserved : 4;		// Reserved
    uint8_t  flags;				// Control Flags
    uint16_t window;			// Window Size
    uint16_t checksum;			// Checksum
    uint16_t urgent_pointer;	// Urgent Pointer
};

// Pseudo-header for checksum calculation
struct pseudo_header {
    uint32_t	src_addr;		// Source IP Address
    uint32_t	dst_addr;		// Destination IP Address
    uint8_t 	zeros;			// zero bits
    uint8_t		protocol;		// protocol field in IP header
    uint16_t	length;			// length of TCP/UDP header + App data
};

// 802.11 wlan header, defined in include/linux/ieee80211.h
struct ieee80211_hdr {
	uint16_t	frame_control;
	uint16_t	duration_id;
	uint8_t		addr1[6];
	uint8_t		addr2[6];
	uint8_t		addr3[6];
	uint16_t	seq_ctrl;
	//uint8_t	addr4[6];
} __attribute__ ((packed));

// Radiotap Header
static const uint8_t u8aRadiotapHeader[] = {
	0x00, 			// u_int8_t		it_version : major version of the radiotap header
	0x00, 			// u_int8_t		it_pad : unused)
	0x18, 0x00, 	// u_int16_t	it_len : the entire length of the radiotap data (in bytes)
	0x0f, 0x80, 0x00, 0x00,	// u_int32_t	it_present : a bitmask of the radiotap data fields that follows the radiotap header	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // timestamp
	0x10,											// flags: set to IEEE80211_RADIOTAP_F_FCS, request the card to add a FCS at the end of the packet
	0x00,											// rate (in 500KHz unit)
	0x00, 0x00, 0x00, 0x00, 						// channel
	0x08, 0x00,										// set to IEEE80211_RADIOTAP_F_TX_NOACK, which means the card won't wait for an ACK for this frame,
};

// TCP/UDP header fields example
uint16_t    src_port    = 9999;
uint16_t    dst_port    = 80;
uint32_t    seq_num     = 100;
uint32_t    ack_num     = 100;
uint8_t     flags       = 0x18;	// CEUAPRSF: 0x18="00011000"=ACK+PSH flags


// Function prototypes
void		ifprint1(pcap_if_t* alldevs);
void		ifprint0(pcap_if_t* alldevs, int* dnum);
const char* iptos(struct sockaddr* sockaddr);

void		generate_packet_tcp_udp(u_char* packet, int* pklen);
void		generate_packet_wlan_udp(u_char* packet, int* pklen);
void	    generate_packet_arp(u_char* packet, int op, u_char *smac, u_char *tmac, int* pklen);

void		get_header_ethernet(struct eth_header* eth);
void		get_header_ip(struct ip_header* iph);
void		get_header_tcp(struct tcp_header* tcph);
void		get_header_udp(struct udp_header* udph);
void	    get_header_pseudo(struct pseudo_header* psh);

uint16_t	calculate_checksum(uint8_t* buffer, int length);