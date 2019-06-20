
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
    char *linkOutput;
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
    if(isLink(pathname, &linkOutput))
        pathname = linkOutput;

    if(separatePath(pathname, &path, &dirName) != 0)
        return -1;

    if(strlen(dirName) <= 0)
        return -1;

    clusterDir = pathToCluster(path);

    readCluster(clusterDir, buffer);

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
      char *dirName;
      char *path;
      DIRENT2 newDir;

      clusterNewDir = FindFreeCluster();

      if (clusterNewDir == -1)
        return -1;

      if (strcmp(pathname,"/") == 0) {
          return -1;
      }

      separatePath(pathname, &path, &dirName);

      if(strlen(dirName) <= 0)
        return -1;

      clusterDir = pathToCluster(path);

      strcpy(newDir.name, "");
      strcpy(newDir.name, dirName);
      newDir.fileType = 0x02;
      newDir.fileSize = 0;
      newDir.firstCluster = clusterNewDir;

      createEnt (clusterDir, newDir);

      FATbitmap[clusterNewDir] = '1';
      FATwrite();

      return 0;
}

int deleteEnt(int clusterDir, DIRENT2 record){

    int i, j;
    int found = -1;
    DIRENT2* folderFind = malloc ( superblock.clusterSize );
    unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
    unsigned char *emptyBuffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);

    memset(emptyBuffer, '\0', superblock.sectorSize * superblock.SectorsPerCluster);


    if(record.fileType < 0x01 || record.fileType > 0x03)
        return -1;

    readCluster(clusterDir, buffer);

    folderFind = readDataClusterFolder(clusterDir);


    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ); i++){
        if ((strcmp(folderFind[i].name, record.name) == 0) && (folderFind[i].fileType == record.fileType)){
          found = i;
          break;
      }
    }

    if(found == -1){
        free(buffer);
        free(emptyBuffer);
        free(folderFind);
        return -1;
    }



    if(record.fileType != 0x03){
        FATbitmap[folderFind[i].firstCluster] = '0';

        if (record.fileType == 0x01){

            for(i = 0; i < 10; i++){

                if(openFiles[i].clusterDir == clusterDir){ //então tava aberto
                    openFiles[i].file = -1;
                    openFiles[i].clusterNo = -1;
                    openFiles[i].currPointer = -1;
                    openFiles[i].clusterDir = -1;
                }
            }

            j = record.firstCluster;

            writeCluster(j, emptyBuffer, 0, superblock.clusterSize);

            // Apagando todos os clusters referenciados
            while(FATnext[j] != -1){

                j = FATnext[j];
                writeCluster(j, emptyBuffer, 0, superblock.clusterSize);
                FATnext[j] = -1;
                FATbitmap[j] = '0';
            }
        }


        if (record.fileType == 0x02){

            for(i = 0; i < 10; i++){
                if(openDirectories[i].clusterDir == clusterDir){
                    openDirectories[i].handle = -1;
                    openDirectories[i].noReads = -1;
                    openDirectories[i].clusterDir = -1;
                    openDirectories[i].directory = setNullDirent();

                    return 0;
                }
            }

            writeCluster(folderFind[i].firstCluster, emptyBuffer, 0, superblock.clusterSize);
        }

    }

    // limpa a entrada, todo tipo de arquivo faz isso
    writeCluster(clusterDir, emptyBuffer, (found * sizeof(DIRENT2)) , sizeof(DIRENT2) );
    FATwrite();

    free(buffer);
    free(emptyBuffer);
    free(folderFind);

    return 0;
}

