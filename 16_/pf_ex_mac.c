#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/bpf.h>

int main() {
    int bpf_fd;
    char bpf_device[16];
    struct ifreq ifr;
    unsigned char buffer[4096];
    ssize_t len;

    // BPF 장치 열기
    for (int i = 0; i < 99; i++) {
        snprintf(bpf_device, sizeof(bpf_device), "/dev/bpf%d", i);
        bpf_fd = open(bpf_device, O_RDWR);
        if (bpf_fd != -1) {
            printf("Opened BPF device: %s\n", bpf_device);
            break;
        }
    }
    if (bpf_fd == -1) {
        perror("Failed to open BPF device");
        return -1;
    }

    // 네트워크 인터페이스 설정 (예: en0)
    strncpy(ifr.ifr_name, "en0", sizeof(ifr.ifr_name));
    if (ioctl(bpf_fd, BIOCSETIF, &ifr) == -1) {
        perror("Failed to set interface");
        close(bpf_fd);
        return -1;
    }

    // 필터 없음 (모든 패킷 수신)
    int immediate = 1;
    if (ioctl(bpf_fd, BIOCIMMEDIATE, &immediate) == -1) {
        perror("Failed to set immediate mode");
        close(bpf_fd);
        return -1;
    }

    // 버퍼 크기 확인
    int buffer_size;
    if (ioctl(bpf_fd, BIOCGBLEN, &buffer_size) == -1) {
        perror("Failed to get buffer length");
        close(bpf_fd);
        return -1;
    }
    printf("Using buffer size: %d bytes\n", buffer_size);

    printf("Listening for packets on interface %s...\n", ifr.ifr_name);

    // 패킷 수신 루프
    while (1) {
        len = read(bpf_fd, buffer, sizeof(buffer));
        if (len == -1) {
            perror("Failed to read packet");
            break;
        }

        printf("Received packet of length %ld\n", len);
        for (int i = 0; i < len; i++) {
            printf("%02x ", buffer[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    }

    close(bpf_fd);
    return 0;
}