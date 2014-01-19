#define __GDT__

#include <error.h>
#include <gdt.h>
#include <memmap.h>
#include <paging.h>
#include <std/types.h>

// Segment selector
// 15   3   2  1 0
// |Index| TI |RPL|

GDT::GDT()
{
    // Init TSS struct
    defaultTss.debug_flag = 0x0;
    defaultTss.io_map = 0x0;
    defaultTss.esp0 = RING0_STACK; // Offset (from base of segment descripted by ss0)
    defaultTss.ss0 = 0b1000000; // Segment descriptor 8

    // Init GDTR struct
    gdtr.limite = GDTSIZE * 8;
    gdtr.base = GDTBASE;
}

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
    // Init segment descriptors
    initDescriptor(0x0, 0x0,     0b0,        0b0000, &gdt[0]);  // Null descriptor (to describe unused segments)
    initDescriptor(0x0, 0xFFFFF, 0b10011011, 0b1101, &gdt[1]);	// Kernel code 0-FFFFF * 4KiB (all memory), (4GiB)
    initDescriptor(0x0, 0xFFFFF, 0b10010011, 0b1101, &gdt[2]);	// Kernel data 0-FFFFF * 4KiB (all memory), (4GiB)
    initDescriptor(0x0, 0x0,     0b10010111, 0b1101, &gdt[3]);	// Kernel stack +inf-0, all memory, base not interpreted, limit is 0 (downward)

    initDescriptor(0x0, 0xFFFFF, 0b11111111, 0b1101, &gdt[4]);	// user code, all memory
    initDescriptor(0x0, 0xFFFFF, 0b11110011, 0b1101, &gdt[5]);	// user data, all memory
    initDescriptor(0x0, 0x0,     0b11110111, 0b1101, &gdt[6]);	// user stack, all memory

    initDescriptor((u32) & defaultTss, 0x67, 0b11101001, 0b0, &gdt[7]);	// Default TSS descriptor
    initDescriptor(0x0, RING0_STACK_LIM/4096,     0b10010111, 0b1101, &gdt[8]);	// Default TSS stack (downward limit of 516 kiB)

    // Runtime check that we're not setting a wrong stack limit
    if (int(RING0_STACK_LIM/4096) != RING0_STACK_LIM/4096)
        fatalError("GDT::init(): RING0_STACK_LIM must be a multiple of 4096");

    // Copy and load the GDT
    memcpy((char *) gdtr.base, (char *) gdt, gdtr.limite);
    asm("lgdtl %0": "=m" (gdtr):);

    // Init segments (with descriptor 2). The long jump is to force them to update
    asm("   movw $0b10000, %ax	\n \
            movw %ax, %ds	\n \
            movw %ax, %es	\n \
            movw %ax, %fs	\n \
            movw %ax, %gs	\n \
            ljmp $0x08, $_GDT_LJMP	\n \
            _GDT_LJMP:		\n");

    // Init stack (with descriptor 3)
    asm("	movw $0b11000, %%ax \n \
            movw %%ax, %%ss"::);

    // Load the TSS (with descriptor 7)
    asm("movw $0b111000, %ax; ltr %ax");
}

struct GDTDesc* GDT::getDescriptorTable()
{
    return gdt;
}

struct TSS* GDT::getDefaultTss()
{
    return &defaultTss;
}
