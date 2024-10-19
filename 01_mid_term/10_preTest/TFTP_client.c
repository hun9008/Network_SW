#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFSIZE 512

int set_nickname = 0;
char nickname[20];  // 전역 변수로 닉네임 저장

enum TFTP_OPCODE {
    RRQ = 1,
    WRQ = 2,
    DATA = 3,
    ACK = 4,
    ERROR = 5
};

void err_quit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void err_display(const char *msg) {
    perror(msg);
}

// TFTP 형식에 맞는 RRQ 요청 생성 및 전송
void send_rrq(int sock, const char *filename) {
    char buffer[BUFSIZE];
    int len = snprintf(buffer, sizeof(buffer), "%c%c%s%c%s%c", 
                       0, 1,  // Opcode = RRQ (1)
                       filename, 0,  // 파일 이름
                       "octet", 0);  // 모드 (binary)

    if (send(sock, buffer, len, 0) == -1) {
        err_display("send() for RRQ");
    } else {
        printf("[TCP Client] Sent RRQ for file: %s\n", filename);
    }
}

// 파일 수신: ./client_files에 저장
void receive_file(int sock, const char *filename) {
    char filepath[BUFSIZE];
    snprintf(filepath, sizeof(filepath), "./client_files/%s_%s", nickname, filename);

    mkdir("./client_files", 0755);  // 디렉터리 생성

    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("File open error");
        return;
    }

    char buffer[BUFSIZE + 4];  // 헤더 포함 버퍼
    int block = 1;

    while (1) {
        int retval = recv(sock, buffer, BUFSIZE + 4, 0);
        if (retval < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("recv() would block, retrying...\n");
                continue;
            } else {
                perror("recv()");
                break;
            }
        } else if (retval == 0) {
            printf("Server closed the connection.\n");
            break;
        }

        if (buffer[1] != DATA) {  // DATA opcode 확인
            printf("Invalid packet received.\n");
            break;
        }

        int received_block = (buffer[2] << 8) | buffer[3];
        if (received_block != block) {
            printf("Block mismatch: expected %d, received %d\n", block, received_block);
            break;
        }

        fwrite(buffer + 4, 1, retval - 4, file);  // 파일 쓰기

        if (retval - 4 < BUFSIZE) {  // 마지막 블록 확인
            printf("[File Transfer] Transfer complete.\n");
            break;
        }

        // ACK 전송
        char ack[4] = {0, ACK, (block >> 8) & 0xFF, block & 0xFF};
        send(sock, ack, sizeof(ack), 0);

        block++;
    }

    fclose(file);
}

void *SendMessageThread(void *arg) {
    int sock = *(int *)arg;
    char temp[BUFSIZE + 1];

    printf("\n[SET NICKNAME] : ");
    if (fgets(nickname, 20, stdin) == NULL) return NULL;

    int nlen = strlen(nickname);
    if (nickname[nlen - 1] == '\n') nickname[nlen - 1] = '\0';

    if (send(sock, nickname, strlen(nickname), 0) == -1) {
        err_display("send()");
    }
    printf("[TCP Client] Sent nickname.\n");
    set_nickname = 1;

    while (1) {
        printf("\n[Request File] ");
        if (fgets(temp, BUFSIZE, stdin) == NULL) break;

        int len = strlen(temp);
        if (temp[len - 1] == '\n') temp[len - 1] = '\0';

        send_rrq(sock, temp);  // 파일 요청 보내기
    }

    return NULL;
}

void *ReceiveMessageThread(void *arg) {
    int sock = *(int *)arg;
    char buffer[BUFSIZE + 4];  // TFTP 패킷 (4바이트 헤더 포함)
    int block = 1;  // 예상되는 첫 번째 블록 번호

    char block_content[BUFSIZE * 1000];  // 블록 내용 저장

    while (1) {
        // 서버로부터 데이터 수신
        int retval = recv(sock, buffer, sizeof(buffer), 0);
        if (retval < 0) {
            err_display("[TCP Client] recv()");
            break;
        } else if (retval == 0) {
            printf("[TCP Client] Server closed the connection.\n");
            break;
        }

        // TFTP 데이터 패킷인지 확인
        if (buffer[1] != DATA) {
            printf("[TCP Client] Invalid packet received.\n");
            break;
        }

        // 수신한 블록 번호 확인
        int received_block = (buffer[2] << 8) | buffer[3];
        if (received_block != block) {
            printf("[TCP Client] Block mismatch: expected %d, received %d\n", block, received_block);
            break;
        }

        // 수신한 데이터를 출력
        printf("[TCP Client] Received block %d (%d bytes)\n", block, retval - 4);

        // 마지막 블록인지 확인 (512바이트 미만이면 마지막 블록)
        if (retval - 4 < BUFSIZE) {


            printf("[TCP Client] File transfer complete.\n");
            break;
        }

        // 수신한 데이터를 block_content에 저장
        memcpy(block_content + (block - 1) * BUFSIZE, buffer + 4, retval - 4);

        // ACK 전송
        char ack[4] = {0, ACK, (block >> 8) & 0xFF, block & 0xFF};
        if (send(sock, ack, sizeof(ack), 0) < 0) {
            err_display("[TCP Client] send() ACK");
            break;
        }
        printf("[TCP Client] Sent ACK for block %d\n", block);

        // 다음 블록으로 이동
        block++;
    }

    // 받은 파일의 총 텍스트 수 출력
    printf("[TCP Client] Total text received: %d bytes\n", block * BUFSIZE);

    // 받은 파일을 ./client_files에 nickname_filename으로 저장  
    char filepath[BUFSIZE];
    snprintf(filepath, sizeof(filepath), "./client_files/%s_%s", nickname, "received.txt");
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("[TCP Client] File open error");
        return NULL;
    }

    fwrite(block_content, 1, block * BUFSIZE, file);
    fclose(file);

    printf("[BLOCK CONTENT] %s\n", block_content);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        err_quit("connect()");
    }

    printf("[TCP Client] Connected to the server.\n");

    pthread_t sendThread, recvThread;
    if (pthread_create(&sendThread, NULL, SendMessageThread, &sock) != 0) {
        err_quit("pthread_create() for send");
    }

    if (set_nickname == 0) {
        while (set_nickname == 0) sleep(1);
    }

    if (pthread_create(&recvThread, NULL, ReceiveMessageThread, &sock) != 0) {
        err_quit("pthread_create() for recv");
    }

    pthread_join(sendThread, NULL);

    close(sock);
    return 0;
}