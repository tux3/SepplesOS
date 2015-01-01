#include <debug.h>
#include <error.h>

void setBreakpoint(u8 regNum, u32 addr, u8 len, u8 mode)
{
    if (regNum > 3)
    {
        error("setBreakpoint: regNum must be 0,1,2 or 3");
        return;
    }
    if (len > 3)
    {
        error("setBreakpoint: len must be 0,1,2 or 3");
        return;
    }
    if (mode > 3)
    {
        error("setBreakpoint: mode must be 0,1,2 or 3");
        return;
    }

    u32 lenRw = (len << 2) | mode;

    if (regNum == 0)
    {
        lenRw <<= 16;
        asm("   // Set address \n\
                mov %0, %%eax \n\
                mov %%eax, %%dr0 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%eax \n\
                and 0xFFF00FFF, %%eax \n\
                or %1, %%eax \n\
                mov %%eax, %%dr7 \n\
                "::"m"(addr), "m"(lenRw));
    }
    else if (regNum == 1)
    {
        lenRw <<= 20;
        asm("   // Set address \n\
                mov %0, %%eax \n\
                mov %%eax, %%dr1 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%eax \n\
                and 0xFF0F0FFF, %%eax \n\
                or %1, %%eax \n\
                mov %%eax, %%dr7 \n\
                "::"m"(addr), "m"(lenRw));
    }
    else if (regNum == 2)
    {
        lenRw <<= 24;
        asm("   // Set address \n\
                mov %0, %%eax \n\
                mov %%eax, %%dr2 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%eax \n\
                and 0xF0FF0FFF, %%eax \n\
                or %1, %%eax \n\
                mov %%eax, %%dr7 \n\
                "::"m"(addr), "m"(lenRw));
    }
    else if (regNum == 3)
    {
        lenRw <<= 28;
        asm("   // Set address \n\
                mov %0, %%eax \n\
                mov %%eax, %%dr3 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%eax \n\
                and 0x0FFF0FFF, %%eax \n\
                or %1, %%eax \n\
                mov %%eax, %%dr7 \n\
                "::"m"(addr), "m"(lenRw));
    }

    enableBreakpoint(regNum);
}

void enableBreakpoint(u8 regNum)
{
    if (regNum > 3)
    {
        error("enableBreakpoint: regNum must be 0,1,2 or 3");
        return;
    }

    u32 bits = 0b11 << 2*regNum;
    asm("   mov %%dr7, %%eax \n\
            or %0, %%eax \n\
            or 0x700, %%eax \n\
            mov %%eax, %%dr7 \n\
            "::"m"(bits));
}

void disableBreakpoint(u8 regNum)
{
    if (regNum > 3)
    {
        error("disableBreakpoint: regNum must be 0,1,2 or 3");
        return;
    }

    u32 bits = 0xFFFF0FFF - (0b11 << 2*regNum);
    asm("   mov %%dr7, %%eax \n\
            and %0, %%eax \n\
            mov %%eax, %%dr7 \n\
            "::"m"(bits));
}

void clearBreakpointDR6()
{
    asm("   mov 0xFFFF0FF0,%%eax \n\
            mov %%eax,%%dr6"::);
}
