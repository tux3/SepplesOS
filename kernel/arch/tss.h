#ifndef TSS_H
#define TSS_H

#include <std/types.h>


struct TSS
{
    u32 _reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 _reserved1;
    u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    u64 _reserved2;
    u64 _reserved3;
    u32 _reserved4;
    u16 io_map;

    static void init();
    static TSS* getDefaultTss();

} __attribute__ ((packed));

#endif // TSS_H
