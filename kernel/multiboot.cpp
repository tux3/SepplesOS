#include <multiboot.h>
#include <error.h>
#include <mm/paging.h>
#include <vga/vgatext.h>
#include <lib/humanReadable.h>

void checkMultiboot(u32 mbmagic, struct multiboot_info *mbi)
{
    u8 buf[128];
    VGAText::setCurStyle(VGAText::CUR_BLUE,true); // Sys infos in blue
	if (mbmagic != 0x2BADB002)
		fatalError("Bootloader is not multiboot-compliant. Halting.\n");
	if(mbi->flags & 0b00000001) // If flags[0], we have low_mem and high_mem
        VGAText::printf("Available low mem : %s\nAvailable high mem : %s\n", toIEC(buf,64,mbi->low_mem*1000), toIEC(buf+64,64,mbi->high_mem*1000));
	if(mbi->flags & 0b00000010) // If flags[1], we have boot_device
	{
		if (mbi->boot_device.disk == 0x9f || mbi->boot_device.disk == 0xe0 || mbi->boot_device.disk == 0xef)
            VGAText::printf("Boot device : CD-ROM\n");
		else if (mbi->boot_device.disk < 0x80)
            VGAText::printf("Boot device : Floppy drive 0x%x\n", mbi->boot_device.disk);
		else
            VGAText::printf("Boot device : Partition %x:%x:%x on disk drive 0x%x\n", mbi->boot_device.part3, mbi->boot_device.part2, mbi->boot_device.part1, mbi->boot_device.disk);
	}
	if(mbi->flags & 0b00000100) // If flags[2], we have a cmdline
        VGAText::printf("Boot command line : %s\n", mbi->cmdline);
	if(mbi->flags & 0b00001000) // If flags[3], we have modules
        VGAText::printf("Boot modules : %s\n", mbi->mods_count);
	if(mbi->flags & 0b00010000) // If flags[4], but not flag[5], we have a symbol table in a.out format
        VGAText::printf("Symbol table provided (a.out)\n");
	if(mbi->flags & 0b00100000) // If flags[5], but not flag[4], we have a symbol table in ELF format
        VGAText::printf("Symbol table provided (ELF)\n");
	if(mbi->flags >> 4 & 0b1 && mbi->flags >> 4 & 0b10) // If flag[4] and flag[5], the bootloader isn't conformant. Better exit.
		fatalError("Bootloader is not multiboot-compliant. Halting.\n");
	if(mbi->flags & 0b01000000) // If flags[6], we have a Memory Map
        VGAText::printf("Memory Map provided\n");
	if(mbi->flags & 0b10000000) // If flags[7], we have a Drives Map
        VGAText::printf("Drives Map provided\n");
	if(mbi->flags >> 8 & 0b00000001) // If flags[8], we have a ROM config table
        VGAText::printf("ROM configuration table provided\n");
	if(mbi->flags >> 8 & 0b00000010) // If flags[9], we have the bootloader's name
        VGAText::printf("Bootloader name : %s\n", mbi->bootloader_name);
	if(mbi->flags >> 8 & 0b00000100) // If flags[10], we have an APM table (Advenced Power Management)
        VGAText::printf("APM table provided\n");
	if(mbi->flags >> 8 & 0b00001000) // If flags[11], we have a Graphic table
        VGAText::printf("Graphic table provided\n");
    VGAText::setCurStyle();
}

bool reserveMbiMemmap(struct multiboot_info *mbi)
{
    if(!(mbi->flags & 0b01000000))
        return false;

    unsigned maxPhysPages = Paging::getMaxPhysPages();
    struct mmap* cur = (struct mmap*)(u64) mbi->mmap_addr;
    struct mmap* end = (struct mmap*) ((u64)cur + mbi->mmap_length);
    for (;cur < end; cur = (struct mmap*) ((u64)cur + (u32)cur->size + sizeof(cur->size)))
        if (cur->type != 1) // Type 1 is available RAM, everything else is reserved
            for (unsigned int i=0; i<cur->length; i += PAGESIZE)
                if ((cur->base_addr + i)/PAGESIZE < maxPhysPages)
                    Paging::markPhysPageUsed((cur->base_addr + i)/PAGESIZE);
                else
                    break;

    return true;
}

bool printMbiMemmap(struct multiboot_info *mbi)
{
    if(!(mbi->flags & 0b01000000))
        return false;
    struct mmap* cur = (struct mmap*) (u64)mbi->mmap_addr;
    struct mmap* end = (struct mmap*) ((u64)cur + mbi->mmap_length);
    for (;cur < end; cur = (struct mmap*) ((u64)cur + (u64)cur->size + sizeof(cur->size)))
        VGAText::printf("Type %d : 0x%x - 0x%x\n", cur->type, cur->base_addr, cur->base_addr+cur->length);

    return true;
}

unsigned long getAvailableMem(struct multiboot_info *mbi)
{
    if(!(mbi->flags & 0b00000001))
        return 0;

    return (mbi->low_mem+mbi->high_mem)*1000;
}
