#NOTE: libpwu needs to be installed for this example to work

CC=gcc
ASM=nasm
CFLAGS_EXEC=-O0 -ggdb -Wall
CFLAGS_TGT=-O0 -ggdb -Wall --static

new_thread: exec.c
	${CC} ${CFLAGS_EXEC} -o exec exec.c -lpwu -ldl

target: target.c
	${CC} ${CFLAGS_TGT} -o target target.c

clean:
	rm exec
