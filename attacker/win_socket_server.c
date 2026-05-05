#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define CHUNK_SIZE 1024*256

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Winsock startup failed.\n");
        return 1;
    }

    SOCKET server_socket;
    SOCKET client_socket;
    int port = 32145;
    struct sockaddr_in server_address;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Error creating socket.\n");
        WSACleanup();
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("192.168.1.9");

    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    listen(server_socket, 10);
    printf("Waiting for connection...\n");

    client_socket = accept(server_socket, NULL, NULL);

    DWORD timeout = 7000;
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        printf("setsockopt failed\n");
    }

    while (1) {
        char fileName[260] = {0};
        unsigned long long fileSize = 0;

        int n = recv(client_socket, fileName, 260, 0);
        if (n <= 0) break;

        recv(client_socket, (char*)&fileSize, 8, 0);
        
        printf("Client has sent: %s - %llu bytes\n", fileName, fileSize);

        FILE * fptr = fopen(fileName, "wb");
        if (fptr == NULL) {
            printf("Cannot create file!\n");
            continue;
        }

        char *buffer = (char*)malloc(CHUNK_SIZE);
        unsigned long long total_received = 0;
        int received_bytes = 0;

        while (total_received < fileSize) {
            unsigned long long remaining = fileSize - total_received;
            int to_read = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : (int)remaining;
            
            received_bytes = recv(client_socket, buffer, to_read, 0);
            if (received_bytes <= 0) break;

            fwrite(buffer, 1, received_bytes, fptr);
            total_received += received_bytes;
        }

        free(buffer);
        fclose(fptr);
        printf("File received successfully.\n");
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}