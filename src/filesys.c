
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

      FATread();

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

      FATnext[0] = 0;                 // Cluster 0 reservado para as informações de Próxima e Livre
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
DIRENT2 setNullDirent(){

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
    unsigned char* newBuffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);

    readCluster(clusterNo, newBuffer);

    for(j = position; j < size + position; j++){
        newBuffer[j] = buffer[j - position];
    }

    for(sectorToWrite = sector; sectorToWrite < (sector + superblock.SectorsPerCluster); sectorToWrite++) {
          write_sector(sectorToWrite, newBuffer + k);
          k += 256;
    }

    free(newBuffer);
    return 0;
}

int FATwrite(){

    unsigned char *buffer = malloc(4*sizeof(int));
    int i, k;

    for(i = 0, k = 0; i < nClusters; i++ , k += 4){
      memcpy(buffer, dwordToLtlEnd(FATnext[i]), 4);
      writeCluster(0, buffer, k, 4);                                      // Coloca um byte por iteração do k
    }

    writeCluster(0, FATbitmap, k, sizeof(unsigned char) * nClusters);     // Coloca o bitmap depois da FAT

    free(buffer);

    return 0;
}

int FATread (){

  unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
  int i, k;

  readCluster(0, buffer);

  for(i = 0, k = 0; i < nClusters ; i++, k += 4)
      FATnext[i] = convertToDword(buffer + k);
                                                                              // Leitura tranquila

  for(i = 0; i < nClusters; i++, k++){
      FATbitmap[i] = buffer[k];
  }

  free(buffer);

  return 0;

}

int FindFreeCluster (){
    int i;
    int freeCluster = 0;

    for(i = 1; i < nClusters; i++){
        if (FATbitmap[i] == '0'){
            freeCluster = i;
            break;
        }
      }

    if (freeCluster == 0)
      return -1;
    else
      return freeCluster;

}

int changeDir(char * path){

    char * absolute;
    char * firstOut;
    char * secondOut;

    if(strlen(path) == 0){
        //Se a string for vazia não altera o lugar
        return 0;
    }

    if(strcmp(path, "/") == 0){
        // Se o path contiver somente "/" vai para o root
        free(currentPath.absolute);
        currentPath = malloc(sizeof(char) * 2);
        strcpy(currentPath.absolute, path);
        currentPath.clusterNo = superblock.RootDirCluster;
    }

    if (separatePath(absolute, &firstOut, &secondOut) != 0)
        return -1;



    currentPath.clusterNo = descriptor.clusterDir;
    //currentPath.absolute = absolute;

    return 0;
}
// Função que separa o último nome de um path
int separatePath(char * path, char ** FirstStringOutput, char ** SecondStringOutput) {
    const char dir_div = '/';
    int lenghtAux;
    int lenghtPath = strlen(path);
    char *aux =  malloc(lenghtPath);
    //Nunca vão ter um tamanho maior que o path
    *SecondStringOutput = malloc(lenghtPath);
    memset(*SecondStringOutput,'\0',lenghtPath);
    *FirstStringOutput = malloc(lenghtPath);
    memset(*FirstStringOutput,'\0',lenghtPath);

    aux = strrchr(path, dir_div);
    lenghtAux = strlen(aux);
    memcpy(*SecondStringOutput,aux+1,lenghtAux);
    memcpy(*FirstStringOutput, path, lenghtPath-lenghtAux);
    strcat(*FirstStringOutput,"/");
    return 0;
}

DIR2 CreateDir (char *pathname){
  int clusterDir = 0;
  int clusterNewDir = 0;
  int dirSpace = 0;
  char *dirName = malloc(sizeof(char) * MAX_FILE_NAME_SIZE);
  char *path = malloc(sizeof(char) * MAX_FILE_NAME_SIZE * 8)   // 8 é um numero arbitrario
  unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
  DIRENT2 newDirEnt;

  clusterNewDir = FindFreeCluster();

  if (clusterNewDir == -1)
    return -1;

  separatePath(pathname, *dirName, *path);

  // Adicionar dirétorio no diretorio parti

  cluster = pathToCluster(path)

  readCluster(cluster, buffer)



  strcpy (newDirEnt.name, dirName);
  newDirEnt.fileType = 0x02;
  newDirEnt.fileSize = 0;
  newDIrEnt.firstCluster = cluster;






  //  Livre cluster





}

// VAMOS TER QUE MODIFICAR AS ESTRUTURAS PARA UTILIZAR AS TRÊS FUNÇÕES ABAIXO

