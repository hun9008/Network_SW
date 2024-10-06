#include <winsock.h>
#include <stdlib.h> 
#include <stdio.h>

#define BUFSIZE 512
#define MAX_CLIENTS 100

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
    printf("****************************************\n");
}

typedef struct {
    SOCKADDR_IN addr;  
    int active;      
    char nickname[20]; 
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];  // 클라이언트 정보 저장 배열

void add_client(SOCKADDR_IN* clientaddr, char* nickname) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].addr = *clientaddr;
            clients[i].active = 1;
            strcpy(clients[i].nickname, nickname);
            printf("New client added: [%s] %s:%d\n", nickname, inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port));
            return;
        }
    }
    printf("Client list is full!\n");
}

int is_known_client(SOCKADDR_IN* clientaddr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active &&
            clients[i].addr.sin_addr.s_addr == clientaddr->sin_addr.s_addr &&
            clients[i].addr.sin_port == clientaddr->sin_port) {
            return 1;  // 이미 등록된 클라이언트
        }
    }
    return 0;  // 새로운 클라이언트
}

void broadcast_message(SOCKET sock, char* buf, int len, SOCKADDR_IN* senderaddr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active &&
            (clients[i].addr.sin_addr.s_addr != senderaddr->sin_addr.s_addr ||
             clients[i].addr.sin_port != senderaddr->sin_port)) {
            // 메시지를 보낸 클라이언트를 제외한 나머지에게 메시지를 전송
            int retval = sendto(sock, buf, len, 0, (SOCKADDR*)&clients[i].addr, sizeof(clients[i].addr));
            if (retval == SOCKET_ERROR) {
                err_display("sendto()");
            }
        }
    }
}

DWORD WINAPI ProcessClient(LPVOID arg) {
    SOCKET sock = (SOCKET)arg;
    SOCKADDR_IN clientaddr;
    char buf[BUFSIZE + 1];
    int addrlen, retval;
    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        // select 대기
        retval = select(0, &readfds, NULL, NULL, NULL);
        if (retval == SOCKET_ERROR) {
            err_display("select()");
            continue;
        }

        // 소켓에 읽을 데이터가 있는지 확인
        if (FD_ISSET(sock, &readfds)) {
            addrlen = sizeof(clientaddr);
            retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&clientaddr, &addrlen);
            if (retval == SOCKET_ERROR) {
                err_display("recvfrom()");
                continue;
            }

            buf[retval] = '\0';
            printf("Message from client (%s:%d): %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);

            // buf는 [nickname] message 형태로 전송됨. 이 중 nickname을 추출
            char nickname[20];
            int i, j = 0;
            for (i = 1; i < retval; i++) {  
                if (buf[i] == ']') {
                    break;
                }
                nickname[j++] = buf[i];
            }
            nickname[j] = '\0';


            // 새로운 클라이언트라면 추가
            if (!is_known_client(&clientaddr)) {
                add_client(&clientaddr, nickname);
            }

            // 에코 메시지: 보낸 클라이언트를 제외한 모든 클라이언트에게 전송
            broadcast_message(sock, buf, retval, &clientaddr);
        }
    }

    return 0;
}

void printClientInfo() {
    int active_clients_num = 0;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            active_clients_num++;
        }
    }

    printf("****************************************\n");
    printf("*              CLIENT INFO             *\n");
    printf("****************************************\n");
    printf("*           Client Number : %d         *\n", active_clients_num);

    printf("* No. IP Address      Port Number       *\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            printf("* %s. %s:%d\n", &clients[i].nickname, inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
        }
    }
    printf("****************************************\n");
}

void printChatStatistics() {
    printf("****************************************\n");
    printf("*           CHAT STATISTICS            *\n");
    printf("****************************************\n");
    printf("*                                      *\n");
    printf("*                                      *\n");
    printf("*                                      *\n");
    printf("*                                      *\n");
    printf("*                                      *\n");
    printf("****************************************\n");
}

void printQuit() {
    printf("****************************************\n");
    printf("*              I'm Quit!               *\n");
    printf("****************************************\n");
}


DWORD WINAPI ProcessStocastic(LPVOID arg) {

    char command;

    if (fgets(command, 1, stdin) == NULL) 
        return 0;
    
    if (command == 'i') {
        printClientInfo();
    } else if (command == 's') {
        printChatStatistics();
    } else if (command == 'q') {
        printQuit();
    } else {
        printf("Invalid Command\n");
    }


}

int main(int argc, char* argv[])
{
    int retval, addrlen;
    SOCKET sock;
    SOCKADDR_IN serveraddr, clientaddr;
    HANDLE hThread;
    DWORD ThreadId;

    WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;
      
    sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == INVALID_SOCKET) err_quit("listen socket()");

    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(9000);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");

    printCommandMenu();
	printf("[UDP SERVER READY] success bind & listen.\n");

	hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)sock, 0, &ThreadId);
	if (hThread == NULL) err_quit("CreateThread()");

	WaitForSingleObject(hThread, INFINITE);

	closesocket(sock);
	WSACleanup();
	
	return 0;
}

