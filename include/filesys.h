#ifndef _FILESYS_
#define _FILESYS_

#include "t2fs.h"

DWORD convertToDword(unsigned char* buffer);

WORD convertToWord(unsigned char* buffer);

unsigned char* wordToLtlEnd(WORD entry);

unsigned char* dwordToLtlEnd(DWORD entry);

typedef struct diskf {
    FILE2 file;
    int currPointer;
    int clusterNo;
    int clusterDir;
} DISK_FILE;

DISK_FILE openFiles[10];

typedef struct currp {
    char* absolute;
    int clusterNo;
} CURRENT_PATH;                         // OBSERVAR O USO DESSAS ESTRUTURAS E POSSIVEIS UTILIZAÇÕES

CURRENT_PATH currentPath;

typedef struct diskd {
    DIR2 handle;
    int noReads;
    int clusterDir;
    DIRENT2 directory;
} DISK_DIR;

DISK_DIR openDirectories[10]; // vetor global de diretórios abertos



#endif
