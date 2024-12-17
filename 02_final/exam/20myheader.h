// myheader.h 
// includes useful headers

// Ethernet Header Structure
struct eth_header {
    uint8_t  dst_mac[6];  // Destination MAC Address
    uint8_t  src_mac[6];  // Source MAC Address
    uint16_t ethertype;   // EtherType (e.g., IPv4: 0x0800)
};
  
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

// ICMP header (IPv4)
struct icmp_header {
    uint8_t  type;        // icmp type
    uint8_t  code;        // icmp code
    uint16_t checksum;    // checksum
    uint16_t id;          // echo id
    uint16_t seq;         // echo seq
};

// Pseudo-header for checksum calculation
struct pseudo_header {
    uint32_t	src_addr;		// Source IP Address
    uint32_t	dst_addr;		// Destination IP Address
    uint8_t 	zeros;			// zero bits
    uint8_t		protocol;		// protocol field in IP header
    uint16_t	length;			// length of TCP/UDP header + App data
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

#define		ETHERTYPE_IP			0x0800	// IP protocol
#define     ORIG_PACKET_LEN 		64
//#define		IPPROTO_ICMP	1
#define		ICMP_ECHO				8		  // Echo request
#define 	ICMP_TIME_EXCEEDED      11        // Time Exceeded
#define		ICMP_ECHOREPLY			0         // Eecho reply

char *app_data = "hi?";