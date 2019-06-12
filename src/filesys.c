
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/filesys.h"
#include "../include/t2fs.h"



DWORD convertToDword(unsigned char* buffer) {
    return (DWORD) ((DWORD)buffer[0] | (DWORD)buffer[1] << 8 |(DWORD)buffer[2] << 16 |(DWORD)buffer[3] << 24 );
}

WORD convertToWord(unsigned char* buffer) {
    return (WORD) ((WORD)buffer[0] | (WORD)buffer[1] << 8);
}

unsigned char* wordToLtlEnd(WORD entry) {
    unsigned char* buffer = malloc(sizeof(unsigned char)*2);

    buffer[0] = entry;
    buffer[1] = entry >> 8;

    return buffer;
}

unsigned char* dwordToLtlEnd(DWORD entry) {
    unsigned char* buffer = malloc(sizeof(unsigned char)*4);

    buffer[0] = entry;
    buffer[1] = entry >> 8;
    buffer[2] = entry >> 16;
    buffer[3] = entry >> 24;

    return buffer;
}

int FATinit () {

  int i;
  BYTE buffer[SECTOR_SIZE];	// buffer para leitura do setor


  // Lẽ o MBR, retorna erro se não conseguir
  if (read_sector(0, buffer) != 0) {
    return -1;
  };

  superblock.version = buffer[0];
  superblock.clusterSize = SECTOR_SIZE * sectors_per_block;
  // montar o superbloco aqui

  printf("%d\n", superblock.version);

  // Inicialização do vetor de arquivos abertos
  for (i = 0; i < 10; i++) {
            openFiles[i].file = -1;
            openFiles[i].currPointer = -1;
            openFiles[i].clusterNo = -1;
            openDirectories[i].handle = -1;
            openDirectories[i].noReads = -1;
            openDirectories[i].clusterDir = -1;
          //  openDirectories[i].directory = setNullDirent(); // falta definir esta função
    }

  currentPath.absolute = malloc(sizeof(char)*5); // Valor inicial arbitrario
    strcpy(currentPath.absolute, "/");
    currentPath.clusterNo = 5; 					// Ainda não definido, numero puramente cabalistico sem significado

}

int FATformat (int sectors_per_block) {



      FATinit();

      




}
