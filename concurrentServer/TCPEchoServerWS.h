#include <stdio.h>
#include <winsock.h>
#include <stdlib.h>

void DieWithError(char *errorMessage);
void HandleTCPClient(int clntSocket);
int CreateTCPServerSocket(unsigned short port);
int AcceptTCPConnection(int servSock);
