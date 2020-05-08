CC = gcc
CFLAGS = -g -m32

all: bmpFilter

bmpFilter: bmpBlackWhite.o
	mpicc bmpBlackWhite.o bmpFilterStatic.c -o bmpFilterStatic 
	mpicc bmpBlackWhite.o bmpFilterDynamic.c -o bmpFilterDynamic 

clean:
	rm -f bmpFilterStatic
	rm -f bmpFilterDynamic