/*
int link(char * path, char ** output) {
    int i;
    int isLink = 0;
    unsigned char* buffer = malloc(superblock.clusterSize;);
    char * absolute;
    char * pathToFile;
    char * fileName;
    int pathClusterNo;
    int linkClusterNo;


    //printf("\nAboslute: %s", absolute);
    separatePath(absolute, &pathToFile, &fileName);

    pathClusterNo = pathToCluster(pathToFile);

    if(pathClusterNo == -1) {
        free(buffer);
        free(absolute);
        free(fileName);
        free(pathToFile);
        return -1;
    }

    readCluster(pathClusterNo, buffer);

    // printf("\nfileName: %s", fileName);
    for(i = 0; i < superblock.clusterSize;; i+= sizeof(struct t2fs_record)) {
        // printf("\nNumero de vezes do for %d\n", i);
        if ( (strcmp((char *)buffer+i+1, fileName) == 0) && (((BYTE) buffer[i]) == TYPEVAL_LINK) && !isLink ) {
            isLink = 1;
        }
    }

    if(!isLink) {
        free(buffer);
        free(absolute);
        free(fileName);
        free(pathToFile);
        *output = malloc(sizeof(char)*(strlen(path)+1));
        strcpy(*output,path);
        return 0;//BOTEI RETORNO COMO 0 PARA NAO SOFTLINK E COMO 1 PARA SOFTLINK
    }

    linkClusterNo = pathToCluster(path);

    memset(buffer,0,superblock.clusterSize;);

    readCluster(linkClusterNo,buffer);

    *output = malloc(sizeof(char)*(strlen((char*)buffer)+1));
    strcpy(*output,(char*)buffer);

    free(buffer);
    free(absolute);
    free(fileName);
    free(pathToFile);
    //BOTEI RETORNO COMO 0 PARA NAO SOFTLINK E COMO 1 PARA SOFTLINK
    return 1;
}

int pathToCluster(char* path) {
    int i;
    int found = 0;
    int pathsNo = 0;
    int folderInPath = 1;
    int pathComplete = 0;
    unsigned int currentCluster;
    char* pathTok;
    char* pathcpy = malloc(sizeof(char)*(strlen(path)+1));
    int folderSize = ( superblock.clusterSize / sizeof(struct t2fs_record) );
    struct t2fs_record* folderContent = malloc(sizeof(struct t2fs_record)*( (superblock.clusterSize) / sizeof(struct t2fs_record) ));

    strcpy(pathcpy,path);

    if (pathcpy[0] == '/') {
        currentCluster = superBlock.RootDirCluster;
    }else {
        currentCluster = currentPath.clusterNo;
    }

    if (strcmp(pathcpy,"/") == 0) {
        return superBlock.RootDirCluster;
    }

    pathTok = strtok(pathcpy,"/");

    while(pathTok != NULL && pathsNo == found && folderInPath) {
        pathsNo += 1;
        folderContent = readDataClusterFolder(currentCluster);
        for(i = 0; i < folderSize; i++) {
            if (strcmp(folderContent[i].name,pathTok) == 0) {
                currentCluster = folderContent[i].firstCluster;
                found += 1;
                if (folderContent[i].TypeVal != TYPEVAL_DIRETORIO) {
                    folderInPath = 0;
                }
            }
        }
        pathTok = strtok(NULL,"/");
        if (pathTok == NULL) {
            pathComplete = 1;
        }
    }

    if (pathsNo != found) {
        free(pathcpy);
        free(folderContent);
        return -1;
    }

    if (!pathComplete) {
        free(pathcpy);
        free(folderContent);
        return -1;
    }
    free(pathcpy);
    free(folderContent);
    return currentCluster;
}

struct t2fs_record* readDataClusterFolder(int clusterNo) {
    int j;
    int folderSizeInBytes = sizeof(struct t2fs_record)*( (SECTOR_SIZE*superBlock.SectorsPerCluster) / sizeof(struct t2fs_record) );
    unsigned int sector = superBlock.DataSectorStart + superBlock.SectorsPerCluster*clusterNo;
    unsigned char* buffer = malloc(sizeof(unsigned char)*SECTOR_SIZE*superBlock.SectorsPerCluster);
    struct t2fs_record* folderContent = malloc(folderSizeInBytes);

    if (sector >= superBlock.DataSectorStart && sector < superBlock.NofSectors) {
        readCluster(clusterNo, buffer);

        for(j = 0; j < folderSizeInBytes/sizeof(struct t2fs_record); j++) {
            folderContent[j].TypeVal = (BYTE) *(buffer + sizeof(struct t2fs_record)*j);
            memcpy(folderContent[j].name, buffer + 1 + sizeof(struct t2fs_record)*j, 51);
            folderContent[j].bytesFileSize = convertToDword(buffer + 52 + sizeof(struct t2fs_record)*j);
            folderContent[j].clustersFileSize = convertToDword(buffer + 56 + sizeof(struct t2fs_record)*j);
            folderContent[j].firstCluster = convertToDword(buffer + 60 + sizeof(struct t2fs_record)*j);
        }
        free(buffer);
        return folderContent;
    }
    free(buffer);
    return NULL;
}*/
