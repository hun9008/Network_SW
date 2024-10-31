#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 1500

int TIMEOUT_SEC = 10;

void err_quit(char *msg) {
    perror(msg);
    exit(-1);
}

void err_display(char *msg) {
    perror(msg);
}

int stoi(char* str) {
    int num = 0;
    int i = 0;
    while (str[i] != '\0') {
        num = num * 10 + (str[i] - '0');
        i++;
    }
    printf("[STOI] %d\n", num);
    return num;
}

char itoc(int num) {
    return (char)(num);
}

char* itos(int num) {
    if (num == 0) {
        return "0";
    }
    int size = 0;
    for (int i = num; i > 0; i /= 10) {
        size++;
    }

    char* str = (char*)malloc(size + 1);
    if (str == NULL) {
        err_quit("malloc()");
    }
    int i = 0;
    for (i = size - 1; i >= 0; i--) {
        str[i] = (num % 10) + '0';
        num /= 10;
    }
    str[size] = '\0';
    return str;
}

unsigned char* encode_data_to_string(int data, int size) {
    unsigned char* encoded_data = (unsigned char*)malloc(size);
    if (encoded_data == NULL) {
        err_quit("malloc()");
    }
    for (int i = 0; i < size; i++) {
        encoded_data[i] = (data >> (8 * i)) & 0xFF;
    }
    return encoded_data;
}

unsigned char* encode_ip_to_string(char* ip, int size) {
    unsigned char* encoded_ip = (unsigned char*)malloc(size);
    if (encoded_ip == NULL) {
        err_quit("malloc()");
    }
    char* token = strtok(ip, ".");
    int i = 0;
    while (token != NULL) {
        encoded_ip[i] = itoc(stoi(token));
        token = strtok(NULL, ".");
        i++;
    }
    return encoded_ip;
}

int decode_string_to_data(unsigned char* encoded_data, int size) {
    int data = 0;
    for (int i = 0; i < size; i++) {
        data |= (encoded_data[i] << (8 * i));
    }
    return data;
}

char* decode_string_to_ip(unsigned char* encoded_ip, int size) {
    char* ip = (char*)malloc(size);
    if (ip == NULL) {
        err_quit("malloc()");
    }
    for (int i = 0; i < size; i++) {
        if (i == size - 1) {
            strcat(ip, itos((int)encoded_ip[i]));
        } else {
            strcat(ip, itos((int)encoded_ip[i]));
            strcat(ip, ".");
        }
    }
    return ip;
}

typedef struct msg_format {
    int seq_num;
    char msg[BUFSIZE + 1];
} msg_format;

unsigned char* make_send_buf(struct msg_format msg_form) {
    unsigned char* send_buf = (unsigned char*)malloc(BUFSIZE + 3);
    if (send_buf == NULL) {
        err_quit("malloc()");
    }
    unsigned char* encoded_seq_num = encode_data_to_string(msg_form.seq_num, 2);
    for (int i = 0; i < 2; i++) {
        send_buf[i] = encoded_seq_num[i];
    }
    for (int i = 0; i < strlen(msg_form.msg); i++) {
        send_buf[i + 2] = msg_form.msg[i];
    }
    printf("\n[ROW DATA]\t\t%s", send_buf);
    printf("\n[SEND MSG]\t\t%s", &send_buf[2]);
    return send_buf;
}

int main(int argc, char* argv[]) {

    struct msg_format msg_form;
    msg_form.seq_num = 0;

    int quit_flag = 0;

    int syntax;
    char ip[20] = "";
    int port;

    time_t start_time, current_time;

    if (argc < 3) {
        printf("[Parameter Error]\tToo small\n");
        return -1;
    } else if (argc == 3) {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    } else if (argc == 4) {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);

        TIMEOUT_SEC = atoi(argv[3]);
    } else {
        printf("[Parameter Error]\tToo many\n");
        return -1;
    }

    int retval;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    struct sockaddr_in peeraddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];
    int len;

    fd_set readfds;
    struct timeval timeout;

    while (1) {
        printf("\n[Input Message] ");
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        if (strcmp(buf, "quit") == 0 || strcmp(buf, "QUIT") == 0)
            quit_flag = 1;


        strcpy(msg_form.msg, buf);
        unsigned char *send_buf = make_send_buf(msg_form);
        // printf("\n[SEND SIZE]\t\t%lu", sizeof(send_buf));
        memset(buf, 0, sizeof(buf));

        while(1) {

            retval = sendto(sock, send_buf, len + 2, 0,
                (struct sockaddr *)&serveraddr, sizeof(serveraddr));
            if (retval == -1) {
                err_display("sendto()");
                continue;
            }
            printf("\n[UDP Client]\t\tSent %d bytes.", retval);
            printf("\n[SEQ NUM]\t\t%d", msg_form.seq_num);
            printf("\n\t\t\t[ ... WAITING ... ]");
            printf("\n");

            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = 0;

            retval = select(sock + 1, &readfds, NULL, NULL, &timeout);

            if (retval == -1) {
                err_display("select()");
                break;
            } else if (retval == 0) {
                printf("\n[TIMEOUT]\t\tRetransmitting...");
                continue;
            }

            memset(&send_buf, 0, sizeof(send_buf));

            addrlen = sizeof(peeraddr);
            unsigned char recv_buf[BUFSIZE + 1];
            retval = recvfrom(sock, recv_buf, BUFSIZE, 0,
                (struct sockaddr *)&peeraddr, &addrlen);
            if (retval == -1) {
                err_display("recvfrom()");
                continue;
            }

            buf[retval] = '\0';
            printf("\n[UDP Client]\t\tReceived %d bytes.", retval);
            printf("\n[Received Message]\t%s", &recv_buf[2]);
            printf("\n\t\t\t[ ..... DONE .... ]\n");

            msg_form.seq_num += len - 1;
            break;
        }

        free(send_buf);

        if (quit_flag == 1)
            break;
    }

    close(sock);
    return 0;
}