#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include <kernel/std/types.h>

struct boot_device
{
	u8 part3;
	u8 part2;
	u8 part1;
	u8 disk;
};

struct apm_table
{
	u16 version;
	u16 cseg;
	u16 offset;
	u16 cseg_16;
	u16 dseg;
	u16 flags;
	u16 cseg_len;
	u16 cseg_16_len;
	u16 dseg_len;
};

struct mmap
{
	u32 size;
	u64 base_addr;
	u64 length;
	u32 type; // Type 1 = availlable RAM, anything else = reserved RAM
};

struct multiboot_info
{
	u32 flags;
	u32 low_mem;     // < 1 MiB
	u32 high_mem;    // > 1 MiB
	struct boot_device boot_device;
	u32 cmdline;
	u32 mods_count;
	u32 mods_addr;
	struct
	{
		u32 num;
		u32 size;
		u32 addr;
		u32 shndx;
	} elf_sec;
	u32 mmap_length;
	u32 mmap_addr;
	u32 drives_length;
	u32 drives_addr;
	u32 config_table;
	u32 bootloader_name;
	u32 apm_table;
	u32 vbe_control_info;
	u32 vbe_mode_info;
	u16 vbe_mode;
	u16 vbe_interface_seg;
	u16 vbe_interface_off;
	u16 vbe_interface_len;
};

/// Print some info about the multiboot struct the bootloader gave us.
/// Will halt if the bootloader isn't multiboot-compliant
void checkMbi(u32 mbmagic, struct multiboot_info *mbi);

/// If a memory map is provided, reserve all the pages corresponding to reserved RAM and return true.
/// If there's no memory map in the mbi, return false.
bool reserveMbiMemmap(struct multiboot_info *mbi);

/// If a memory map is provided, print it
/// If there's no memory map in the mbi, return false.
bool printMbiMemmap(struct multiboot_info *mbi);

#endif
