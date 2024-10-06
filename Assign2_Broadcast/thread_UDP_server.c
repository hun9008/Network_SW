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

typedef struct {
    SOCKADDR_IN addr;  // 클라이언트 주소
    int active;        // 활성화 여부
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];  // 클라이언트 정보 저장 배열

void add_client(SOCKADDR_IN* clientaddr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].addr = *clientaddr;
            clients[i].active = 1;
            printf("New client added: %s:%d\n", inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port));
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

            // 새로운 클라이언트라면 추가
            if (!is_known_client(&clientaddr)) {
                add_client(&clientaddr);
            }

            // 에코 메시지: 보낸 클라이언트를 제외한 모든 클라이언트에게 전송
            broadcast_message(sock, buf, retval, &clientaddr);
        }
    }

    return 0;
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

	printf("[UDP SERVER READY] success bind & listen.\n");

	hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)sock, 0, &ThreadId);
	if (hThread == NULL) err_quit("CreateThread()");

	WaitForSingleObject(hThread, INFINITE);

	closesocket(sock);
	WSACleanup();
	
	return 0;
}

