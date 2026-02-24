#ifndef CRYPTO_H
#define CRYPTO_H

#include <windows.h>
#include <wincrypt.h>

void InitializeCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hAesKey, HCRYPTKEY *hRsaKey, BYTE **aesKeyBlob);
void KeyEncrypt(BYTE **key, HCRYPTKEY hKey);
void FileEncrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey);
void HexToBytes(const char *hex, BYTE* bytes);
HCRYPTKEY ImportRSAPrivateKey(char *hexStr, HCRYPTPROV hProv);
HCRYPTKEY ImportRSAPublicKey(char *hexStr, HCRYPTPROV hProv);
HCRYPTKEY ImportAESKey(char *hexStr, HCRYPTPROV hProv);
void FileDecrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey);
void KeyDecrypt(BYTE **key, HCRYPTKEY hKey);
void FinalizeCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hAesKey, HCRYPTKEY *hRsaKey, BYTE **keyBlob);

#endif