#
# Makefile pricinpal
#

NAME=sepples

# Les cibles .PHONY ne sont pas des fichiers à creer !
.PHONY: kernel shell prepare-usb update-usb clean

# Si make sans cible ou make all, on recompil et on fait une iso
all: clean $(NAME).iso

# Compile un noyau, rend la cle bootable et copie le noyau dessus
prepare-usb: $(NAME).iso
	-mkdir /mnt/USB # Ignore les erreurs de creation de dossier (existe deja ?)
	mount /dev/sdb1 /mnt/USB
	grub-install --force --recheck --no-floppy --root-directory=/mnt/USB /dev/sdb
	cp ./cdrom/BOOT/GRUB.CFG /mnt/USB/boot/grub/grub.cfg
	cp ./cdrom/BOOT/$(NAME).ELF /mnt/USB/boot/$(NAME).elf
	umount /mnt/USB
	eject /dev/sdb
	rmdir /mnt/USB

# Copie le dernier noyau compile sur la cle
update-usb: $(NAME).iso
	-mkdir /mnt/USB # Ignore les erreurs de creation de dossier (existe deja ?)
	mount /dev/sdb1 /mnt/USB
	cp ./cdrom/BOOT/GRUB.CFG /mnt/USB/boot/grub/grub.cfg --remove-destination
	cp ./cdrom/BOOT/$(NAME).ELF /mnt/USB/boot/$(NAME).elf --remove-destination
	umount /mnt/USB
	eject /dev/sdb
	rmdir /mnt/USB

kernel:
	$(MAKE) -C kernel

shell:
	$(MAKE) -B -C shell all

$(NAME).iso: kernel shell
	./makegrub2cdimage.sh

test: clean $(NAME).iso
	-bochs -qf bochsrc-nodebug.txt -rc bochsdebug.txt

clean:
	rm -f *.o bochs.log $(NAME).iso
	$(MAKE) clean -C kernel

