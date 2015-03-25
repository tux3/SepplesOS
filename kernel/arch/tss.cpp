#include <arch/tss.h>
#include <mm/memmap.h>

static TSS defaultTss;

extern "C" void writeDescriptor(u64 baseLimite, u8 access, u8 flags, u64 index);

void TSS::init()
{
    // Init default TSS struct
    defaultTss.io_map = 0x0;
    defaultTss.rsp0 = RING0_STACK;
    defaultTss.rsp1 = RING0_STACK;
    defaultTss.rsp2 = RING0_STACK;
    defaultTss.ist1 = (u64)&defaultTss.rsp0;

    // Write TSS descriptor in GDT
    writeDescriptor((u64)TSS::getDefaultTss()<<32|0x67, 0b11101001, 0b0, 10);
    writeDescriptor(0, 0, 0, 11);
    // Write TSS stack descriptor in GDT
    static_assert(RING0_STACK_LIM%4096==0,
                  "RING0_STACK_LIM must be a multiple of 4096");
    writeDescriptor(RING0_STACK_LIM/4096, 0b10010111, 0b1101, 12);
    // Load TSS
    asm("mov $0x50, %rax; ltr %ax");
}

TSS* TSS::getDefaultTss()
{
    return &defaultTss;
}
