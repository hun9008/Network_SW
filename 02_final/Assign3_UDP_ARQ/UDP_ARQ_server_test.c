#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BUFSIZE 512

float P = 0.5;

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
    //printf("[STOI] %d\n", num);
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

void save_recv_buf(struct msg_format* msg_form, unsigned char* recv_buf) {
    msg_form->seq_num = decode_string_to_data(recv_buf, 2);
    for (int i = 0; i < BUFSIZE; i++) {
        msg_form->msg[i] = recv_buf[i + 2];
    }
    msg_form->msg[BUFSIZE] = '\0';
    return;
}

typedef struct statistics {
    float p;
    int msgs;
    int retry_msgs;
    int all_msgs;
    float retry_rate;
} statistics;

void init_statistics(struct statistics* stats) {
    stats->p = P;
    stats->msgs = 1;
    stats->retry_msgs = 0;
    stats->all_msgs = 1;
    stats->retry_rate = 0.0;
}

typedef struct quit_sock {
    struct sockaddr_in addr;
    int quit_flag;
    int retry_flag;
    struct statistics stats;
} quit_sock;

int main(int argc, char* argv[]) {

    struct msg_format msg_form;
    struct quit_sock quit_sock[1024];
    int quit_sock_idx = -1;

    int syntax;
    char ip[20] = "";
    int port;

    if(argc < 3) {
        //printf("[Parameter Error]\tToo small\n");
        return -1;
    } else if (argc == 3) {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        //printf("IP : %s\n", ip);
        //printf("Port : %d\n", port);
    } else if (argc == 4) {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        //printf("IP : %s\n", ip);
        //printf("Port : %d\n", port);

        P = atof(argv[3]);
    } else {
        //printf("[Parameter Error]\tToo many\n");
        return -1;
    }

    int retval;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(retval == -1) err_quit("bind()");
    //printf("\n[INFO] BIND SUCCESS\n");

    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    unsigned char buf[BUFSIZE + 1];

    while(1) {
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
        if(retval == -1) {
            err_display("recvfrom()");
            continue;
        }

        if (quit_sock_idx == -1) {
            quit_sock_idx++;
            quit_sock[quit_sock_idx].addr = clientaddr;
            quit_sock[quit_sock_idx].quit_flag = 0;
            quit_sock[quit_sock_idx].retry_flag = 0;
            init_statistics(&quit_sock[quit_sock_idx].stats);
        } else {
            int flag = 0;
            for(int i = 0; i < quit_sock_idx + 1; i++) {
                if (quit_sock[i].addr.sin_addr.s_addr == clientaddr.sin_addr.s_addr &&
                    quit_sock[i].addr.sin_port == clientaddr.sin_port) {
                    flag = 1;
                    if (quit_sock[i].retry_flag == 0) {
                        // //printf("[DEBUG]\t\tcontinue");
                        quit_sock[i].stats.msgs++;
                        quit_sock[i].stats.all_msgs++;
                    }
                    break;
                }
            }
            if (flag == 0) {
                quit_sock_idx++;
                quit_sock[quit_sock_idx].addr = clientaddr;
                quit_sock[quit_sock_idx].quit_flag = 0;
                quit_sock[quit_sock_idx].retry_flag = 0;
                init_statistics(&quit_sock[quit_sock_idx].stats);
            }
        }

        // //printf("\n[DEBUG]\t\t\t%d", quit_sock_idx);

        //printf("\n[ROW DATA]\t\t%s", buf);
        //printf("\n[RECV MSG]\t\t%s", &buf[2]);

        memset(&msg_form, 0, sizeof(msg_form));
        save_recv_buf(&msg_form, buf);

        if (strcmp(msg_form.msg, "quit") == 0 || strcmp(msg_form.msg, "QUIT") == 0) {
            for(int i = 0; i < quit_sock_idx + 1; i++) {
                if (quit_sock[i].addr.sin_addr.s_addr == clientaddr.sin_addr.s_addr &&
                    quit_sock[i].addr.sin_port == clientaddr.sin_port) {
                    quit_sock[i].quit_flag = 1;
                    
                    quit_sock[i].stats.msgs--;
                    quit_sock[i].stats.all_msgs--;

                    break;
                }
            }
        } else {

            buf[retval] = '\0';
            //printf("\n[UDP/%s:%d]\t%s", inet_ntoa(clientaddr.sin_addr), 
            // ntohs(clientaddr.sin_port), msg_form.msg);
        }

        float rand_num = (rand() % 100) / 100.0;
        //printf("\n[RAND SEED]\t\t%0.2f", rand_num);

        if(rand_num > P) {
            ////printf("\n[DROP MSG]\t\t%s", "[ ... XXX ... ]");
            ////printf("\n");
            
            for(int i = 0; i < quit_sock_idx + 1; i++) {
                if (quit_sock[i].addr.sin_addr.s_addr == clientaddr.sin_addr.s_addr &&
                    quit_sock[i].addr.sin_port == clientaddr.sin_port) {
                    quit_sock[i].retry_flag = 1;

                    // ambiguous
                    if (quit_sock[i].quit_flag == 0) {
                        quit_sock[i].stats.retry_msgs++;
                        quit_sock[i].stats.all_msgs++;
                    }
                    break;
                }
            }

            memset(buf, 0, sizeof(buf));
            continue;
        } else {

            retval = sendto(sock, buf, retval, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
            if(retval == -1) {
                err_display("sendto()");
                continue;
            }

            ////printf("\n[ACK ABOUT]\t\t%d", msg_form.seq_num);
            ////printf("\n");
            
            for(int i = 0; i < quit_sock_idx + 1; i++) {
                if (quit_sock[i].addr.sin_addr.s_addr == clientaddr.sin_addr.s_addr &&
                    quit_sock[i].addr.sin_port == clientaddr.sin_port) {
                    quit_sock[i].stats.retry_rate = (float)quit_sock[i].stats.retry_msgs / (float)quit_sock[i].stats.all_msgs;
                    quit_sock[i].retry_flag = 0;
                    break;
                }
            }

            // ////printf("\n[STATISTICS]\tp=<%0.2f>, N1=<%d>, N2=<%d>, N3=<%d>, R=<%0.2f>",
            // quit_sock[quit_sock_idx].stats.p, quit_sock[quit_sock_idx].stats.msgs, quit_sock[quit_sock_idx].stats.retry_msgs,
            // quit_sock[quit_sock_idx].stats.all_msgs, quit_sock[quit_sock_idx].stats.retry_rate);

            memset(buf, 0, sizeof(buf));

            for(int i = 0; i < quit_sock_idx + 1; i++) {
                if (quit_sock[i].quit_flag == 1) {
                    ////printf("\n[QUIT/%s:%d]\t[REMOVED]", inet_ntoa(quit_sock[i].addr.sin_addr), 
                    // ntohs(quit_sock[i].addr.sin_port));
                    ////printf("\n[STATISTICS]\tp=<%0.2f>, N1=<%d>, N2=<%d>, N3=<%d>, R=<%0.2f>",
                    // quit_sock[i].stats.p, quit_sock[i].stats.msgs, quit_sock[i].stats.retry_msgs,
                    // quit_sock[i].stats.all_msgs, quit_sock[i].stats.retry_rate);

                    ////printf("\n");
                    quit_sock[i].quit_flag = 0;
                    memset(&quit_sock[i].addr, 0, sizeof(quit_sock[i].addr));
                    quit_sock_idx--;

                    // save statistics path : ./data.tsv
                    FILE* fp = fopen("./data.tsv", "a");
                    if (fp == NULL) {
                        err_quit("fopen()");
                    }

                    // file에 아무것도 없을 때
                    if (ftell(fp) == 0) {
                        fprintf(fp, "p\tN1\tN2\tN3\tR\n");
                    }

                    fprintf(fp, "%0.2f\t%d\t%d\t%d\t%0.2f\n", quit_sock[i].stats.p, quit_sock[i].stats.msgs,
                    quit_sock[i].stats.retry_msgs, quit_sock[i].stats.all_msgs, quit_sock[i].stats.retry_rate);

                    fclose(fp);
                }
            }
        }
    }

    close(sock);

    return 0;
}