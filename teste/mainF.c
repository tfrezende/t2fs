
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"
#include "../include/filesys.h"

void help() {

	printf ("Testing program - read and write setores do arquivo t2fs_disk.dat\n");
	printf ("   DISPLAY - d <setor>\n");
	printf ("\n");
	printf ("   HELP    - ?\n");
	printf ("   FIM     - f\n");
}

int main(int argc, char *argv[])
{
	char	command[128];
	char	*cmd;

	help();
	while (1) {
		printf ("CMD> ");
		gets(command);
		if (strlen(command)==0)
			continue;

		cmd = strtok(command, " ");

		if (strcmp(cmd,"?")==0) {

			help();
			continue;
		}

		if (strcmp(cmd,"d")==0) {							// FUNÇÃO DO CECHIN, TÁ COM UNS PROBLEMAS, DEIXA AÍ
			// comando d (display)
			unsigned char buffer[SECTOR_SIZE];
			char *p1 = strtok(NULL, " ");
			if (p1!=NULL) {
				printf ("Erro no comando.\n");
				continue;
			}

			int sector = 0;
			int error = read_sector (sector, buffer);
			if (error) {
				printf ("read_sector (%d) error = %d\n", sector, error);
				continue;
			}

			char str[20];
			int linhaBase = SECTOR_SIZE * sector;
			int linha, coluna;
			for (linha=0; linha<16; ++linha) {
			    printf ("%04X  ", linhaBase+16*linha);
			    for (coluna=0; coluna<16; ++coluna) {
				int index = 16*linha+coluna;
				char c = buffer[index];
				if (c>=' ' && c<='z') str[coluna]=c;
				else str[coluna]=' ';
				printf ("%02X ", c&0xFF);
			    }
			    str[16]='\0';
			    printf (" *%s*\n", str);
			}
			continue;
		}

		if (strcmp(cmd,"f")==0) {
			printf ("Fim.\n");
			break;
		}

		if (strcmp(cmd,"e")==0) {			// TESTE DE ESCRITA, SÓ MUDAR OS VALORES DE TESTE E O VALOR DO write_sector

			unsigned char teste[SECTOR_SIZE] = {"32305"};

		//	gets(teste);

			int error = write_sector (1, teste);

			if (error) {
				printf ("Deu pau");
				continue;
			}

			printf("Parece que foi\n");

			continue;
		}

		if (strcmp(cmd,"l")==0) {					// TESTE EM GERAL, PODE USAR QUALQUER FUNÇÃO AQUI

		 	int error = 0;
			int i, teste;
			char *path;
		    char *dirName;
			int handle, handle1;
			int bytesWritten = 0;
			int bytesRead = 0;
			char *buffer = malloc(sizeof(char) * 3023);

		 	error = FATformat(8);

			mkdir2("/jubileu");

			create2("no foco da dengue");

			mkdir2("/cechin calvo cabeludo");

			create2("/jubileu/teste");

			handle = open2("no foco da dengue");

			bytesWritten = write2(handle, "sera que escreve?", 17);

			write2(handle, "vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 400);
			write2(handle, "na busca do cluster cheio vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);
			write2(handle, "certeza que ainda não encheu então vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);
			write2(handle, "ainda não tenho certeza, pq esse cluster é grande pra cacete então vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);
			write2(handle, "bom ser precavido e escrever mais coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);

			write2(handle, "vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 400);
			write2(handle, "na busca do cluster cheio vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);
			write2(handle, "certeza que ainda não encheu então vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);
			write2(handle, "ainda não tenho certeza, pq esse cluster é grande pra cacete então vamo escrever coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);
			write2(handle, "bom ser precavido e escrever mais coisa pra caralho nessa porra nao sei mais o que escrever mais sei que essa porra tem que ser enorme e nao tenho criatividade o suficiente no momento pra elaborar uma disseracao de mestrado puta merda so quero deixar essa porra de string grande pra caralho", 1000);

			printf("Current Pointer antes do read: %d\n", openFiles[0].currPointer);

			seek2(handle, 0);

			bytesRead = read2(handle, buffer, 3023);

			seek2(handle, 1000);

			printf("Current Pointer depois do read: %d\n", openFiles[0].currPointer);

			truncate2(handle);

			free(buffer);
			continue;
		}

		if (strcmp(cmd,"t")==0) {				// TESTE DE LEITURA, MUDAR O VALOR DE SECTOR QUE QUER LER

			unsigned char testeleitura[SECTOR_SIZE];
			int sector;


			for(sector = 25; sector < 34; sector ++){
				int error = read_sector (sector, testeleitura);
				if (error) {
					printf ("read_sector (%d) error = %d\n", sector, error);
					continue;
				}
				char str[20];
				int linhaBase = SECTOR_SIZE * sector;
				int linha, coluna;
				for (linha=0; linha<16; ++linha) {
						printf ("%04X  ", linhaBase+16*linha);
						for (coluna=0; coluna<16; ++coluna) {
					int index = 16*linha+coluna;
					char c = testeleitura[index];
					if (c>=' ' && c<='z') str[coluna]=c;
					else str[coluna]=' ';
					printf ("%02X ", c&0xFF);
						}
						str[16]='\0';
						printf (" *%s*\n", str);
				}
			}
			continue;
		}

		printf("Comando nao reconhecido.\n");
	}

    return 0;
}
