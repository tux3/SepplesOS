global asmboot
extern boot, initVGABIOS, initVGA, print16VGA, memzeroasm, setupGDT

%include "kernel/mm/memmap.h"

%define MULTIBOOT_HEADER_MAGIC			0x1BADB002
%define MULTIBOOT_HEADER_FLAGS			0b110
%define MULTIBOOT_HEADER_CHECKSUM -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
%define MULTIBOOT_HEADER_HEADER_ADDR	0 ; Used if flags[16] is set
%define MULTIBOOT_HEADER_LOAD_ADDR		0 ; Used if flags[16] is set
%define MULTIBOOT_HEADER_LOAD_END_ADDR	0 ; Used if flags[16] is set
%define MULTIBOOT_HEADER_BSS_END_ADDR	0 ; Used if flags[16] is set
%define MULTIBOOT_HEADER_ENTRY_ADDR		0 ; Used if flags[16] is set
%define MULTIBOOT_HEADER_MODE_TYPE		1 ; Used if flags[2] is set
%define MULTIBOOT_HEADER_WIDTH			80 ; Used if flags[2] is set
%define MULTIBOOT_HEADER_HEIGHT			25 ; Used if flags[2] is set
%define MULTIBOOT_HEADER_DEPTH			0 ; Used if flags[2] is set


section .text
; Remember that this code has to be valid x86 AND x86_64 until we switch to x64 !
; That means no legacy size prefixes or REX prefixes!

align 4
multiboot_header:
dd MULTIBOOT_HEADER_MAGIC
dd MULTIBOOT_HEADER_FLAGS
dd MULTIBOOT_HEADER_CHECKSUM
dd MULTIBOOT_HEADER_HEADER_ADDR
dd MULTIBOOT_HEADER_LOAD_ADDR
dd MULTIBOOT_HEADER_LOAD_END_ADDR
dd MULTIBOOT_HEADER_BSS_END_ADDR
dd MULTIBOOT_HEADER_ENTRY_ADDR
dd MULTIBOOT_HEADER_MODE_TYPE
dd MULTIBOOT_HEADER_WIDTH
dd MULTIBOOT_HEADER_HEIGHT
dd MULTIBOOT_HEADER_DEPTH

endOfCodeErrorStr:
db "boot.asm: Kernel returned unexpectedly!",0
invalidCPUStateErrorStr:
db "The bootloader left the CPU in an invalid state, can't boot",0
badCPUErrorStr:
db "Your CPU does not have the required features, can't boot",0

; Expects a zero-terminated string in RDI
; DOES NOT preserve RDI
BITS 64
print:
	mov eax, VGATEXT_FRAMEBUFFER_VADDR
	mov ch, 15
.printloop:
	mov cl, [rdi]
	mov [rax], ecx
	add eax, 2
	inc edi
	cmp cl, 0
	jne .printloop
	ret

; Entry point
BITS 64
asmboot:
	cli

	; Detect if we have 16bit operands by default
	; If so, the bootloader started us wrong, so
	; we halt with an error message
	; This code uses the stack to save/restore RAX
	db 0x50, 0xEB, 0x03, 0xE9
	dw invalidCPUState16-$-2
	db 0x35, 0x00, 0x00, 0xEB, 0xF8, 0x58

	; Save some multiboot info
	mov esp, KERN_STACK
	mov ebp, esp
	push rax ; Magic of multiboot_info
	push rbx ; Adress of multiboot_info

	call setupGDT

BITS 32
.switch_to_real:
	; Use the BIOS's IVT
	mov eax, PML4_ADDR ; We don't page yet, so can use that memory
	mov [eax], word 0x3FF
	mov [eax+2], dword 0
	lidt [eax]

	; Use our real-mode data segments
	mov eax, 8*4
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	mov ss, eax
	mov eax, esp
	mov esp, RING0_STACK ; Steal the interrupts's stack for now
	push eax

	; Patch .switch_to_real_farjmp2 depending on CS
	mov ebx, .switch_to_real_farjmp2
	mov [ebx+3], word 8*5
	mov edx, .switch_to_real_cont2
	mov eax, 8*5
	shl eax, 4
	sub edx, eax
	mov [ebx+1], dx

	; Jump to our new 16bit code segment
	db 0xEA
	dd .switch_to_real_cont
	dw 8*5
BITS 16
.switch_to_real_cont:
	; Go to real mode
	mov eax, cr0
	and al, ~1
	mov cr0, eax

	; Far jump to .switch_to_real_cont, to force reload
	jmp .switch_to_real_farjmp2 ; Sync pipeline after rewriting the jmp
