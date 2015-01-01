#
# Makefile des apps utilisateur de Dusk
#

OBJ=
CC=gcc -I. -m32 -Wall -fno-builtin -fno-stack-protector -fomit-frame-pointer

all: lib/sepples.o lib/string.o hello.elf sh.elf

lib/%.o: lib/%.c
	$(CC) -c $^ -o $@

%.o: %.c
	$(CC) -c $^
	
%.elf: %.o
	ld -Ttext=40000000 --entry=main --oformat=elf32-i386 -melf_i386 -o $@ $^ lib/sepples.o lib/string.o

deploy:
	../mount.sh
	cp *.elf ../mnt/bin
	../umount.sh

clean:
	rm -f $(OBJ) *.o *.a *.elf
	rm lib/sepples.o
