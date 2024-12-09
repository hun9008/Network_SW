#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h> // For inet_ntop
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h> 
#include <sys/socket.h>
#endif
#define SNAPLEN 2048  // ĸó�� �ִ� ��Ŷ ����


struct ieee80211_frame {
    uint16_t frame_control;    // Frame Control �ʵ� (2����Ʈ)
    uint16_t duration_id;      // Duration/ID �ʵ� (2����Ʈ)
    uint8_t address1[6];       // �ּ� 1 (BSSID �Ǵ� Destination MAC �ּ�)
    uint8_t address2[6];       // �ּ� 2 (Source MAC �ּ�)
    uint8_t address3[6];       // �ּ� 3 (BSSID �Ǵ� AP MAC �ּ�)
    uint16_t seq_ctrl;         // Sequence Control (2����Ʈ)
}__attribute__((packed));

void packet_handler(u_char* dumpfile, const struct pcap_pkthdr* header, const u_char* packet) {
    pcap_dump(dumpfile, header, packet);

    printf("Captured a packet of length %d\n", header->len);
}

int main() {
    pcap_if_t* all_devices, * device;
    char errbuf[PCAP_ERRBUF_SIZE];
    int i = 0;
    const char* filename = "80211_frames.pcap";  
    pcap_dumper_t* dumpfile;    

    if (pcap_findalldevs(&all_devices, errbuf) == -1) {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

    printf("Available devices:\n");
    for (device = all_devices; device != NULL; device = device->next) {
        printf("%d. %s - %s\n", ++i, device->name,
            device->description ? device->description : "No description available");
    }

    if (i == 0) {
        printf("No devices found. Make sure Npcap is installed.\n");
        return 0;
    }

    int interface_number;
    printf("Enter the interface number (1-%d): ", i);
    scanf("%d", &interface_number);

    if (interface_number < 1 || interface_number > i) {
        printf("Invalid interface number.\n");
        pcap_freealldevs(all_devices);
        return 0;
    }

    device = all_devices;
    for (i = 1; i < interface_number; ++i) {
        device = device->next;
    }

    pcap_t* handle = pcap_open_live(device->name, 65536, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", device->name, errbuf);
        pcap_freealldevs(all_devices);
        return 0;
    }

    dumpfile = pcap_dump_open(handle, filename);
    if (dumpfile == NULL) {
        fprintf(stderr, "Couldn't open dump file: %s\n", pcap_geterr(handle));
        return 1;
    }
    printf("Saving 802.11 frames to %s\n", filename);

    pcap_set_rfmon(handle, 1); 
    pcap_dispatch(handle, 500, packet_handler, (u_char*)dumpfile); // dispatch to call upon packet
    //pcap_loop(handle, 100, packet_handler, (u_char*)dumpfile);

    pcap_dump_close(dumpfile);
    pcap_close(handle);

    return 0;
}