.switch_to_real_farjmp2:
	db 0xEA
	dw .switch_to_real_cont2
	dw 8*5
.switch_to_real_cont2:
	; Switch to zero data segments
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax


BITS 16
.real_mode_setup:
	call initVGABIOS

BITS 16
.switch_to_protected:
	; Disable interrupts
	cli
	mov eax, PML4_ADDR ; We don't pages yet, so can use that memory
	mov [eax], word 0
	mov [eax+2], dword 0
	lidt [eax]

	; Prepare CS:IP for the switch
.switch_to_protected_farjmp:
	db 0xEA
	dw .switch_to_protected_cont-(8*3*16)
	dw 8*3
.switch_to_protected_cont:

	; Switch the PE bit
	mov eax, cr0
	or al, 1
	mov cr0, eax

	; Switch to our 32bit code segment
.switch_to_protected_farjmp2:
	db 0xEA
	dw .switch_to_protected_cont2
	dw 8*3
BITS 32
.switch_to_protected_cont2:
	; Use our protected-mode data segments
	mov eax, 8*1
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	mov ss, eax
	pop esp

BITS 32
.protected:
	; Check that the CPU supports 0x80000001 extended CPUID
	pushf
	pop eax
	mov ecx, eax
	xor eax, 1 << 21
	push eax
	popf
	pushf
	pop eax
	push ecx
	popf
	xor eax, ecx
	jz badCPU
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb badCPU

	; Check that the CPU supports IA32e long mode and MSRs
	mov eax, 0x80000001
	cpuid
	test edx, 1 << 29
	jz badCPU
	mov eax, 0x1
	cpuid
	test edx, 1 << 5
	jz badCPU

	; Check that we're not already in long mode
	mov ecx,0xC0000080
	rdmsr
	test eax,0x500
	jnz invalidCPUState

	call initVGA

	; Disable paging, enable PAE
	mov ecx, cr0
	and ecx, 0x7FFFFFFF
	mov cr0, ecx
	mov ecx, cr4
	or ecx, 0b100000
	mov cr4, ecx

	; Set up PML4 table
	mov edi, PML4_ADDR
	mov eax, PDPT0_ADDR | 0b11
	mov [edi], eax
	add edi, 8
	mov ecx, 0xFF8
	call memzeroasm

	; Set up PDPT 0
	mov edi, PDPT0_ADDR
	mov eax, PD0_ADDR | 0b11
	mov [edi], eax
	add edi, 8
	mov ecx, 0xFF8
	call memzeroasm

	; Set up PD 0
	mov edi, PD0_ADDR
	mov eax, PT0_ADDR | 0b11
	mov [edi], eax
	add edi, 8
	mov ecx, 0xFF8
	call memzeroasm

	; Set up PT 0 to identity-map 0x1000-0x7FFFF
	mov edi, PT0_ADDR
	mov ecx, PAGESIZE
	call memzeroasm
	mov ecx, 0x7F
	mov edi, PT0_ADDR + 8
	mov eax, PAGESIZE | 0b11
	mov edx, PAGESIZE
fillPT0:
	mov [edi], eax
	add eax, edx
	add edi, 8
	loop fillPT0

	; Map the VGA text framebuffer
	mov edi, PT0_ADDR + VGATEXT_FRAMEBUFFER_PADDR/PAGESIZE*8
	mov eax, VGATEXT_FRAMEBUFFER_PADDR | 0b11
	mov [edi], eax

	; Enable IA32e mode, enable paging, enable syscalls instruction (SCE)
	mov eax, PML4_ADDR
	mov cr3, eax
	mov ecx,0xC0000080
	rdmsr
	or eax,0x101
	wrmsr
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	; Save stuff from our stack before reloading
	pop esi ; Adress of multiboot_info
	pop edi ; Magic of multiboot_info

	; Reload our segments
	push ecx
	push esp
	pushf
	push 8*2 ; Code segment
	push reloadCS
	xor ecx, ecx
	mov cx, 8*1 ; Data segment
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	mov ss, cx
	iret
reloadCS:

	; Jump to kernel
	call boot

endOfCode:
	; We shouldn't reach this point
	mov edi, endOfCodeErrorStr

fatalError:
	call print

halt:
	cli
	hlt

invalidCPUState:
	mov edi, invalidCPUStateErrorStr
	jmp fatalError

badCPU:
	mov edi, badCPUErrorStr
	jmp fatalError

BITS 16
invalidCPUState16:
	mov di, invalidCPUStateErrorStr
	call print16VGA
	jmp halt


