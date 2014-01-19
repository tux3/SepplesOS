#ifndef MEMMAP_H_INCLUDED
#define MEMMAP_H_INCLUDED

#define	RAM_MAXSIZE	0x100000000
#define	RAM_MAXPAGE	0x100000

#define IDTSIZE				0xFF	// Number of descriptors in the table. Each is 8B
#define GDTSIZE				0xFF	// Number of descriptors in the table. Each is 8B

// Watch out for the reserved RAM. You just can't put data anywhere.
// Check the mbi's map before changing this file
#define	KERN_BASE			0x00000000 // Apply changes in kern makefile (ld -Ttext=KERN_BASE) before compiling !
#define	KERN_STACK_LIM      0x00020000 // Limit of the stack used by the kernel (sepples.ELF is 73kiB as of the 17 jan 14)
#define	KERN_STACK			0x0009C7FF // Stack used by the kernel
#define IDTBASE				0x0009C800	// physical addr of the IDT (2kiB large)
#define GDTBASE				0x0009D000	// physical addr of the GDT (2kiB large)
#define VGA_FRAMEBUFFER     0x000B8000 // Start of the VGA text framebuffer
#define VGA_FRAMEBUFFER_LIM 0x000B8FA0 // End of the VGA text framebuffer (with the default screen size of 80x25)
#define RING0_STACK_LIM     0x00100000 // Must be a multiple of 4096 (0x1000)
#define RING0_STACK			0x0017FFFF // Stack used by tasks with ring0 tss (interrupts)
#define KERN_PAGE_BMP       0x00180000 // 1MiB large if RAM_MAXPAGE == 0x100000
#define	KERN_PAGE_DIR		0x00280000 // 4kiB large. (1024 entries * 4B/entry)
#define KERN_PAGE_TABLES    0x00281000 // 4088kiB large. (1022 * 1024 entries * 4B/entry)
#define KERN_HEAP			0x0067F000
#define KERN_HEAP_LIM		0x40000000

// If you use those again, you'll need to make room for them
//#define KERN_PAGE_HEAP		0x00800000
//#define KERN_PAGE_HEAP_LIM	0x10000000

// All pages between START and END are mapped directly to phisical address spaces
#define KERN_IDENTITY_START	0x00000000
#define KERN_IDENTITY_END	0x0067F000 // Must be 4kiB (pagesize) aligned

#define	USER_OFFSET 		0x40000000 // Apply changes in shell makefile (ld -Ttext=USER_OFFSET) before compiling !
#define	USER_STACK 			0xE0000000

#endif

/** Sample MBI memory maps

=> Old laptop
==> Type 1 : 0x0 - 0x9FC00 and 0x00100000 - 0x1DFD0000
Type 1, base 0x00000000, size 0x0009FC00
Type 2, base 0x0009FC00, size 0x00000400
Type 2, base 0x000E0000, size 0x00020000
Type 1, base 0x00100000, size 0x1DED0000
Type 3, base 0x1DFD0000, size 0x0000F000
Type 4, base 0x1DFDF000, size 0x00021000
Type 2, base 0xFFF80000, size 0x00080000

=> Bochs
==> Type 1 : 0x0 - 0x9F000 and 0x00100000 - 0x0FFF0000
Type 1, base 0x00000000, size 0x0009F000
Type 2, base 0x0009F000, size 0x00001000
Type 2, base 0x000E8000, size 0x00018000
Type 1, base 0x00100000, size 0x0FEF0000
Type 3, base 0x0FFF0000, size 0x00010000
Type 2, base 0xFFFC0000, size 0x00040000

=> New laptop
Type 1, 0x0 - 0x9D800
Type 2, 0x9D800 - 0xA0000
Type 2, 0xE0000 - 0x100000
Type 1, 0x100000 - 0x20000000
Type 2, 0x20000000 - 0x20200000
Type 1, 0x20200000 - 0x40004000
Type 2, 0x40004000 - 0x40005000
Type 1, 0x40005000 - 0xD97DB000
Type 2, 0xD97DB000 - 0xD9F42000
Type 1, 0xD9F42000 - 0xD9FCB000
Type 4, 0xD9FCB000 - 0xDA06A000
Type 2, 0xDA06A000 - 0xDA560000
Type 1, 0xDA560000 - 0xDA561000
Type 4, 0xDA561000 - 0xDA5A4000
Type 1, 0xDA5A4000 - 0xDAD42000
Type 2, 0xDAD42000 - 0xDAFF3000
Type 1, 0xDAFF3000 - 0xDB000000
Type 2, 0xDB800000 - 0xDFA00000
Type 2, 0xF8000000 - 0xFC000000
Type 2, 0xFEC00000 - 0xFEC01000
Type 2, 0xFED00000 - 0xFED04000
Type 2, 0xFED1C000 - 0xFED20000
Type 2, 0xFEE00000 - 0xFEE01000
Type 2, 0xFF000000 - 0x100000000
Type 1, 0x100000000 - 0x21E600000
**/
