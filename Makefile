PROJECT = sepples
SRC_ASM = $(shell find . -name *.asm)
SRC_CPP = $(shell find . -name *.cpp)
OBJ_ASM = $(SRC_ASM:%.asm=%.o)
OBJ_CPP = $(SRC_CPP:%.cpp=%.o)
OBJ_KERNEL = kernel.elf
ISO = $(PROJECT).iso
BUILD_ISO = isobuild
CXXFLAGS = -Os -Ikernel -nostdinc -nostdinc++ -std=c++14 -ffreestanding -fno-builtin -fno-rtti -nostdlib -fno-exceptions -nodefaultlibs -nostartfiles -fno-stack-protector -Wall -Wextra -Wunreachable-code
GRUB_MENUENTRY = "Sepples OS"
GRUB_MENUDELAY = 0
GRUB_MODULES = "iso9660 multiboot configfile part_acorn part_amiga part_apple part_bsd part_dfly part_dvh part_gpt part_plan part_sun part_sunpc"
GRUB_COMPRESS = no # no/gz/xz

%.o: %.asm
	nasm -f elf64 $^

.PHONY: all clean mrproper

all: $(ISO)
	
clean:
	rm -f $(OBJ_ASM)
	rm -f $(OBJ_CPP)
	rm -f $(OBJ_KERNEL)
	rm -rf ./$(BUILD_ISO)

mrproper: clean
	rm -f $(ISO)
	
$(OBJ_KERNEL): $(OBJ_ASM) $(OBJ_CPP)
	ld -melf_x86_64 -static -Ttext=0x8000 -z max-page-size=0x1000 --entry=asmboot $^ -o $(OBJ_KERNEL)

$(ISO): $(OBJ_KERNEL)
	mkdir -p $(BUILD_ISO)/boot/grub
	echo "set timeout=$(GRUB_MENUDELAY)\nmenuentry \"$(GRUB_MENUENTRY)\" {multiboot /boot/$(PROJECT).elf;boot}" > $(BUILD_ISO)/boot/grub/grub.cfg
	cp $(OBJ_KERNEL) ./$(BUILD_ISO)/boot/$(PROJECT).elf
	grub-mkrescue --compress=$(GRUB_COMPRESS) --install-modules=$(GRUB_MODULES) -o $(ISO) ./$(BUILD_ISO) 2>&1

test: $(ISO)
	bochs -qf bochsrc-nodebug.txt -rc bochsdebug.txt || return 0

debug: CXXFLAGS += -g -Og
debug: $(ISO)

release: $(ISO)
		
