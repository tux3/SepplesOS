#include <std/types.h>

// CLear/set/get IF
#define cli asm("cli"::)
#define sti asm("sti"::)
#define gti ({ \
			bool _IF; \
			asm("pushfl;popl %%eax;andl $0x200,%%eax;shr $9,%%eax;mov %%al,%0":"=a"(_IF):); \
			_IF; \
			})

// Write a byte on a port
#define outb(port,value) \
	asm volatile ("outb %%al, %%dx" :: "d" (port), "a" (value));

// Write a byte on a port and wait
#define outbp(port,value) \
	asm volatile ("outb %%al, %%dx; jmp 1f; 1:" :: "d" (port), "a" (value));

// Read a byte from a port
#define inb(port) ({    \
	unsigned char _v;       \
	asm volatile ("inb %%dx, %%al" : "=a" (_v) : "d" (port)); \
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