int deleteDir(char * pathname){

    DIRENT2 record;
    int clusterDir = 0;
    char *dirName;
    char *path;

    if (strcmp(pathname,"/") == 0) {
        return -1;
    }

    separatePath(pathname, &path, &dirName);

    if(strlen(dirName) <= 0)
      return -1;

    clusterDir = pathToCluster(path);

    if (FATbitmap[clusterDir] == '0')
        return -1;

    strcpy(record.name, dirName);
    record.fileType = 0x02;

    if(deleteEnt(clusterDir, record) == -1)
        return -1;

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

int createEnt (int clusterDir, DIRENT2 newDirEnt){

    int i;
    int dirSpace = -1;
    unsigned char *buffer = malloc(sizeof(unsigned char) * superblock.sectorSize * superblock.SectorsPerCluster);
    DIRENT2* freeSpaceFind = malloc ( superblock.clusterSize );

    readCluster(clusterDir, buffer);

    freeSpaceFind = readDataClusterFolder(clusterDir);

    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ) ; i++){
        if (strcmp(freeSpaceFind[i].name, "") == 0){
            dirSpace = i;
            break;
        }
    }

    printf("Dir : %d\n", dirSpace);

    if(dirSpace == -1){
        free(buffer);
        free(freeSpaceFind);
        return -1;
    }

    memset(buffer,'\0', superblock.sectorSize * superblock.SectorsPerCluster);

    memcpy(buffer, newDirEnt.name, strlen(newDirEnt.name));             // dirName
    memcpy(buffer + 31, wordToLtlEnd(newDirEnt.fileType), 1);           // fileType
    memcpy(buffer + 32, dwordToLtlEnd(newDirEnt.fileSize), 4);          // fileSize
    memcpy(buffer + 36, dwordToLtlEnd(newDirEnt.firstCluster), 4);      // firstCluster



    writeCluster(clusterDir, buffer, (dirSpace * sizeof(DIRENT2)) , sizeof(DIRENT2));


    free(buffer);
    free(freeSpaceFind);
    return 0;
}

FILE2 createFile(char * filename){

    char *path;
    char *name = malloc(strlen(filename) + 1);
    const char dir_div = '/';

    int firstClusterFreeInFAT;
    int handle = makeAnewHandle();
    int clusterToRecordFile;
    DIRENT2 toRecord;

    memset(&toRecord,'\0', sizeof(DIRENT2));


    if(strrchr(filename, dir_div) == NULL){
        clusterToRecordFile = pathToCluster(currentPath.absolute);
        strcpy(name, filename);
    }
    else{
        separatePath(filename, &path, &name);
        clusterToRecordFile = pathToCluster(path);
    }



//caminho inexistente
    if(clusterToRecordFile == -1)
        return -1;

//Bitmap liberado
    if(FATbitmap[clusterToRecordFile] == '0')
        return -1;

//n tinha espaço para adicionar um novo arquivos
    if(handle == -1)
        return -1;


//se n achar um cluster livre na fat
    firstClusterFreeInFAT = FindFreeCluster();

    if(firstClusterFreeInFAT == -1)
        return -1;

//declaração de seus atributos
    strcpy(toRecord.name, "");
    strcpy(toRecord.name, name);
    toRecord.fileType = 0x01;
    toRecord.fileSize = 0;
    toRecord.firstCluster = firstClusterFreeInFAT;

    //se ja tiver um arquivo com esse nome nesse diretorio

    if(isInCluster(clusterToRecordFile, toRecord))
        deleteFile(name);


//escrita no diretorio
    if(createEnt(clusterToRecordFile, toRecord) == - 1){       //se n tiver espaço na folder
        return -1;
    }

//marcação na fat de cluster ocupado
    FATbitmap[firstClusterFreeInFAT] = '1';
    FATwrite();

    free(name);

//retorna o handle já colocando ele no array de opens         openFile (filename)
    return (5);
}

int makeAnewHandle(){
    int i;

    for(i = 0; i < 10; i++){
        if(openFiles[i].file == -1){
            return (i+1);
        }
    }
    //se chegou até aqui é pq n encontrou nenhuma posição no array de 10 para botar um novo arquivo
    return -1;
}

