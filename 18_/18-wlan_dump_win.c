#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<winsock2.h> // for the uint8_t

int	 count = 0;
void pcapDump(u_char* args, const struct pcap_pkthdr* header, const u_char* packet);

int main() {
    char			errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t		*alldevs, *d;
    pcap_t			*handle;
    int				interface_number;
	int				i=0;
	const char		*filename = "80211_frames2.pcap";  // PCAP file name
	pcap_dumper_t	*dumpfile;						   // PCAP file handle

    // list available devices
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        printf("pcap_findalldevs 실패: %s\n", errbuf);
        return 1;
    }

    printf("Available devices:\n");
    for (d = alldevs; d != NULL; d = d->next) {
        printf("%2d. %s - %s\n", ++i, d->name,
            d->description ? d->description : "No description available");
    }

    // select a device to be used
    printf("Enter the interface number (1-%d): ", i);
    scanf_s("%d", &interface_number);

    if (interface_number < 1 || interface_number > i) {
        printf("Invalid interface number.\n");
        pcap_freealldevs(alldevs);
        return 0;
    }

    // jump to the selected device
    d = alldevs;
    for (i = 1; i < interface_number; ++i) {
        d = d->next;
    }
    printf("Selected device: %s\n", d->name);

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

	// opne PCAP file
	dumpfile = pcap_dump_open(handle, filename);
	if (dumpfile == NULL) {
		fprintf(stderr, "Couldn't open dump file: %s\n", pcap_geterr(handle));
		return 1;
	}
	printf("Saving 802.11 frames to %s\n", filename);

    // capture packets
    pcap_loop(handle, 100, pcapDump, (u_char*)dumpfile); // dispatch to call upon packet 

	// close all
	pcap_dump_close(dumpfile);
    pcap_close(handle);
    pcap_freealldevs(alldevs);
    return 0;
}

// Packet handler:
void pcapDump(u_char* dumpfile, const struct pcap_pkthdr* header, const u_char* packet) 
{
	// store captured packet
	pcap_dump(dumpfile, header, packet);

	// simple information
	printf("%d-th captured packet length %d\n", count++, header->len);
}