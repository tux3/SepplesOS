#include <std/types.h>
#include <lib/string.h>
#include <vga/vgatext.h>
#include <error.h>
#include <arch/pinio.h>
#include <keyboard.h>
#include <keyboardLayout.h>
#include <power.h>
#include <proc/process.h>
//#include "schedule.h"
//#include <memmap.h>
#include <mm/paging.h>
#include <syscalls.h>
//#include "console.h"
//#include "rtc.h"
#include <arch/pic.h>
#include <debug.h>

extern "C" void isr_default_int(void)
{
    fatalError("Unhandled interrupt\n");
}

extern "C" void isr_reserved_int(void)
{
    fatalError("Unhandled reserved interrupt\n");
}

extern "C" void isr_exc_DIV0(void)
{
    fatalError("Division by 0\n");
}

extern "C" void isr_exc_DEBUG(void)
{
    u64 dr6, nReg;
    u64 rip, addr;
    asm("   mov %%dr6, %%rax \n\
            mov %%rax, %0 \n\
            mov %%rdi, %1":"=m"(dr6),"=m"(rip)::"rdi","rax");
    clearBreakpointDR6();

    if ((dr6 & 0b1111) == 0) // If this wasn't caused by a breakpoint, return
        return;
    else if (dr6 & 0b0001)
    {
        nReg = 0;
        asm("   mov %%dr0, %%rax \n\
                mov %%rax, %0":"=m"(addr));
    }
    else if (dr6 & 0b0010)
    {
        nReg = 1;
        asm("   mov %%dr1, %%rax \n\
                mov %%rax, %0":"=m"(addr));
    }
    else if (dr6 & 0b0100)
    {
        nReg = 2;
        asm("   mov %%dr2, %%rax \n\
                mov %%rax, %0":"=m"(addr));
    }
    else if (dr6 & 0b1000)
    {
        nReg = 3;
        asm("   mov %%dr3, %%rax \n\
                mov %%rax, %0":"=m"(addr));
    }

    TaskManager::disableScheduler();

    disableBreakpoint(nReg);
    VGAText::setCurStyle(VGAText::CUR_RED,false,VGAText::CUR_WHITE);
    VGAText::printf("#DB breakpoint %d (%p) from RIP %p, press enter to disable\n",nReg,addr,rip);
    VGAText::setCurStyle();

    bool IF = gti; sti;
    char key[2];
    Keyboard::nget(&key[0], 1);
    if (key[0] != 0x0A)
        enableBreakpoint(nReg);
    if (!IF) cli;

    TaskManager::enableScheduler();
}

extern "C" void isr_exc_BP(void)
{
    error("Breakpoint interrupt received\n");
}

extern "C" void isr_exc_NOMATH(void)
{
    fatalError("Math coprocessor not available\n");
}

extern "C" void isr_exc_MF(void)
{
    fatalError("Coprocessor segment overrun\n");
}

extern "C" void isr_exc_TSS(void)
{
    fatalError("Invalid TSS\n");
}

extern "C" void isr_exc_SWAP(void)
{
    fatalError("Segment not present in memory, but SWAP is not implemented\n");
}

extern "C" void isr_exc_AC(void)
{
    fatalError("Aligment check exception\n");
}

extern "C" void isr_exc_MC(void)
{
    fatalError("Machine check exception\n");
}

extern "C" void isr_exc_XM(void)
{
    fatalError("SIMD Floating-Point Exception\n");
}

extern "C" void isr_exc_NMI(void)
{
    error("Non-maskable interrupt received\n");
}

extern "C" void isr_exc_OVRFLW(void)
{
    error("Overflow interrupt\n");
}

extern "C" void isr_exc_BOUNDS(void)
{
    fatalError("Bound interrupt\n");
}

extern "C" void isr_exc_OPCODE(void)
{
    u64 eip;
    asm(" 	mov %%rdi, %%rax	\n \
            mov %%rax, %0" : "=m"(eip)::"rdi","rax");
    fatalError("Invalid opcode at %p\n",eip);
}

extern "C" void isr_exc_DOUBLEF(void)
{
    fatalError("Double Fault\n");
}

extern "C" void isr_exc_STACKF(void)
{
    u64 rip, error;
    asm(" 	mov %%rdi, %0		\n \
            mov %%rsi,  %1"
        : "=m"(rip), "=m"(error)::"rdi","rsi");
    fatalError("Stack Fault at RIP %p, error code %p\n",rip, error); // Will halt
}

extern "C" void isr_exc_GP(void)
{
    u64 rip, error;
    asm(" 	mov %%rdi, %0		\n \
            mov %%rsi,  %1"
        : "=m"(rip), "=m"(error)::"rdi","rsi");
    fatalError("GP fault, eip:%p, error:%p\n",rip,error);
}

