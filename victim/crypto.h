#ifndef CRYPTO_H
#define CRYPTO_H

#include <windows.h>
#include <wincrypt.h>

void InitializeCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hAesKey, HCRYPTKEY *hRsaKey, BYTE **aesKeyBlob);
void SaveToRegistry(char *valueName, BYTE *data, DWORD dataLen);
void KeyEncrypt(BYTE **key, HCRYPTKEY hKey);
void FileEncrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey);
void HexToBytes(const char *hex, BYTE* bytes);
char* GetRSAKey(char *id);
HCRYPTKEY ImportRSAPrivateKey(char *hexStr, HCRYPTPROV hProv);
HCRYPTKEY ImportRSAPublicKey(char *hexStr, HCRYPTPROV hProv);
HCRYPTKEY ImportAESKeyFromBlob(BYTE *blob, DWORD blobLen, HCRYPTPROV hProv);
HCRYPTKEY ImportAESKey(char *hexStr, HCRYPTPROV hProv);
void FileDecrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey);
BYTE* LoadFromRegistry(char *valueName, DWORD *dataLen);
void KeyDecrypt(BYTE **key, HCRYPTKEY hKey);
void FinalizeCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hAesKey, HCRYPTKEY *hRsaKey, BYTE **keyBlob);

#endif