#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>

#define CHUNK_SIZE 1024*256

int main() {
    int server_socket;
    int client_socket;
    int port = 32145;
    struct sockaddr_in server_address;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("172.29.71.202");

    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    listen(server_socket, 10);

    client_socket = accept(server_socket, NULL, NULL);

    struct timeval timeout;
    timeout.tv_sec = 7;
    timeout.tv_usec = 0;

    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
    }
    while (1) {
        char fileName[260] = {};
        unsigned long long fileSize = 0;
        recv(client_socket, fileName, 260, 0);
        recv(client_socket, (char*)&fileSize, 8, 0);
        printf("Client has sent: %s - %llu\n", fileName, fileSize);
        FILE * fptr = fopen(fileName, "wb");
        char *buffer = (char*)malloc(CHUNK_SIZE);
        unsigned long long total_received = 0;
        int received_bytes = 0;
        while (total_received < fileSize) {
            unsigned long long remaining = fileSize - total_received;
            int to_read = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
            received_bytes = recv(client_socket, buffer, to_read, 0);
            if (received_bytes <= 0) break;
            fwrite(buffer, 1, received_bytes, fptr);
            total_received += received_bytes;
        }
        free(buffer);
        fclose(fptr);
    }
    close(client_socket);
    close(server_socket);
    return 0;
}
