#ifndef CRYPTO_H
#define CRYPTO_H

#include <windows.h>
#include <wincrypt.h>

void InitializeSymmetricCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hKey, BYTE **keyBlob);
void FileEncrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey);
void HexToBytes(const char *hex, BYTE* bytes);
HCRYPTKEY ImportKey(char *hexStr, HCRYPTPROV hProv);
void FileDecrypt(char *path, HCRYPTPROV hProv, HCRYPTKEY hKey);
void FinalizeSymmetricCrypto(HCRYPTPROV *hProv, HCRYPTKEY *hKey, BYTE **keyBlob);

#endif