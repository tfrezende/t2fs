
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
      superblock.RootDirCluster = 1;    // root fixo no cluster 1
      superblock.SectorsPerCluster = sectors_per_block;

      nClusters = ((superblock.pLastBlock - superblock.pFirstBlock) * SECTOR_SIZE)/superblock.clusterSize;

      FATnext = malloc(sizeof(int)*nClusters);
      FATbitmap = malloc(sizeof(char)*nClusters);

      FATnext[0] = 0;                   // Cluster 0 reservado para as informações de Próxima e Livre

      FATbitmap[0] = '5';

      for(i = 1; i < nClusters; i++) {
          FATnext[i] = -1;                  // Inicializa os vetores
          FATbitmap[i] = '0';
      }

      FATbitmap[1] = '1';               // Cluster 1 reservado para o Root

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

int changeDir(char * pathname){

    int clusterDir = 0;
    int dirFound = 0;
    int i;
    char *dirName;
    char *path;
    unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.clusterSize);
    DIRENT2 *findPath = malloc ( superblock.clusterSize );

    if(strcmp(pathname, "/") == 0){
        // Se o path contiver somente "/" vai para o root
        free(currentPath.absolute);
        currentPath.absolute = malloc(sizeof(char) * 2);
        strcpy(currentPath.absolute, pathname);
        currentPath.clusterNo = superblock.RootDirCluster;
        return 0;
    }

    if(separatePath(pathname, &path, &dirName) != 0)
        return -1;

    if(strlen(dirName) <= 0)
        return -1;

    clusterDir = pathToCluster(path);

    readCluster(clusterDir, buffer);

    printf("AGORA É PRA VALER \n\n");

    findPath = readDataClusterFolder(clusterDir);

    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ) ; i++){
        if ((strcmp(findPath[i].name, dirName) == 0 ) && findPath[i].fileType == 0x02 ){
          dirFound = 1;
          break;
        }
    }

    if(dirFound == 0)
        return -1;

    free(currentPath.absolute);
    currentPath.absolute = malloc(sizeof(char) * strlen(pathname) + 1);
    strcpy(currentPath.absolute, pathname);
    currentPath.clusterNo = findPath[i].firstCluster;

    free(buffer);

    return 0;
}

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

    memcpy(*SecondStringOutput, aux+1,lenghtAux);
    memcpy(*FirstStringOutput, path, lenghtPath-lenghtAux);
    strcat(*FirstStringOutput,"/");

    return 0;
}

DIR2 createDir (char *pathname){

      int clusterDir = 0;
      int clusterNewDir = 0;
      int i;
      int dirSpace = 0;
      char *dirName = malloc(sizeof(char) * 31);
      char *path;
      unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
      DIRENT2 newDirEnt;
      DIRENT2* freeSpaceFind = malloc ( superblock.clusterSize );


      clusterNewDir = FindFreeCluster();

      if (clusterNewDir == -1)
        return -1;

      if (strcmp(pathname,"/") == 0) {
          return -1;
      }

      separatePath(pathname, &path, &dirName);

      if(strlen(dirName) <= 0)
        return -1;

      // Adicionar dirétorio no diretorio pai

      clusterDir = pathToCluster(path);

      readCluster(clusterDir, buffer);

      freeSpaceFind = readDataClusterFolder(clusterDir);

      for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ) ; i++){
          if (strcmp(freeSpaceFind[i].name, dirName) == 0)
            return -1;
          if (strcmp(freeSpaceFind[i].name, "") == 0){
              dirSpace = i;
              break;
          }
      }

      strcpy (newDirEnt.name, dirName);
      newDirEnt.fileType = 0x02;
      newDirEnt.fileSize = 0;
      newDirEnt.firstCluster = clusterNewDir;


      memcpy(buffer, newDirEnt.name, strlen(dirName));                                                                                               // dirName
      memcpy(buffer + 31, wordToLtlEnd(newDirEnt.fileType), 1);           // fileType
      memcpy(buffer + 32, dwordToLtlEnd(newDirEnt.fileSize), 4);          // fileSize
      memcpy(buffer + 36, dwordToLtlEnd(newDirEnt.firstCluster), 4);      // firstCluster

      writeCluster(clusterDir, buffer, (dirSpace * sizeof(DIRENT2)) , sizeof(DIRENT2));

      FATbitmap[clusterNewDir] = '1';
      FATwrite();

      free(buffer);
      free(freeSpaceFind);

      return 0;
}

int deleteDir(char * pathname){

    int clusterDir = 0;
    int i;
    int dirFound = 0;
    char *dirName;
    char *path;
    unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
    unsigned char *emptyBuffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
    DIRENT2* folderFind = malloc ( superblock.clusterSize );

    memset(emptyBuffer, '\0', superblock.sectorSize * superblock.SectorsPerCluster);

    if (strcmp(pathname,"/") == 0) {
        return -1;
    }

    separatePath(pathname, &path, &dirName);

    if(strlen(dirName) <= 0)
      return -1;

    clusterDir = pathToCluster(path);

    if (FATbitmap[clusterDir] == '0')
        return -1;

    readCluster(clusterDir, buffer);

    folderFind = readDataClusterFolder(clusterDir);

    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ); i++){
        if (strcmp(folderFind[i].name, dirName) == 0){
          dirFound = i;
          break;
      }
    }

    if(!dirFound)
        return -1;

    FATbitmap[folderFind[i].firstCluster] = '0';
    FATwrite();

    writeCluster(folderFind[i].firstCluster, emptyBuffer, 0, superblock.clusterSize);       // Limpa o cluster

    writeCluster(clusterDir, emptyBuffer, (dirFound * sizeof(DIRENT2)) , sizeof(DIRENT2) ); // Limpa a entrada de diretorio

    free(buffer);
    free(emptyBuffer);


