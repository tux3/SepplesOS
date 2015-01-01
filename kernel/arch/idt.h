/*

Man: IDT entries - IRQs

IDT 0-31 : Exceptions
	IRQ 0-7 = IDT 8-15

IDT 32-255 : The rest
	IRQ 0 = System timer (sheduler ticks)
	IRQ 1 = Keyboard
	IRQ 5 = Sound card
	IRQ 6 = Floppy disk controller
	IRQ 8 = RTC from CMOS (Real Time Clock)
	IRQ 9-11 = Open IRQ (anything)
	IRQ 12 = PS/2 mouse
	IRQ 14 = Primary IDE controller
	IRQ 15 = Secondary IRQ controller
	IRQ 8-15 = IDT 112-120

*/

#ifndef IDT_H_INCLUDED
#define IDT_H_INCLUDED

#include <std/types.h>

#define INTGATE  0x8E00		// For 'regular' interrupts, can't be preempted
#define TRAPGATE 0xEF00		// For syscalls, can be preempted (by another interrupt)

// Segment descriptor. Binds an IRQ (coming from the PIC, by the CPU), to an ISR
struct IDTDesc
{
	u16 offset0_15;
	u16 select;
	u16 type;
	u16 offset16_31;
    u32 offset32_63;
    u32 reserved;
} __attribute__ ((packed));

// IDT register struct
struct IDTR
{
	u16 limite;
    u64 base;
} __attribute__ ((packed));


class IDT
{
    public:
        IDT() = delete;
        static void initDescriptor(u16, u64, u16, struct IDTDesc *);
        static void init();

    private:
        static IDTR idtr; // IDT register
        static IDTDesc* idt; // IDT
};

extern "C" void _asm_default_int(void);
extern "C" void _asm_reserved_int(void);
extern "C" void _asm_exc_DIV0(void);
extern "C" void _asm_exc_DEBUG(void);
extern "C" void _asm_exc_BP(void);
extern "C" void _asm_exc_NOMATH(void);
extern "C" void _asm_exc_MF(void);
extern "C" void _asm_exc_TSS(void);
extern "C" void _asm_exc_SWAP(void);
extern "C" void _asm_exc_AC(void);
extern "C" void _asm_exc_MC(void);
extern "C" void _asm_exc_XM(void);
extern "C" void _asm_exc_NMI(void);
extern "C" void _asm_exc_OVRFLW(void);
extern "C" void _asm_exc_BOUNDS(void);
extern "C" void _asm_exc_OPCODE(void);
extern "C" void _asm_exc_DOUBLEF(void);
extern "C" void _asm_exc_STACKF(void);
extern "C" void _asm_exc_GP(void);
extern "C" void _asm_exc_PF(void);
extern "C" void _asm_irq_0(void);
extern "C" void _asm_irq_1(void);
extern "C" void _asm_irq_7(void);
extern "C" void _asm_irq_8(void);
extern "C" void _asm_irq_hd_op_complete(void);
extern "C" void _asm_syscalls(void);

#endif // IDT_H_INCLUDED