extern "C" void isr_exc_PF(void)
{
    // Page-fault error code:
    // 31         4    3   2   1  0
    // |Reserved|I/D|RSVD|U/S|W/R|P|
    // P	0 The fault was caused by a non-present page.
    //		1 The fault was caused by a page-level protection violation.
    // W/R	0 The access causing the fault was a read.
    //		1 The access causing the fault was a write.
    // U/S	0 The access causing the fault originated when the processor was executing in supervisor mode.
    //		1 The access causing the fault originated when the processor was executing in user mode.
    // RSVD	0 The fault was not caused by reserved bit violation.
    //		1 The fault was caused by reserved bits set to 1 in a page directory.
    // I/D	0 The fault was not caused by an instruction fetch.
    //		1 The fault was caused by an instruction fetch.
    u64 faulting_addr, errorcode;
    u64 rip;

    asm(" 	mov %%rdi, %0		\n \
            mov %%cr2, %%rax	\n \
            mov %%rax, %1		\n \
            mov %%rsi, %2"
        : "=m"(rip), "=m"(faulting_addr), "=m"(errorcode)::"rdi","rsi","rax");

    u64 pid = TaskManager::getCurrent()->pid;

    if (!pid && faulting_addr < 0x1000)
        fatalError("Page fault in page 0 at %p from rip %p\n",faulting_addr, rip);

    //fatalError("Page fault at %p from rip %p\n",faulting_addr, eip); // Temporary safeguard

    if (pid && faulting_addr >= USER_OFFSET && faulting_addr < USER_STACK) // Allocate memory for the user
    {
        logE9("Page fault in user space, at rip:%p cr2:%p error code:%p\n", rip, faulting_addr, errorcode);
        if (!Paging::mapPage((u8*)(faulting_addr&0xFFFFF000), nullptr, PAGE_ATTR_USER, TaskManager::getCurrent()->pml4))
            fatalError("Couldn't map the faulting page\n");
    }
    else
    {
        error("Page fault (pid:%d,eip:0x%x,cr2:0x%x,error:0x%x)\n",pid, rip, faulting_addr, errorcode);
        sysExit(1);
    }
}

extern "C" void isr_clock_int(void)
{
//    static int tic = 0;
//    static int sec = 0;

    //VGAText::put('.');
//	if (tic==0 && sec==0)
//		globalTerm.print("Chrono : 0s");

//    tic++;
//    if (tic % PIC::getFrequency() == 0) // Every 1s
//    {
//        sec++;
//        tic = 0;

//        if (sec>=10)
//            VGAText::rubout();
//        VGAText::rubout();
//        VGAText::rubout();
//        VGAText::printf("%ds",sec);
//    }

/*	u32 ebp,esp;
    asm(" 	movl %%ebp, %%eax	\n \
            mov %%eax, %0		\n \
        mov %%esp, %%eax	\n \
        mov %%eax, %1"
        : "=m"(ebp), "=m"(esp)); */
    //globalTerm.printf("EBP:0x%x, ESP:0x%x\n",ebp,esp);
    //BOCHSBREAK
    TaskManager::schedule();
}

/// Handles keyboard interrupts
extern "C" void isr_kbd_int(void)
{
    unsigned char i;
    static int lshift_enable=0;
    static int rshift_enable=0;
    static int alt_enable=0;
    static int ctrl_enable=0;

    do
    {
        i = inb(0x64);
    } while ((i & 0x01) == 0);

    i = inb(0x60);
    i--;

    if (i < 0x80) // Key pressed (Codes from 0x0 to 0x79)
    {
        switch (i)
        {
        case 0x00: // ESC
            if (ctrl_enable || alt_enable)        reboot(); // CTRL+ESC to reboot
            halt(); // ESC to halt
            break;
        case 0x29: // LEFT SHIFT
            lshift_enable = 1;
            break;
        case 0x35: // RIGHT SHIFT
            rshift_enable = 1;
            break;
        case 0x1C: // CTRL
            ctrl_enable = 1;
            break;
        case 0x37: // ALT
            alt_enable = 1;
            break;
        case 0x44: // Break
            Paging::printStats();
            Malloc::printStats(true);
            break;
        case 0x48: // Page Up
            VGAText::scroll(-1);
            break;
        case 0x50: // Page Down
            VGAText::scroll(1);
            break;
        default:
            {
                Keyboard::input(kbdmap[i * 4 + (lshift_enable || rshift_enable)]);
            }
        }
    }
    else // Key released (Codes from 0x80 to 0xFF)
    {
        i -= 0x80;
        switch (i)
        {
        case 0x29:
            lshift_enable = 0;
            break;
        case 0x35:
            rshift_enable = 0;
            break;
        case 0x1C:
            ctrl_enable = 0;
            break;
        case 0x37:
            alt_enable = 0;
            break;
        }
    }
}

// Apparently this is not a real interrupt but it results from a race condition PIC/CPU.
extern "C" void isr_spurious_int(void)
{
    //error("Spurious IRQ received\n");
}

extern "C" void isr_rtc_int(void)
{
//	if(read_cmos(0x0C) & 0x40){
//        if(bcd){
//            global_time.second = bcd2bin(read_cmos(0x00));
//            global_time.minute = bcd2bin(read_cmos(0x02));
//            global_time.hour   = bcd2bin(read_cmos(0x04));
//            global_time.month  = bcd2bin(read_cmos(0x08));
//            global_time.year   = bcd2bin(read_cmos(0x09));
//            global_time.day_of_week  = bcd2bin(read_cmos(0x06));
//            global_time.day_of_month = bcd2bin(read_cmos(0x07));
//        }else {
//            global_time.second = read_cmos(0x00);
//            global_time.minute = read_cmos(0x02);
//            global_time.hour   = read_cmos(0x04);
//            global_time.month  = read_cmos(0x08);
//            global_time.year   = read_cmos(0x09);
//            global_time.day_of_week  = read_cmos(0x06);
//            global_time.day_of_month = read_cmos(0x07);
//        }
//    }
}

extern "C" void isr_hd_op_complete(void)
{
    logE9("IRQ 14/15: Hard disk controller operation complete\n");
}
