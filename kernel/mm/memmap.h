;/*
%if 0;*/

//This file contains both ASM and C defines, they are centralized to make editing easier

#ifndef MEMMAP_H
#define MEMMAP_H

#define PML4_ADDR 0x1000UL
#define PDPT0_ADDR 0x2000UL
#define PD0_ADDR 0x3000UL
#define PT0_ADDR 0x4000UL

#define GDTBASE 0x5000UL
#define GDTSIZE 0xFF

#define IDTBASE 0x5800UL
#define IDTSIZE 0xFF // Number of 8 bytes descriptors in the table

#define RING0_STACK_LIM     0x6000UL // Must be a multiple of 4096 (0x1000)
#define RING0_STACK         0x6FF8UL // Stack used by tasks with ring0 tss (interrupts)

#define TMPMAP_PAGE_VADDR   0x7000UL // Temporary map page

#define VGA_TEXT_START 0xB8000UL

#define FREE_PAGES_START 0x100000UL

#define	USER_OFFSET 		0x40000000UL // A lot of code assumes that this value will not change. Make sure to review it before changing it
#define	USER_STACK 			0xE0000000UL

/* Start of ASM defines
%endif

%define PAGESIZE						0x1000

%define PML4_ADDR						0x1000
%define PDPT0_ADDR						0x2000
%define	PD0_ADDR						0x3000
%define PT0_ADDR						0x4000

%define GDT_ADDR						0x5000
%define IDT_ADDR						0x5800
%define KERN_STACK						0x7FFF8

%define VGATEXT_FRAMEBUFFER_VADDR		0xB8000
%define VGATEXT_FRAMEBUFFER_PADDR		0xB8000

%if 0;*/

#endif // MEMMAP_H

;/*
%endif;*/
