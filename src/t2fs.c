
/**
*/
#include "../include/t2fs.h"
#include "../include/filesys.h"
#include "../include/apidisk.h"
#include <stdio.h>
#include <string.h>

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size) {

	char *devNames = "Filipe Bachini Lopes - 291401\nMoatan Pedroso Godoy - 246789\nThalles Fernandes Rezende - 288546\n\0";

	if (strlen(devNames) < size) {
		strcpy(name, devNames);

		return 0;
	}
	// Se chegou aqui deu erro
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Formata logicamente o disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho
		corresponde a um múltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2 (int sectors_per_block) {

		FATformat(sectors_per_block);
		return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo arquivo no disco e abrí-lo,
		sendo, nesse último aspecto, equivalente a função open2.
		No entanto, diferentemente da open2, se filename referenciar um
		arquivo já existente, o mesmo terá seu conteúdo removido e
		assumirá um tamanho de zero bytes.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename) {
	if (strcmp(filename, "") == 0)
		return -1;

	FATinit();

	return createFile(filename);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco.
-----------------------------------------------------------------------------*/
int delete2 (char *filename) {
	if (strcmp(filename, "") == 0)
		return -1;

	FATinit();

	return deleteFile(filename);
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename) {
	if (strcmp(filename, "") == 0)
		return -1;

	FATinit();

	return openFile(filename);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle) {
	if (handle < 0)
		return -1;

	FATinit();

	return closeFile(handle);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size) {
	if(handle < 0 || size < 0)
		return -1;

	FATinit();

	return readFile(handle, buffer, size);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size) {
	if(handle < 0 || size < 0)
		return -1;

	FATinit();

	return writeFile(handle, buffer, size, 0);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo. Remove do arquivo
		todos os bytes a partir da posição atual do contador de posição
		(current pointer), inclusive, até o seu final.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle) {
	if(handle < 0)
		return -1;

	FATinit();

	return truncateFile(handle);
}

/*-----------------------------------------------------------------------------
Função:	Altera o contador de posição (current pointer) do arquivo.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, DWORD offset) {
	if (handle < 0 || offset < 0)
		return -1;

	FATinit();

	return updatePointer(handle, offset);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo diretório.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname) {
	if (strcmp(pathname, "") == 0)
		return -1;

	FATinit();
	return createDir(pathname);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um diretório do disco.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname) {
	if (strcmp(pathname, "") == 0)
		return -1;

	FATinit();
	return deleteDir(pathname);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para alterar o CP (current path)
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname) {
	if(strcmp(pathname,"") == 0)
		return -1;

	FATinit();
	return changeDir(pathname);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para obter o caminho do diretório corrente.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size) {
	if((strlen(currentPath.absolute) + 1) > size)
		return -1;

	if(strcmp(pathname,"") == 0)
		return -1;

	FATinit();
	memset(pathname,'\0',size);
	strcpy(pathname, currentPath.absolute);

	return 0;

}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname) {
	if (strcmp(pathname, "") == 0)
		return -1;
	FATinit();
	return openDir(pathname);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry) {

	if (handle < 0)
		return -1;

	FATinit();
	dentry = readDir(handle);

	if(dentry == NULL)
		return -1;

	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle) {
	if (handle < 0)
		return -1;

	FATinit();
	return closeDir(handle);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink) com
		o nome dado por linkname (relativo ou absoluto) para um
		arquivo ou diretório fornecido por filename.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename) {
	if (strcmp(linkname, "") == 0)
		return -1;
	if (strcmp(filename, "") == 0)
		return -1;

	FATinit();
	return createSoftlink(linkname, filename);
}
