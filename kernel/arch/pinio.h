#include <std/types.h>

// Clear/set/get IF
#define cli asm("cli"::)
#define sti asm("sti"::)
#define gti ({ \
            u8 _IF; \
            asm("pushf;pop %%rax;and $0x200,%%rax;shr $9,%%rax;mov %%al,%0":"=m"(_IF)::"rax"); \
            _IF; \
            })

// Write a byte on a port and wait
#define outb(port,value) \
    asm volatile ("outb %%al, %%dx;" :: "d" (port), "a" (value));

// Write a byte on a port, saves and restores registers
#define outbsave(port,value) \
    asm volatile ("push %%rax; push %%rdx; mov %1, %%al; mov %0, %%dx; \
                    outb %%al, %%dx; pop %%rdx; pop %%rax"::"g"(port),"g"(value));

// Write a byte on a port and wait
#define outbp(port,value) \
    asm volatile ("outb %%al, %%dx; jmp 1f; 1:" :: "d" (port), "a" (value));

// Read a byte from a port
#define inb(port) ({    \
    unsigned char _v;       \
    asm volatile ("inb %%dx, %%al;":"=a"(_v):"d"(port)); \
        _v;     \
})

// Write a word (2B) on a port
#define outw(port,value) \
    asm volatile ("outw %%ax, %%dx" :: "d" (port), "a" (value));

// Read a word (2B) from a port
#define inw(port) ({		\
    u16 _v;			\
    asm volatile ("inw %%dx, %%ax" : "=a" (_v) : "d" (port));	\
        _v;			\
})

// Read from a CMOS register (ports 0x70/0x71)
#define read_cmos(reg) ({		\
    unsigned char _v;			\
    outb(0x70, reg);			\
    _v = inb(0x71);				\
        _v;						\
})

// Write to a CMOS register (ports 0x70/0x71)
#define write_cmos(reg,value) ({		\
    outb(0x70, reg);					\
    outb(0x71, value);					\
})