int deleteFile(char * filename){
    char * path;
    char * name;
    int clusterToDelete;    //cluster que tem q apagar
    DIRENT2 toDelete;
    const char dir_div = '/';

    if(strrchr(filename, dir_div) == NULL){
        clusterToDelete = pathToCluster(currentPath.absolute);
        strcpy(name, filename);
    }
    else{
        separatePath(filename, &path, &name);
        clusterToDelete = pathToCluster(path);
    }

    if (FATbitmap[clusterToDelete] == '0')
        return -1;

    strcpy(toDelete.name, name);
    toDelete.fileType = 0x01;

    if(deleteEnt(clusterToDelete, toDelete) == -1)
        return -1;


    return 0;
}

int isInCluster(int clusterNo, DIRENT2 toFind) {

    int i;
    DIRENT2* fileIn = malloc ( superblock.clusterSize );

    fileIn = readDataClusterFolder(clusterNo);

    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ) ; i++){
        if ( (strcmp(fileIn[i].name, toFind.name) == 0) && (fileIn[i].fileType == 0x02) )
            return 1;
    }
    return 0;
}

/*int isLink (char *filename, char ** output){

    char * path;
    char * name;
    int i;
    int clusterLink;
    const char dir_div = '/';
    DIRENT2* linkIn = malloc ( superblock.clusterSize );

    if(strrchr(filename, dir_div) == NULL){
        clusterLink = pathToCluster(currentPath.absolute);
        strcpy(name, filename);
    }
    else{
        separatePath(filename, &path, &name);
        clusterLink = pathToCluster(path);
    }

    linkIn = readDataClusterFolder(clusterLink);

    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ) ; i++){
        if ( (strcmp(linkIn[i].name, name) == 0) && (linkIn[i].fileType == 0x03) )
            return clusterLink;
    }

    return 0;
}
*/

int closeFile(FILE2 handle){

    int i;

    for(i = 0; i < 10; i++){
        if(openFiles[i].file == handle){
            openFiles[i].file = -1;
            openFiles[i].currPointer = -1;
            openFiles[i].clusterNo = -1;
            openFiles[i].clusterDir = -1;

            return 0;
        }
    }

    return -1;
}

DIR2 openDir(char *pathname){
    int i;
    int dirCluster;
    char *linkOutput;

    if(strcmp(pathname, "/") == 0)
        dirCluster = superblock.RootDirCluster;

    if(isLink(pathname, &linkOutput))
        pathname = linkOutput;

    dirCluster = pathToCluster(pathname);

    for(i = 0; i < 10; i++){
        if(openDirectories[i].handle == -1){
            openDirectories[i].handle = i;
            openDirectories[i].noReads = 0;
            openDirectories[i].clusterDir = dirCluster;

            return openDirectories[i].handle;
        }
    }
    return -1;
}

DIRENT2 readDir(DIR2 handle){

    int i;
    DIRENT2* folderContent = malloc(superblock.clusterSize);
    int folderSize = superblock.clusterSize / sizeof(DIRENT2);

    for(i = 0;i < 10;i++){

       if(openDirectories[i].handle == handle){
           printf("Read: %d\n",openDirectories[i].clusterDir);
           folderContent = readDataClusterFolder(openDirectories[i].clusterDir);

           if(openDirectories[i].noReads < folderSize){
               openDirectories[i].directory.fileSize = folderContent[openDirectories[i].noReads].fileSize;
               openDirectories[i].directory.fileType = folderContent[openDirectories[i].noReads].fileType;
                openDirectories[i].directory.firstCluster = folderContent[openDirectories[i].noReads].firstCluster;
               strcpy(openDirectories[i].directory.name, folderContent[openDirectories[i].noReads].name);
               openDirectories[i].noReads++;

               free(folderContent);
               return openDirectories[i].directory;
           }
       }
    }
    return setNullDirent();
}



int closeDir(DIR2 handle){

    int i;

    for(i = 0; i < 10; i++){
        if(openDirectories[i].handle == handle){
            openDirectories[i].handle = -1;
            openDirectories[i].noReads = -1;
            openDirectories[i].clusterDir = -1;
            openDirectories[i].directory = setNullDirent();

            return 0;
        }
    }

    return -1;

}

