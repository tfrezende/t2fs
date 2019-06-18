#ifndef _FILESYS_
#define _FILESYS_

#include "t2fs.h"

DWORD convertToDword(unsigned char* buffer);

WORD convertToWord(unsigned char* buffer);

unsigned char* wordToLtlEnd(WORD entry);

unsigned char* dwordToLtlEnd(DWORD entry);

DIRENT2 setNullDirent();

int FATinit();

int FATwrite();

int FATread();

int FATformat (int sectors_per_block);

int *FATnext;

unsigned char *FATbitmap;

int nClusters;

int readCluster(int clusterNo, unsigned char* buffer);

int writeCluster(int clusterNo, unsigned char* buffer, int position, int size);

int FindFreeCluster ();

int changeDir(char * pathname);

int separatePath(char * path, char ** FirstStringOutput, char ** SecondStringOutput);

DIRENT2* readDataClusterFolder(int clusterNo);

int pathToCluster(char* path);

DIR2 createDir (char *pathname);

int deleteDir(char *pathname);

int delete(int clusterDir, int fileType);

int isFolder(char *pathname);

int makeAnewHandle();

int closeFile(FILE2 handle);

int closeDir(DIR2 handle);

//int deleteFile(char * filename);

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
} CURRENT_PATH;

CURRENT_PATH currentPath;

typedef struct diskd {
    DIR2 handle;
    int noReads;
    int clusterDir;
    DIRENT2 directory;
} DISK_DIR;

DISK_DIR openDirectories[10]; // vetor global de diretórios abertos



#endif
