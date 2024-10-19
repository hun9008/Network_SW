#include <winsock.h>
#include <stdlib.h> 
#include <stdio.h>
#include <time.h>

#define BUFSIZE 512
#define MAX_CLIENTS 100

int sum_messages = 0;
int sum_bytes = 0;
time_t start_time;
time_t current_time;
volatile int running_time = 0;
int server_running = 1; // 서버 실행 상태 플래그

void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void printCommandMenu() {
    printf("****************************************\n");
    printf("*              COMMAND MENU            *\n");
    printf("****************************************\n");
    printf("*      ___                             *\n");
    printf("*     |   |      Press 'i' to get      *\n");
    printf("*     | i |   -> Client Info           *\n");
    printf("*     |___|                            *\n");
    printf("*                                      *\n");
    printf("*      ___                             *\n");
    printf("*     |   |      Press 's' to get      *\n");
    printf("*     | s |   -> Chat Statistics       *\n");
    printf("*     |___|                            *\n");
    printf("*                                      *\n");
    printf("*      ___                             *\n");
    printf("*     |   |      Press 'q' to Quit     *\n");
    printf("*     | q |                            *\n");
    printf("*     |___|                            *\n");
    printf("*                                      *\n");
    printf("****************************************\n");
}

typedef struct {
    SOCKET socket;  
    SOCKADDR_IN addr;  
    int active;      
    char nickname[20]; 
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];  // 클라이언트 정보 저장 배열

void add_client(SOCKET client_sock, SOCKADDR_IN* clientaddr, char* nickname) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].socket = client_sock;
            clients[i].addr = *clientaddr;
            clients[i].active = 1;
            strcpy(clients[i].nickname, nickname);
            printf("New client added: [%s] %s:%d\n", nickname, inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port));
            return;
        }
    }
    printf("Client list is full!\n");
}

void remove_client(SOCKET client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_sock) {
            clients[i].active = 0;
            closesocket(clients[i].socket);
            printf("Client disconnected: %s:%d\n", inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
            return;
        }
    }
}

int is_known_client(SOCKET client_sock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_sock) {
            return 1;
        }
    }
    return 0;
}


void broadcast_message(SOCKET sender_sock, char* buf, int len) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socket != sender_sock) {
            // 메시지를 보낸 클라이언트를 제외한 나머지에게 메시지를 전송
            int retval = send(clients[i].socket, buf, len, 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                printf("retval : %d\n", retval);
            }
        }
    }
}

DWORD WINAPI ProcessClient(LPVOID arg) {
    SOCKET client_sock = (SOCKET)arg;
    char buf[BUFSIZE + 1];
    int retval;

    while (server_running) {
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            printf("retval : %d\n", retval);
            remove_client(client_sock);
            break;
        }

        buf[retval] = '\0';
        printf("Message from client: %s\n", buf);
        sum_messages++;
        sum_bytes += retval;

        // 에코 메시지: 보낸 클라이언트를 제외한 모든 클라이언트에게 전송
        broadcast_message(client_sock, buf, retval);
    }

    return 0;
}

void printClientInfo() {
    int active_clients_num = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].nickname[0] != '\0') {  // 닉네임이 설정된 클라이언트만 카운트
            if (clients[i].active)
                active_clients_num++;
        }
    }

    printf("****************************************\n");
    printf("*              CLIENT INFO             *\n");
    printf("****************************************\n");
    printf("*           Client Number : %d          *\n", active_clients_num);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].nickname[0] != '\0') {  // 닉네임이 설정된 클라이언트만 출력
            printf("* %-11s %-8s %s:%d *\n",
                   clients[i].nickname,
                   clients[i].active ? "active" : "inactive",
                   inet_ntoa(clients[i].addr.sin_addr),
                   ntohs(clients[i].addr.sin_port));
        }
    }
    printf("****************************************\n");
}

void printChatStatistics() {
    current_time = time(NULL);
    running_time = (int)difftime(current_time, start_time);

    printf("****************************************\n");
    printf("*           CHAT STATISTICS            *\n");
    printf("****************************************\n");
    printf("* Msg/min : %-27d*\n", sum_messages / running_time * 60);
    printf("* Bytes/min : %-25d*\n", sum_bytes / running_time * 60);
    printf("* Total Msgs : %-24d*\n", sum_messages);
    printf("* Total Bytes : %-23d*\n", sum_bytes);
    printf("* Total Time : %-24d*\n", running_time);
    printf("****************************************\n");
}

void printQuit() {
    printf("****************************************\n");
    printf("*              I'm Quit!               *\n");
    printf("****************************************\n");
    server_running = 0; // 서버 종료 플래그 설정
    exit(0);
}

DWORD WINAPI ProcessStocastic(LPVOID arg) {
    char command[2];

    while (server_running) {
        if (fgets(command, 2, stdin) == NULL)
            return 0;

        if (command[0] == 'i') {
            printClientInfo();
        } else if (command[0] == 's') {
            printChatStatistics();
        } else if (command[0] == 'q') {
            printQuit();
        } else {
            printf("*  i - info || s - static || q - quit  *\n");
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int retval;
    SOCKET listen_sock, client_sock;
    SOCKADDR_IN serveraddr, clientaddr;
    int addrlen;
    HANDLE mThread, sThread;
    DWORD mThreadId, sThreadId;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9000);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    retval = listen(listen_sock, SOMAXCONN); // 클라이언트 연결 대기
    if (retval == SOCKET_ERROR) err_quit("listen()");

    start_time = time(NULL); // 서버 시작 시간 기록

    printCommandMenu();
    printf("[TCP SERVER READY] success bind & listen.\n");

    // 서버 명령어 처리 스레드 생성
    sThread = CreateThread(NULL, 0, ProcessStocastic, NULL, 0, &sThreadId);
    if (sThread == NULL) err_quit("CreateThread()");

    while (server_running) {
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen); // 클라이언트 접속 수락
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            continue;
        }

        // 클라이언트의 nickname을 수신하여 추가
        char nickname[20];
        retval = recv(client_sock, nickname, sizeof(nickname) - 1, 0);
        if (retval <= 0) {
            closesocket(client_sock);
            continue;
        }
        nickname[retval] = '\0'; // 닉네임 문자열 종료

        // 클라이언트 추가
        add_client(client_sock, &clientaddr, nickname);

        // 각 클라이언트 처리 스레드 생성
        mThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)client_sock, 0, &mThreadId);
        if (mThread == NULL) {
            err_quit("CreateThread()");
        }
        // CloseHandle(mThread); // 스레드 핸들 닫기
    }

    // 서버가 종료되면 소켓 정리
    WaitForSingleObject(sThread, INFINITE);
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}