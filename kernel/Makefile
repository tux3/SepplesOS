#
# Makefile of the kernel
#

# Find all source files
CPP_SRC=$(shell find . -name "*.cpp")
CPP_OBJ=$(CPP_SRC:.cpp=.o)
ASM_SRC=$(shell find . -name "*.asm")
ASM_OBJ=$(ASM_SRC:.asm=.o)

# Compile flags
CC=g++ -m32 -std=c++0x -ffreestanding -fno-builtin -fno-rtti -nostdlib -fno-exceptions -nodefaultlibs -fstrength-reduce -nostartfiles -fno-stack-protector -Wall -Wextra -Wunreachable-code -I . -I ..
ENTRY=0 # Kernel entry point

# Cibles qui ne produisent pas de fichier
.PHONY: clean

all: kernel

kernel: $(ASM_SRC) $(CPP_SRC)
	for f in $(ASM_SRC); do \
		nasm -f elf $$f ; \
	done
	$(CC) -c $(CPP_SRC)
	ld -melf_i386 -static -Ttext=$(ENTRY) --entry=_boot $(ASM_OBJ) $(notdir $(CPP_OBJ)) -o $@

clean:
	rm -f *.o kernel
