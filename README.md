SepplesOS
=========

Toy OS in C++

Shamelessly stealing code and ideas from Arnauld Michelizza, and pretty much everyone else.

<h3>Features</h3>
- Paged and segmented memory management
- Text terminal with scrolling and logging
- VFS with read-only ext2/ext3 driver

<h3>System requirements</h3>
- 1 MiB free space (hard drive or live USB/CD)
- 6.5 MiB RAM
- VGA Text Mode compatible graphic card
- 32 or 64 bit x86 CPU

<h3>Build system</h3>
Autotool (make). A Code::Blocks project file is also provided.
- make kernel : Build the kernel
- make shell : Build the userland
- make sepples.iso : Build and creates a bootable .iso
- make test : Rebuild sepples.iso and run it with bochs.
- make clean : Removes objects files, logs and sepples.iso
- make prepare-usb : Install the kernel and GRUB2 on /dev/sdb
- make update-usb : Overwrite the kernel on /dev/sdb (use prepare-usb once before)
WARNING: Do NOT use "make prepare-usb" and "make update-usb" if /dev/sdb isn't a USB key !

<h3>Keyboard</h3>
QWERTY layout. May be incomplete.<br/>
Use the PAUSE/BREAK key to get memory stats (heap/stack free/used).<br/>
Use CTRL+ESC to reboot and ALT+ESC to halt.<br/>
Use PageUp and PageDown to scroll up/down in the terminal.<br/>

<h4>But why ?</h4>
Learning.
