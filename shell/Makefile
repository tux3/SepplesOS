#
# Makefile des apps utilisateur de Dusk
#

OBJ=
CC=gcc -m32 -Wall -fno-builtin -fno-stack-protector

all: 

.o: .c
	$(CC) -c $^

clean:
	rm -f $(OBJ) *.o *.a
	make -C libc clean
