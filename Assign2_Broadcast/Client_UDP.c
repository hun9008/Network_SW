#include <winsock.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 512

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

// 전달 인자 구조체 정의
typedef struct {
    SOCKET sock;
    SOCKADDR_IN serveraddr;
} ThreadArgs;

// 메시지 송신을 담당하는 함수
DWORD WINAPI SendMessageThread(LPVOID arg)
{
    ThreadArgs* args = (ThreadArgs*)arg;
    SOCKET sock = args->sock;
    SOCKADDR_IN* serveraddr = &args->serveraddr;
    char buf[BUFSIZE + 1];
    char temp[BUFSIZE + 1];
    char nickname[20];
    int retval;

    printf("\n[SET NICKNAME] : ");
    
    if (fgets(nickname, 20, stdin) == NULL)
        return 0;

    int nlen = strlen(nickname);
    if (nickname[nlen - 1] == '\n')
        nickname[nlen - 1] = '\0';

    while (1) {

        printf("\n[Input Message] ");

        if (fgets(temp, BUFSIZE + 1, stdin) == NULL)
            break;
    
        sprintf(buf, "[%s] %s", nickname, temp);

        int len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (strlen(buf) == 0) {
            printf("Message is empty\n");
            continue;
        }

        // 서버로 메시지 전송
        retval = sendto(sock, buf, strlen(buf), 0, (SOCKADDR*)serveraddr, sizeof(*serveraddr));
        if (retval == SOCKET_ERROR) {
            err_display("sendto()");
            continue;
        }

        printf("[UDP Client] Sent %d bytes.\n", retval);
    }
    
    return 0;
}

// 메시지 수신을 담당하는 함수
DWORD WINAPI ReceiveMessageThread(LPVOID arg)
{
    ThreadArgs* args = (ThreadArgs*)arg;
    SOCKET sock = args->sock;
    SOCKADDR_IN clientaddr;
    int addrlen, retval;
    char buf[BUFSIZE + 1];

    addrlen = sizeof(clientaddr);

    while (1) {
        // 메시지 수신
        retval = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&clientaddr, &addrlen);
            
        if (retval == SOCKET_ERROR) {
            err_display("recvfrom()");
            continue;
        }

        buf[retval] = '\0';  // 수신된 메시지 처리
        printf("[UDP Client] Received %d bytes from server.\n", retval);
        printf("[Received Message] %s\n", buf);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    char ip[20] = "";
    int port;

    // 파라미터 확인
    if (argc != 3) {
        printf("Parameter Error\n");
        return -1;
    } else {
        strcpy(ip, argv[1]);
        port = atoi(argv[2]);
        printf("IP : %s\n", ip);
        printf("Port : %d\n", port);
    }

    int retval;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    // UDP 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 클라이언트 소켓을 바인딩할 로컬 주소 설정
    SOCKADDR_IN clientaddr;
    ZeroMemory(&clientaddr, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(0);  // 임의의 포트를 사용
    clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 로컬 주소

    // 클라이언트 소켓을 바인딩
    retval = bind(sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // 서버 주소 설정
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    // ThreadArgs 구조체 생성하여 소켓과 서버 주소 전달
    ThreadArgs args;
    args.sock = sock;
    args.serveraddr = serveraddr;
    
    // 스레드 생성
    HANDLE sendThread, recvThread;
    DWORD sendThreadId, recvThreadId;

    sendThread = CreateThread(NULL, 0, SendMessageThread, &args, 0, &sendThreadId);
    if (sendThread == NULL) err_quit("CreateThread() for send");

    recvThread = CreateThread(NULL, 0, ReceiveMessageThread, &args, 0, &recvThreadId);
    if (recvThread == NULL) err_quit("CreateThread() for recv");

    // 스레드 핸들 닫기
    WaitForSingleObject(sendThread, INFINITE);
    WaitForSingleObject(recvThread, INFINITE);

    CloseHandle(sendThread);
    CloseHandle(recvThread);

    closesocket(sock);

    WSACleanup();

    return 0;
}

