#include <std/types.h>
#include <lib/string.h>
#include <screen.h>
#include <error.h>
#include <io.h>
#include <keyboard.h>
#include <keyboardLayout.h>
#include <power.h>
//#include <process.h>
//#include "schedule.h"
#include <memmap.h>
#include <paging.h>
//#include <syscalls.h>
#include <error.h>
//#include "console.h"
//#include "rtc.h"
//#include <pic.h>

extern "C" void isr_default_int(void)
{
	fatalError("Unhandled interrupt\n");
}

extern "C" void isr_reserved_int(void)
{
	error("Unhandled reserved interrupt\n");
}

extern "C" void isr_exc_DIV0(void)
{
	fatalError("Division by 0\n");
}

extern "C" void isr_exc_DEBUG(void)
{
	error("Debug interrupt received\n");
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
	fatalError("Bound interrupt\n"); // Will halt
}

extern "C" void isr_exc_OPCODE(void)
{
	fatalError("Invalid opcode\n"); // Will halt
}

extern "C" void isr_exc_DOUBLEF(void)
{
	fatalError("Double Fault\n"); // Will halt
}

extern "C" void isr_exc_STACKF(void)
{
	fatalError("Stack Fault\n"); // Will halt
}

extern "C" void isr_exc_GP(void)
{
	u32 eip, error;
	asm(" 	movl 60(%%ebp), %%eax	\n \
    		mov %%eax, %0		\n \
 		movl 56(%%ebp), %%eax	\n \
    		mov %%eax, %1"
		: "=m"(eip), "=m"(error));
	fatalError("GP fault, eip:0x%x, error:0x%x\n",eip,error); // Will halt
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
/*	u32 faulting_addr, error;
	u32 eip;
	struct page *page;

 	asm(" 	movl 60(%%ebp), %%eax	\n \
    		mov %%eax, %0		\n \
		mov %%cr2, %%eax	\n \
		mov %%eax, %1		\n \
 		movl 56(%%ebp), %%eax	\n \
    		mov %%eax, %2"
		: "=m"(eip), "=m"(faulting_addr), "=m"(error));

	if (faulting_addr >= USER_OFFSET && faulting_addr < USER_STACK) // Allocate memory for the user
	{
		//error("Page fault in user space, on eip:0x%x cr2:0x%x error code:0x%x\n", eip, faulting_addr, error);
		page = (struct page *) globalPaging.kmalloc(sizeof(struct page));
		page->pAddr = globalPaging.getPageFrame();
		page->vAddr = (char *) (faulting_addr & 0xFFFFF000);

		globalTaskManager.getCurrent()->pageList << page;
		globalPaging.pageDirAddPage(page->vAddr, page->pAddr, PAGE_USER, globalTaskManager.getCurrent()->pageDir);
	}
	else
	{
		error("Page fault (pid:%d,eip:0x%x,cr2:0x%x,error:0x%x)\n",globalTaskManager.getCurrent()->pid, eip, faulting_addr, error);
		Dusk::sys_exit(1);
	} */
}

extern "C" void isr_clock_int(void)
{
//	static int tic = 0;
//	static int sec = 0;

//	//globalTerm.put('.');
//	if (tic==0 && sec==0)
//		globalTerm.print("Chrono : 0s");

//	tic++;
//	if (tic % globalPic.getFrequency() == 0) // Every 1s
//	{
//		sec++;
//		tic = 0;

//		if (sec>=10)
//			globalTerm.rubout();
//		globalTerm.rubout();
//		globalTerm.rubout();
//		globalTerm.printf("%ds",sec);


//	}
	u32 ebp,esp;
	asm(" 	movl %%ebp, %%eax	\n \
    		mov %%eax, %0		\n \
		mov %%esp, %%eax	\n \
		mov %%eax, %1"
		: "=m"(ebp), "=m"(esp));
	//globalTerm.printf("EBP:0x%x, ESP:0x%x\n",ebp,esp);
	//BOCHSBREAK
	//globalTaskManager.schedule(); // Every clock int, call the scheduler
}

/// Handles keyboard interrupts
extern "C" void isr_kbd_int(void)
{
	unsigned char i;
	static int lshift_enable;
	static int rshift_enable;
	static int alt_enable;
	static int ctrl_enable;

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
            if (ctrl_enable)        reboot(); // CTRL+ESC to reboot
            else if (alt_enable)    halt(); // ALT+ESC to halt
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
        case 0x48: // Page Up
            gTerm.scroll(-1);
            break;
        case 0x50: // Page Down
            gTerm.scroll(1);
            break;
		default:
            {
				gKbd.input(kbdmap[i * 4 + (lshift_enable || rshift_enable)]);
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

/// Apparently this is not a real interrupt but it results from a race condition PIC/CPU. Do nothing.
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

/// Unknown IRQ (irq 14)
extern "C" void isr_14_int(void)
{
	//error("IRQ 14 received. Handler not implemented.\n");
}
