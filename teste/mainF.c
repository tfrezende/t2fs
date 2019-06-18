
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

			deleteDir("/outroteste");
			
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
			char *pathname = malloc (sizeof(char) * 50);

		 	error = FATformat(8);

<<<<<<< HEAD
			createDir("/teste");

			createDir("/outroteste");

			createDir("/maisumteste");
=======
			teste = createDir("/teste");

			puts(currentPath.absolute);
			printf("%d\n", currentPath.clusterNo);

			teste = changeDir("/teste");

			puts(currentPath.absolute);
			printf("%d\n", currentPath.clusterNo);
>>>>>>> f9c2432b93a5b8ecbe9e6a0419c0b9ce07e4c085

			printf("Parece que foi\n");

			continue;
		}

		if (strcmp(cmd,"t")==0) {				// TESTE DE LEITURA, MUDAR O VALOR DE SECTOR QUE QUER LER

			unsigned char testeleitura[SECTOR_SIZE];
			int sector;

<<<<<<< HEAD
=======
			printf("DIR : %d\n\n\n", sizeof(DIRENT2));

>>>>>>> f9c2432b93a5b8ecbe9e6a0419c0b9ce07e4c085
			for(sector = 0; sector < 10; sector ++){
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
