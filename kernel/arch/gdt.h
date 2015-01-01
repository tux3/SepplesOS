#ifndef GDT_H_INCLUDED
#define GDT_H_INCLUDED

#include <std/types.h>
#include <mm/memmap.h>

// Segment descriptor
struct GDTDesc
{
    u16 lim0_15;
    u16 base0_15;
    u8 base16_23;
    u8 acces;
    u8 lim16_19:4;
    u8 other:4;
    u8 base24_31;
} __attribute__ ((packed));

// GDT Register
struct GDTR
{
    u16 limite;
    u64 base;
} __attribute__ ((packed));

class GDT
{
    public:
        GDT() = delete;
        static void initDescriptor(u32, u32, u8, u8, struct GDTDesc *);
        static void init();
        static struct GDTDesc* getDescriptorTable();
        static int getDefaultSS();
        static int getUserStackSelector();
        static int getUserDataSelector();
        static int getUserCode32Selector();
        static int getUserCode64Selector();

    private:
        static GDTDesc* gdt;
        static GDTR gdtr;
};

#endif // GDT_H_INCLUDED