int createSoftlink(char *linkname,char *filename){ //Fruto do REUSO

    char * path;
    char * name;
    char * find = malloc (sizeof(char) * 50);
    int clusterToLink;
    int i;
    DIRENT2 toLink;
    DIRENT2 *linkType;
    int found = 0;
    const char dir_div = '/';

    if(strrchr(filename, dir_div) == NULL){

        strcpy(find, currentPath.absolute);
        if(!(strcmp(find,"/") == 0))
            strcat(find, "/");
        strcat(find, filename);
        clusterToLink = pathToCluster(find);

        strcpy(name, filename);
    }
    else{
        separatePath(filename, &path, &name);
        clusterToLink = pathToCluster(filename);
    }

    puts(name);

    if(strlen(name) <= 0)
      return -1;

    linkType = readDataClusterFolder(currentPath.clusterNo);

    for(i = 0; i < ( superblock.clusterSize / sizeof(DIRENT2) ) ; i++){
        if (strcmp(linkType[i].name, name) == 0){

            found = 1;
            break;
        }
      }


    if(!found)
        return -1;


    strcpy(toLink.name, "");
    strcpy(toLink.name, name);
    toLink.fileType = 0x03;
    toLink.fileSize = 0;
    toLink.firstCluster = clusterToLink;

    createEnt(1, toLink);

    return 0;
}

int updatePointer(FILE2 handle, DWORD offset){

    int found = 0;
    int currentPointer;
    int currentCluster;
    int fileNo;
    int newCursorPointer;
    int i;

    //procura o arquivo pelo handle
    for(i=0; i < 10; i++){
        if(openFiles[i].file == handle){
            found = 1;
            fileNo = i;
            break;
        }

    }
    if(found == 0){
        return -1;
    }
    //atribuicao dos parametros do arquivo
    currentPointer = openFiles[fileNo].currPointer;
    currentCluster = openFiles[fileNo].clusterNo;

    //novo cp
    newCursorPointer = (int)offset;

    if(newCursorPointer <- 1)
        return -1;

    if(newCursorPointer > sizeOfFile(openFiles[fileNo].clusterDir, openFiles[fileNo].clusterNo) || newCursorPointer < 0){
        return -1;
    }

    if((int)offset == -1){
        newCursorPointer= sizeOfFile(openFiles[fileNo].clusterDir, openFiles[fileNo].clusterNo) + 1;
    }

    openFiles[fileNo].currPointer= newCursorPointer;

    return 0;
}

int sizeOfFile(int clusterDir, int clusterFile){

    DIRENT2* folderContent = malloc(superblock.clusterSize);
    int folderSize = (superblock.clusterSize) / sizeof(DIRENT2 );
    int i;
    //verifica o tamanho do arquivo com o nome dado;
    folderContent = readDataClusterFolder(clusterDir);

    for(i = 0; i < folderSize; i++){

        if(folderContent[i].firstCluster == clusterFile){
            return (int)folderContent[i].fileSize;
        }
    }
    return -1;
}

int isLink(char * path, char ** output){
    int i;
    int link = 0;
    int clusterByteSize = superblock.clusterSize;
    unsigned char* buffer = malloc(clusterByteSize);
    char * pathToFile;
    char * fileName;
    int pathClusterNo;
    int linkClusterNo;

    separatePath(path, &pathToFile, &fileName);

    pathClusterNo = pathToCluster(pathToFile);

    readCluster(pathClusterNo, buffer);

    for(i = 0; i < clusterByteSize; i += sizeof(DIRENT2)) {
        if ((strcmp((char *)buffer+i+1, fileName) == 0) && (((BYTE) buffer[i]) == 0x03 )) {
            link = 1;
            break;
        }
    }

    if(!link) {
        free(buffer);
        *output = malloc(sizeof(char)*(strlen(path)+1));
        strcpy(*output,path);

        return 0;
    }

    linkClusterNo = pathToCluster(path);

    memset(buffer,0,clusterByteSize);

    readCluster(linkClusterNo,buffer);

    *output = malloc(sizeof(char)*(strlen((char*)buffer)+1));
    strcpy(*output,(char*)buffer);

    free(buffer);

    return 1;
}
