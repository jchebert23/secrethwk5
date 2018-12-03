CC = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall
HWK = /c/cs323/Hwk

all: clean bashful

bashful: ${HWK}5/mainBashful.o ${HWK}2/tokenize.o ${HWK}5/parsnip.o process.o
	${CC} ${CFLAGS} -o $@ $^

process.o: process.c ${HWK}5/process-stub.h
	${CC} ${CFLAGS} -c $<

clean:
	${RM} Bash *.o vgcore*

