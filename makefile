#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
#

CC=gcc
BIN_DIR=./bin
LIB_DIR=./lib
INC_DIR=./include
SRC_DIR=./src

all: fs tfs
	ar crs $(LIB_DIR)/libt2fs.a $(BIN_DIR)/t2fs.o $(BIN_DIR)/filesys.o $(LIB_DIR)/apidisk.o

tfs:
	$(CC) -c $(SRC_DIR)/t2fs.c -o $(BIN_DIR)/t2fs.o -Wall

fs:
	$(CC) -c $(SRC_DIR)/filesys.c -o $(BIN_DIR)/filesys.o -Wall

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~
