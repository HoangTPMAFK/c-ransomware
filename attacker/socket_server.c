#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int server_socket;
    int client_socket;
    int port = 32154;
    struct sockaddr_in server_address;
    char buffer[100] = "Hello from server!\n";
    char recv_buffer[100];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("192.168.1.22");

    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    listen(server_socket, 10);

    client_socket = accept(server_socket, NULL, NULL);

    send(client_socket, buffer, sizeof(buffer), 0);

    recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
    printf("Client has sent: %s\n", recv_buffer);

    close(client_socket);
    close(server_socket);
    return 0;
}