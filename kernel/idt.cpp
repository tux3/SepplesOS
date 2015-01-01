#include <idt.h>
#include <io.h>
#include <std/types.h>
#include <paging.h>

IDT::IDT()
{
    idtr.limite = IDTSIZE * 8;
    idtr.base = IDTBASE;
}

/// Init an IDT descriptor. Type can be INTGATE, TRAPGATE or TASKGATE
void IDT::initDescriptor(u16 select, u32 offset, u16 type, struct IDTDesc *desc)
{
    desc->offset0_15 = (offset & 0xffff);
    desc->select = select;
    desc->type = type;
    desc->offset16_31 = (offset & 0xffff0000) >> 16;
}

/// Init the IDT with suitable values
void IDT::init()
{
    // Init default vectors (reserved)
    for (int i = 0; i < 32; i++)
        initDescriptor(0x08, (u32) _asm_reserved_int, INTGATE, &idt[i]);

    // Init default vectors (user-defined)
    for (int i = 32; i < IDTSIZE; i++)
        initDescriptor(0x08, (u32) _asm_default_int, INTGATE, &idt[i]);

    // Vectors 0 to 31 are reserved for exceptions/CPU interrupts (15 is reserver, 9 is used by i386 and before)
    initDescriptor(0x08, (u32) _asm_exc_DIV0, INTGATE, &idt[0]); // #DivisionError
    initDescriptor(0x08, (u32) _asm_exc_DEBUG, INTGATE, &idt[1]); // #Debug
    initDescriptor(0x08, (u32) _asm_exc_NMI, INTGATE, &idt[2]); // #NMI
    initDescriptor(0x08, (u32) _asm_exc_BP, INTGATE, &idt[3]); // #Breakpoint
    initDescriptor(0x08, (u32) _asm_exc_OVRFLW, INTGATE, &idt[4]); // #OVERFLOW
    initDescriptor(0x08, (u32) _asm_exc_BOUNDS, INTGATE, &idt[5]); // #Bounds
    initDescriptor(0x08, (u32) _asm_exc_OPCODE, INTGATE, &idt[6]); // #Invalid Opcode
    initDescriptor(0x08, (u32) _asm_exc_NOMATH, INTGATE, &idt[7]); // #No Math Coprocessor
    initDescriptor(0x08, (u32) _asm_exc_DOUBLEF, INTGATE, &idt[8]); // #Double Fault
    initDescriptor(0x08, (u32) _asm_exc_MF, INTGATE, &idt[9]); // #CoProcessor Segment Overrun (not used after the i386)
    initDescriptor(0x08, (u32) _asm_exc_TSS, INTGATE, &idt[10]); // #Invalid TSS
    initDescriptor(0x08, (u32) _asm_exc_SWAP, INTGATE, &idt[11]); // #Segment not present in memory (used for SWAP-ing ram)
    initDescriptor(0x08, (u32) _asm_exc_STACKF, INTGATE, &idt[12]); // #Stack Fault (stack operation and ss load)
    initDescriptor(0x08, (u32) _asm_exc_GP, INTGATE, &idt[13]);	// #Global Protection Fault
    initDescriptor(0x08, (u32) _asm_exc_PF, INTGATE, &idt[14]); // #Page Fault
    initDescriptor(0x08, (u32) _asm_exc_MF, INTGATE, &idt[16]); // #Math fault (floating point)
    initDescriptor(0x08, (u32) _asm_exc_AC, INTGATE, &idt[17]); // #Alignment check
    initDescriptor(0x08, (u32) _asm_exc_MC, INTGATE, &idt[18]); // #Machine check
    initDescriptor(0x08, (u32) _asm_exc_XM, INTGATE, &idt[19]); // #SIMD floating-point exception

    // Hardware interrupt vectors
    initDescriptor(0x08, (u32) _asm_irq_0, INTGATE, &idt[32]);	// Scheduler clock (regular ticks)
    initDescriptor(0x08, (u32) _asm_irq_1, INTGATE, &idt[33]);	// Keyboard
    initDescriptor(0x08, (u32) _asm_irq_7, INTGATE, &idt[39]);	// "Fake" interrupt (spurious irq after a race condition PIC/CPU)
    initDescriptor(0x08, (u32) _asm_irq_8, INTGATE, &idt[112]);	// System clock (date and time)
    initDescriptor(0x08, (u32) _asm_irq_14, INTGATE, &idt[118]); // Unknow interrup that fires at startup. TODO: Find what it is and handle it properly.

    // System calls - int 0x30 (48d)
    initDescriptor(0x08, (u32) _asm_syscalls, TRAPGATE, &idt[48]);  // syscalls

    // Copy the IDT and load it
    memcpy((char *) idtr.base, (char *) idt, idtr.limite);
    asm("lidtl %0": "=m" (idtr):);
}
