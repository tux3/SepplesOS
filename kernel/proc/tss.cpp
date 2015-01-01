#include <proc/tss.h>
#include <mm/memmap.h>

static TSS defaultTss;

void TSS::init()
{
    // Init default TSS struct
    defaultTss.io_map = 0x0;
    defaultTss.rsp0 = RING0_STACK;
    defaultTss.rsp1 = RING0_STACK;
    defaultTss.rsp2 = RING0_STACK;
    defaultTss.ist1 = (u64)&defaultTss.rsp0;
}

TSS* TSS::getDefaultTss()
{
    return &defaultTss;
}
