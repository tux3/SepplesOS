#/bin/bash

#
# Create GRUB2 config files
#
mkdir cdrom 2> /dev/null
echo "set prefix=(cd)/BOOT
configfile /BOOT/GRUB.CFG" > cdrom/PREFIX.TXT
mkdir cdrom/BOOT 2> /dev/null
echo 'set timeout=15
set default=0 # Set the default menu entry

menuentry "Sepples OS"
{
   multiboot /BOOT/sepples.ELF   # The multiboot command replaces the kernel command boot
   boot
}' > cdrom/BOOT/GRUB.CFG

#
# Copy GRUB2 bootloader
#
cd cdrom
grub-mkimage biosdisk iso9660 multiboot configfile normal -O i386-pc -p "(cd)/BOOT" -c PREFIX.TXT -o CORE.IMG
cat /usr/lib/grub/i386-pc/cdboot.img CORE.IMG > BOOT/ELTORITO.IMG
cd ..

#
# Copy OS Kernel
#
cp kernel/kernel cdrom/BOOT/sepples.ELF

#
# Create Dusk.iso GRUB2 bootable image
#
genisoimage -R -b BOOT/ELTORITO.IMG -no-emul-boot -boot-load-size 4 -boot-info-table -o sepples.iso cdrom