return 0;
}

int pathToCluster(char* path) {

        int i;
        int found = 0;
        int pathsNo = 0;
        int folderInPath = 1;
        int pathComplete = 0;

        unsigned int currentCluster;
        char* pathTok;
        char* pathcpy = malloc( sizeof(char) * (strlen(path)+1) );
        int folderSize = (superblock.clusterSize / sizeof(DIRENT2)) ;
        DIRENT2* folderContent = malloc(superblock.clusterSize) ;

        strcpy(pathcpy,path);

        if (pathcpy[0] == '/') {
            currentCluster = superblock.RootDirCluster;
        }else {
            currentCluster = currentPath.clusterNo;
        }

        if (strcmp(pathcpy,"/") == 0) {
            return superblock.RootDirCluster;
        }

        pathTok = strtok(pathcpy,"/");


        while(pathTok != NULL && pathsNo == found && folderInPath) {
            pathsNo += 1;
            folderContent = readDataClusterFolder(currentCluster);
            for(i = 0; i < folderSize; i++) {
                if (strcmp(folderContent[i].name,pathTok) == 0) {
                    currentCluster = folderContent[i].firstCluster;
                    found += 1;
                    if (folderContent[i].fileType != 0x02) {
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

DIRENT2* readDataClusterFolder(int clusterNo) {

        int j;
        int folderSizeInBytes = superblock.clusterSize ;
        unsigned int sector = superblock.pFirstBlock + superblock.SectorsPerCluster * clusterNo;
        unsigned char *teste = malloc( sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
        DIRENT2* folderContent = malloc(folderSizeInBytes);

        if (sector >= superblock.pFirstBlock && sector <= superblock.pLastBlock) {

            memset(teste, '\0', superblock.sectorSize * superblock.SectorsPerCluster);
            readCluster(clusterNo, teste);

            for(j = 0; j < ( folderSizeInBytes / sizeof(DIRENT2) ) ; j++) {
                memcpy(folderContent[j].name, teste + j*sizeof(DIRENT2) , 31);
                folderContent[j].fileType = (BYTE) ( *(teste + 31 + j*sizeof(DIRENT2)) );
                folderContent[j].fileSize = convertToDword(teste + 32 + j*sizeof(DIRENT2));
                folderContent[j].firstCluster = convertToDword(teste + 36 + j*sizeof(DIRENT2));
            }

            free(teste);
            return folderContent;
        }
        free(teste);
        return NULL;
}

int isFolder(char *pathname){

    char *dirName = malloc(sizeof(char) * 31);
    char *path;
    int clusterSearch;
    unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
    DIRENT2* folderFind = malloc ( superblock.clusterSize );

    separatePath(pathname, &path, &dirName);
    clusterSearch = pathToCluster(path);

    readCluster(clusterSearch, buffer);

    folderFind = readDataClusterFolder(clusterDir);

    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ); i++){
        if ( (strcmp(folderFind[i].name, dirName) == 0) && (folderFind[i].fileType = 0x02) )
          return 1;
    }

    return 0;

}

FILE2 createFile(char * filename){
    char * firstOut;
    char * secondOut;
    int firstClusterFreeInFAT;
    int handle;
    handle = makeAnewHandle();
    int clusterToRecordFile;
    char *linkOutput;
    DIRENT2 toRecord;

    separatePath(currentPath.absolute, &firstOut, &secondOut)       // A principio vai criar no caminho corrente

    clusterToRecordFile = pathToCluster(secondOut);

//caminho inexistente
    if(clusterToRecordFile == -1)
        return -1;


//Bitmap liberado
    if(FATbitmap[clusterToRecordFile] == '0')
        return -1;

//n tinha espaço para adicionar um novo arquivos
    if(handle == -1)
        return -1;


//se ja tiver um arquivo com esse nome nesse diretorio
//TODO: TEM Q APGAR O TEM E COLOCAR O NOVO.
    //if(isInCluster(clusterToRecordFile, secondOut, TYPEVAL_REGULAR)){
    /*if (sector >= superBlock.DataSectorStart && sector < superBlock.NofSectors) {
        readCluster(clusterNo, buffer);

        for(i = 0; i < clusterByteSize; i+= sizeof(struct t2fs_record)) {
            if ( (strcmp((char *)buffer+i+1, fileName) == 0) && (((BYTE) buffer[i]) == TypeValEntrada) && !wasFound ) {
                wasFound = 1;
            }
        }
        free(buffer);
        if (wasFound) {
            return 1;
        } else {
            return 0;
        }
}
        deleteFile(filename);
    }*/

//se n achar um cluster livre na fat
    firstClusterFreeInFAT = FindFreeCluster();

    if(firstClusterFreeInFAT == -1)
        return -1;

//declaração de seus atributos
    strcpy(toRecord.name, filename);
    toRecord.fileType = 0x01;
    toRecord.fileSize = 0;
    toRecord.firstCluster = firstClusterFreeInFAT;

//escrita no diretorio
    if(writeDataClusterFolder(clusterToRecordFile, toRecord) == - 1){//se n tiver espaço na folder
        return -1;
    }

//marcação na fat de cluster ocupado
    FATbitmap[firstClusterFreeInFAT] = '1';
    FATwrite();

//retorna o handle já colocando ele no array de opens
    return (openFile (filename));
}

int makeAnewHandle(){
    int i;

    for(i = 0; i < MAX_NUM_FILES; i++){
        if(openFiles[i].file == -1){
            return (i+1);
        }
    }
    //se chegou até aqui é pq n encontrou nenhuma posição no array de 10 para botar um novo arquivo
    return -1;
}
