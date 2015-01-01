#include <debug.h>
#include <error.h>
#include <arch/pinio.h>

void logE9(char c)
{
    outb(0xE9, c);
}

void logE9(const char* msg)
{
    while (*msg)
        logE9(*msg++);
}

void setBreakpoint(u8 regNum, u64 addr, BreakpointLength len, BreakpointMode mode)
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

    if (mode == BP_INST && len != BP_BYTE)
    {
        error("setBreakpoint: In BP_INST mode the length must be BP_BYTE. Breakpoint not set.\n");
        return;
    }

    u64 lenRw = (len << 2) | mode;

    if (regNum == 0)
    {
        lenRw <<= 16;
        u64 bits = 0xFFF00FFF;
        asm("   // Set address \n\
                mov %0, %%rax \n\
                mov %%rax, %%dr0 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%rax \n\
                and %2, %%rax \n\
                or %1, %%rax \n\
                mov %%rax, %%dr7 \n\
                "::"m"(addr), "m"(lenRw),"m"(bits));
    }
    else if (regNum == 1)
    {
        lenRw <<= 20;
        u64 bits = 0xFF0F0FFF;
        asm("   // Set address \n\
                mov %0, %%rax \n\
                mov %%rax, %%dr1 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%rax \n\
                and %2, %%rax \n\
                or %1, %%rax \n\
                mov %%rax, %%dr7 \n\
                "::"m"(addr), "m"(lenRw), "m"(bits));
    }
    else if (regNum == 2)
    {
        lenRw <<= 24;
        u64 bits = 0xF0FF0FFF;
        asm("   // Set address \n\
                mov %0, %%rax \n\
                mov %%rax, %%dr2 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%rax \n\
                and %2, %%rax \n\
                or %1, %%rax \n\
                mov %%rax, %%dr7 \n\
                "::"m"(addr), "m"(lenRw), "m"(bits));
    }
    else if (regNum == 3)
    {
        lenRw <<= 28;
        asm("   // Set address \n\
                mov %0, %%rax \n\
                mov %%rax, %%dr3 \n\
                // Set R/W and LEN \n\
                mov %%dr7, %%rax \n\
                and $0x0FFF0FFF, %%rax \n\
                or %1, %%rax \n\
                mov %%rax, %%dr7 \n\
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

    u64 bits = 0b11 << 2*regNum;
    asm("   mov %%dr7, %%rax \n\
            or %0, %%rax \n\
            or $0x700, %%rax \n\
            mov %%rax, %%dr7 \n\
            "::"m"(bits));
}

void disableBreakpoint(u8 regNum)
{
    if (regNum > 3)
    {
        error("disableBreakpoint: regNum must be 0,1,2 or 3");
        return;
    }

    u64 bits = 0xFFFF0FFF - (0b11 << 2*regNum);
    asm("   mov %%dr7, %%rax \n\
            and %0, %%rax \n\
            mov %%rax, %%dr7 \n\
            "::"m"(bits));
}

void clearBreakpointDR6()
{
    asm("   mov $0xFFFF0FF0,%%rax \n\
            mov %%rax,%%dr6"::);
}
