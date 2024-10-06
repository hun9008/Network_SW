// COechoSvr.cpp
// TCP echo server
//
// �����: ��Ʈ��ũ����Ʈ�����
// ���ִ��б� ����Ʈ�����а�
// ����� ��Ƽ�̵�� ��� ��Ʈ��ũ ������ (mmcn.ajou.ac.kr)
//
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFSIZE 1500

// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
	printf("Error [%s] ... program terminated \n",msg);
	exit(-1);
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
	printf("socket function error [%s]\n", msg);
}

int main(int argc, char* argv[])
{
	// ������ ��ſ� ����� ����
	SOCKET listen_sock;
	SOCKET client_sock;
	SOCKADDR_IN serveraddr;
	SOCKADDR_IN clientaddr;
	int		addrlen;
	char	buf[BUFSIZE+1];
	int		retval, msglen;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return -1;

	// socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");	
	
	// server address
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind()
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");
	
	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if(retval == SOCKET_ERROR) err_quit("listen()");


	while(1){
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if(client_sock == INVALID_SOCKET) {
			err_display("accept()");
			continue;
		}

		printf("\n[TCP Server] Client accepted : IP addr=%s, port=%d\n", 
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// Ŭ���̾�Ʈ�� ������ ���
		while(1){
			// ������ �ޱ�
			msglen = recv(client_sock, buf, BUFSIZE, 0);
			if(msglen == SOCKET_ERROR){
				err_display("recv()");
				break;
			}
			else if(msglen == 0)
				break;

			// ���� ������ ���
			buf[msglen] = '\0';
			printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
				ntohs(clientaddr.sin_port), buf);

			// ������ ������
			retval = send(client_sock, buf, msglen, 0);
			if(retval == SOCKET_ERROR){
				err_display("send()");
				break;
			}
		}

		// closesocket()
		closesocket(client_sock);
		printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", 
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	}

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}
