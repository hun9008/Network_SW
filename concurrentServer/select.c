#include "TCPEchoServerWS.h"

void main(int argc, char *argv[])
{
	int *servSock;
	fd_set sockSet;
	long timeout;
	struct timeval selTimeout;
	int running = 1;
	int noPorts;
	int port;
	unsigned short portNo;
	WSADATA wsaData;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s <Timeout (secs.)> <Server Port 1> ...\n", argv[0]);
		exit(1);
	}

	timeout = atol(argv[1]);
	noPorts = argc - 2;

	servSock = (int *) malloc(noPorts * sizeof(int));

	if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0)
	{
		fprintf(stderr, "WSAStartup() failed");
		exit(1);
	}

	for (port = 0; port < noPorts; port++)
	{
		portNo = atoi(argv[port + 2]);
		servSock[port] = CreateTCPServerSocket(portNo);
	}

	while (running)
	{
		FD_ZERO(&sockSet);
		for (port = 0; port < noPorts; port++)
		{
			FD_SET(servSock[port], &sockSet);
		}

		selTimeout.tv_sec = timeout;
		selTimeout.tv_usec = 0;

		if (select(0, &sockSet, NULL, NULL, &selTimeout) == 0) {
			printf("No echo reqeusts for %ld secs... Server still alive\n", timeout);
		}
		else {
			for (port = 0; port < noPorts; port++)
			{
				if (FD_ISSET(servSock[port], &sockSet))
				{
					printf("Request on prot %d (cmd-line position): ", port);
					HandleTCPClient(AcceptTCPConnection(servSock[port]));
				}
			}
		}
	}

	for (port = 0; port < noPorts; port++)
	{
		closesocket(servSock[port]);
	}

	free(servSock);
	WSACleanup();

	exit(0);
}
