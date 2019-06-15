
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/filesys.h"
#include "../include/t2fs.h"

//      LEMBRAR QUE VAMOS USAR A FAT32 !!!!

int init_FAT = 0;               // 0 se a FAT não foi inicializada, 1 se sim

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

int FATinit () {            // Pode ser que o arquivo já esteja formatado, só iniciar as structs auxiliares e ler a FAT

  int i;

  if (init_FAT == 0) {

      // Inicialização do vetor de arquivos abertos
      for (i = 0; i < 10; i++) {
                openFiles[i].file = -1;
                openFiles[i].currPointer = -1;
                openFiles[i].clusterNo = -1;
                openDirectories[i].handle = -1;
                openDirectories[i].noReads = -1;
                openDirectories[i].clusterDir = -1;
                openDirectories[i].directory = setNullDirent(); // falta definir esta função
        }

      currentPath.absolute = malloc(sizeof(char)*5); // Valor inicial arbitrario
      strcpy(currentPath.absolute, "/");
      currentPath.clusterNo = 1;  // Caminho absoluto fixado no cluster 1
      init_FAT = 1;

  }

  return 0;

}

int FATformat (int sectors_per_block) {       // Quem lê o MBR, apaga tudo e faz a FAT

      BYTE buffer[SECTOR_SIZE];	// buffer para leitura do setor
      int i;

      // Lẽ o MBR, retorna erro se não conseguir
      if (read_sector(0, buffer) != 0) {
        return -1;
      };

      superblock.version = convertToWord(buffer);
      superblock.sectorSize = convertToWord(buffer + 2);
      superblock.partTable = convertToWord(buffer + 4);     // byte inicial da tabela de partições
      superblock.numPartitions = convertToWord(buffer + 6); // número de partições no disco
      superblock.pFirstBlock = convertToDword(buffer + 8);  // endereço do primeiro bloco da partição
      superblock.pLastBlock = convertToDword(buffer + 12);   // endereço do último bloco da partição
      memcpy(superblock.partName, buffer + 16, 8);
      superblock.clusterSize = SECTOR_SIZE * sectors_per_block;
      superblock.RootDirCluster = 1;    // root fixo no setor 0
      superblock.SectorsPerCluster = sectors_per_block;

      nClusters = ((superblock.pLastBlock - superblock.pFirstBlock) * SECTOR_SIZE)/superblock.clusterSize;

      FATnext = malloc(sizeof(int)*nClusters);
      FATbitmap = malloc(sizeof(char)*nClusters);

      FATnext[0] = 5;                 // Cluster 0 reservado para as informações de Próxima e Livre
      FATbitmap[0] = '5';

      for(i = 1; i < nClusters; i++) {
          FATnext[i] = -1;                  // Inicializa os vetores
          FATbitmap[i] = '0';
      }

      FATwrite();

      FATinit();

      return 0;
}

// Função para apagar um diretório
DIRENT2 setNullDirent()
{
    DIRENT2 dir;
    strcpy(dir.name,"");
    dir.fileType = (DWORD)6; // valor inválido, lixo
    dir.fileSize = (DWORD)0;

    return dir;
}

int readCluster(int clusterNo, unsigned char* buffer) {
    int i = 0;
    unsigned int sectorToRead;
    unsigned int sector = superblock.pFirstBlock + superblock.SectorsPerCluster*clusterNo;

    for(sectorToRead = sector; sectorToRead < (sector + superblock.SectorsPerCluster); sectorToRead++) {
        printf("Setor : %d\n", sectorToRead);
        read_sector(sectorToRead,buffer + i);
        i += superblock.sectorSize;
    }
    return 0;
}

int writeCluster(int clusterNo, unsigned char* buffer, int position, int size) {
    int j;
    int k = 0;
    unsigned int sectorToWrite;
    unsigned int sector = superblock.pFirstBlock + superblock.SectorsPerCluster*clusterNo;
    unsigned char *newBuffer = malloc(superblock.sectorSize * superblock.SectorsPerCluster);

    readCluster(clusterNo, newBuffer);

    for(j = position; j < size + position; j++){
        newBuffer[j] = buffer[j - position];
    }

    for(sectorToWrite = sector; sectorToWrite < (sector + superblock.SectorsPerCluster); sectorToWrite++) {
          write_sector(sectorToWrite, buffer + k);
          k += 256;
    }

    free(newBuffer);
    return 0;
}

int FATwrite(){

    int  i , k;

    unsigned char *buffer = malloc(4*sizeof(FATnext) + sizeof(FATbitmap));
    strcpy(buffer, "");


    for(i = 0, k = 0; i < nClusters; i++, k += 4){
        strcpy((buffer + k),dwordToLtlEnd(FATnext[i]));
        puts(buffer);
    }

    strcat(buffer, FATbitmap);

    writeCluster(0, buffer, 0, superblock.clusterSize);

//    free(buffer);

}

/*  FUNÇÃO QUE CONSEGUE TRANNSFORMAR UM VETOR DE INT EM CHAR SEM PROBLEMAS
unsigned char buffer[500];

unsigned char** makeStrArr(const int* vals, const int nelems)
{
    unsigned char** strarr = (unsigned char**)malloc(sizeof(unsigned char*) * nelems);
    int i;
    unsigned char buf[128];

    for (i = 0; i < nelems; i++)
    {
        strarr[i] = (unsigned char*)malloc(sprintf(buf, "%d", vals[i]) + 1);
        strcpy(strarr[i], buf);
    }
    return strarr;
}

void freeStrArr(unsigned char** strarr, int nelems)
{
    int i = 0;
    for (i = 0; i < nelems; i++) {
        free(strarr[i]);
    }
    free(strarr);
}

void iarrtostrarrinc()
{
    strcpy(buffer, "");
    int i_array[] = { 0, 1, 1, 1, 0 };
    unsigned char** strarr = makeStrArr(i_array, 5);
    int i;
    for (i = 0; i < 5; i++) {
        strcat(buffer, strarr[i]);
        puts(buffer);
    }

    //strcat(buffer, bitmap);
    puts(buffer);
    freeStrArr(strarr, 5);
}

void main(){
    int i = 0, j = 0;
    unsigned char bitmap[15] = "011100000000000";

    iarrtostrarrinc();
    strcat(buffer, bitmap);
    puts(buffer);
}
*/
