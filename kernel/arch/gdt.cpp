#define __GDT__

#include <error.h>
#include <arch/gdt.h>
#include <std/types.h>
#include <proc/tss.h>
#include <mm/memcpy.h>

GDTDesc* GDT::gdt;
GDTR GDT::gdtr;

// Segment selector
// 15   3   2  1 0
// |Index| TI |RPL|

// Base : Segment base address (not used for expand-down segment)
// Limit : Segment limit (in bytes or 4Kio units, depending on granularity G)
// Acces :	 7 6 5 4  3   2   1  0
// 			|P|DPL|S|D/C|E/C|W/R|A|
// 		P	Segment present
// 		DPL	Descriptor privilege level (0-3)
// 		S	Not-System segment flag (0 for TSS/etc, 1 for code/data/stack)
// 		D/C 0=Data segment, 1=Code segment
//		E/C 1=Expand-down data/Conforming code
// 		W/R 0=Read-only data/Execute-only code, 1=Read-write data/Execute-read code
//		A	1=Accessed by CPU (CPU will set this flag on segment loading), 0=Not accesed by processor (since last time we cleared the flag)
// Other :	 3  2  1  0
// 			|G|D/B|L|AVL|
// 		G : Granularity (0=Byte, 1=4Kio-units)
// 		D/B : 1=32-bit segment, 0=16-bit
//		L : 64-bit code segment
//		AVL : Undefined/Available, we can use this bit for whatever we want
void GDT::initDescriptor(u32 base, u32 limit, u8 acces, u8 other, struct GDTDesc *desc)
{
    desc->lim0_15 = (limit & 0xffff);
    desc->base0_15 = (base & 0xffff);
    desc->base16_23 = (base & 0xff0000) >> 16;
    desc->acces = acces;
    desc->lim16_19 = (limit & 0xf0000) >> 16;
    desc->other = (other & 0xf);
    desc->base24_31 = (base & 0xff000000) >> 24;
}

/// Init and load the GDT, then set up the segments and the TSS
void GDT::init()
{
    gdt = (GDTDesc*)GDTBASE;
    gdtr.base = GDTBASE;
    gdtr.limite = 0x100;

    // Init missing segment descriptors (boot.asm started the job)
    //initDescriptor(0x0, 0x0,     0b0,        0b0000, &gdt[0]);  // Null descriptor (to describe unused segments)
    //initDescriptor(0x0, 0xFFFFF, 0b10011011, 0b1011, &gdt[1]);	// Kernel code
    //initDescriptor(0x0, 0xFFFFF, 0b10010011, 0b1101, &gdt[2]);	// Kernel data/stack

    initDescriptor(0x0, 0xFFFFF, 0b11111111, 0b1101, &gdt[3]);	// user 32bit code
    initDescriptor(0x0, 0xFFFFF, 0b11111111, 0b1011, &gdt[4]);	// user 64bit code
    initDescriptor(0x0, 0xFFFFF, 0b11110011, 0b1101, &gdt[5]);	// user data
    initDescriptor(0x0, 0x0,     0b11110111, 0b1101, &gdt[6]);	// user stack

    initDescriptor((u32)(u64) TSS::getDefaultTss(), 0x67, 0b11101001, 0b0, &gdt[7]);	// Default TSS descriptor
    initDescriptor(0, 0, 0, 0, &gdt[8]);	// Default TSS descriptor (high part)
    initDescriptor(0x0, RING0_STACK_LIM/4096,     0b10010111, 0b1101, &gdt[9]);	// Default TSS stack (downward limit of 516 kiB)

    // Runtime check that we're not setting a wrong stack limit
    if (int(RING0_STACK_LIM/4096) != RING0_STACK_LIM/4096)
        fatalError("GDT::init(): RING0_STACK_LIM must be a multiple of 4096");

    u64 gdtrAddr = (u64)&gdtr;
    asm("mov %0, %%rax \n lgdt (%%rax)"::"m"(gdtrAddr):"rax");
    asm("movw $0x38, %ax; ltr %ax");
}

struct GDTDesc* GDT::getDescriptorTable()
{
    return gdt;
}

int GDT::getDefaultSS()
{
    return 8*2;
}

int GDT::getUserStackSelector()
{
    return 8*6 + 3;
}

int GDT::getUserDataSelector()
{
    return 8*5 + 3;
}

int GDT::getUserCode32Selector()
{
    return 8*3 + 3;
}

int GDT::getUserCode64Selector()
{
    return 8*4 + 3;
}
