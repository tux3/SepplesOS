global asmboot
extern boot, initVGA, memzeroasm

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
print:
	mov eax, VGATEXT_FRAMEBUFFER_VADDR
	mov ch, 15
	printloop:
	mov cl, [rdi]
	mov [rax], ecx
	add eax, 2
	inc edi
	cmp cl, 0
	jne printloop
	ret

; Entry point
asmboot:
	cli
	mov esp, KERN_STACK
	mov ebp, esp
	push rax ; Magic of multiboot_info
	push rbx ; Adress of multiboot_info

	; Check that we're not in real mode or virtual 8086 mode
	mov rcx, cr0
	and rcx, 0x1
	jz invalidCPUState
	pushf
	pop rax
	test rax, 1 << 17
	jnz invalidCPUState

	; Check that the CPU supports 0x80000001 extended CPUID
	pushf
	pop rax
	mov ecx, eax
	xor eax, 1 << 21
	push rax
	popf
	pushf
	pop rax
	push rcx
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
	mov rcx, cr0
	and ecx, 0x7FFFFFFF
	mov cr0, rcx
	mov rcx, cr4
	or ecx, 0b100000
	mov cr4, rcx

	; Set up PML4 table
	mov edi, PML4_ADDR
	mov eax, PDPT0_ADDR | 0b11
	mov [rdi], eax
	add edi, 8
	mov ecx, 0xFF8
	call memzeroasm

	; Set up PDPT 0
	mov edi, PDPT0_ADDR
	mov eax, PD0_ADDR | 0b11
	mov [rdi], eax
	add edi, 8
	mov ecx, 0xFF8
	call memzeroasm

	; Set up PD 0
	mov edi, PD0_ADDR
	mov eax, PT0_ADDR | 0b11
	mov [rdi], eax
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
	mov [rdi], eax
	add eax, edx
	add edi, 8
	loop fillPT0

	; Map the VGA text framebuffer
	mov edi, PT0_ADDR + VGATEXT_FRAMEBUFFER_PADDR/PAGESIZE*8
	mov eax, VGATEXT_FRAMEBUFFER_PADDR | 0b11
	mov [rdi], eax

	; Get the current descriptor for SS, if we changed its limit then SS:[ESP]
	; would point to a different location after reloading SS and we couldn't iret safely
	xor eax, eax
	xor ecx, ecx
	sgdt [rax]
	mov eax, [rax+2]
	mov cx, ss
	add eax, ecx
	mov esi, [rax]
	mov edx, [rax+4]
	push rdx

	; Set up an initial GDT with x64 code/data segments
	; The data segment is the same as the old SS that we fetched earlier
	mov edi, GDT_ADDR
	mov ebx, edi
	mov ecx, 0x800
	call memzeroasm
	mov eax, 0xFFFF
	mov [rbx+8], eax
	mov [rbx+16], esi
	mov eax, 0xAF9B00 ; 64bit code segment
	mov [rbx+12], eax
	pop rsi
	mov [rbx+20], esi
	xor eax, eax
	mov cx, 0x100
	mov [rax], cx
	mov [rax+2], ebx
	lgdt [rax]

	; Enable IA32e mode, enable paging, enable syscalls instruction (SCE)
	mov eax, PML4_ADDR
	mov cr3, rax
	mov ecx,0xC0000080
	rdmsr
	or eax,0x101
	wrmsr
	mov rax, cr0
	or eax, 0x80000000
	mov cr0, rax

	; Save stuff from our stack before reloading
	pop rsi ; Adress of multiboot_info
	pop rdi ; Magic of multiboot_info

	; Reload our segments
	push rcx
	push rsp
	pushf
	push 0x8 ; Code segment
	push reloadCS
	xor ecx, ecx
	mov cx, 0x10 ; Data segment
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	mov ss, cx
	iret
reloadCS:

	; Jump to kernel
	call boot

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
