global setupGDT, writeDescriptor

%include "kernel/mm/memmap.h"

;; Layout of the GDT (each index is a 8 byte entry) :
;0 : Null descriptor
;1 : Long/Protected mode data/stack segment, covers all memory
;2 : Long mode 64 bit code segment
;3 : Long/Protected mode 32 bit code segment
;4 : Real mode data segment
;5 : Real mode code segment
;6 : User mode data segment
;7 : User mode stack segment
;8 : User mode 64bit code segment
;9 : User mode 32bit code segment
;10-11 : Default TSS
;12 : Default TSS stack segment
;13 : Null-paded GDTR

;; Segment selector:
; 15   3   2  1 0
; |Index| TI |RPL|

;; Segment descriptor:
; 31         24 23  22  21   20   19         16 15  14 13  12 11  10  9   8 7          0
; | Base 31:24 | G | D | L | AVL | Limit 19:16 | P | DPL | S |D/C|E/C|W/R|A| Base 23:16 |    4
; 31                                         16 15                                     0
; |               Base Address 15:00           |         Segment Limit 15:00            |    0
;

; Base : Segment base address (not used for expand-down segment)
; Limit : Segment limit (in bytes or 4Kio units, depending on granularity G)
; Acces :
; 		P	Segment present
; 		DPL	Descriptor privilege level (0-3)
; 		S	Not-System segment flag (0 for TSS/etc, 1 for code/data/stack)
; 		D/C 0=Data segment, 1=Code segment
;		E/C 1=Expand-down data/Conforming code
; 		W/R 0=Read-only data/Execute-only code, 1=Read-write data/Execute-read code
;		A	1=Accessed by CPU (CPU will set this flag on segment loading), 0=Not accesed by processor (since last time we cleared the flag)
; Other :	 3  2  1  0
; 			|G|D/B|L|AVL|
; 		G : Granularity (0=Byte, 1=4Kio-units)
; 		D/B : 1=32-bit segment, 0=16-bit
;		L : 64-bit code segment
;		AVL : Undefined/Available, we can use this bit for whatever we want

; Will write a general-purpose GDT
; to its assigned physical address (see the memory map)
; and load it in GDTR
; Will NOT setup or load the TSS, although space is reserved for a descriptor
BITS 32
setupGDT:
	mov eax, GDT_ADDR

	;0
	mov [eax+4*0], dword 0
	mov [eax+4*1], dword 0

	;1
	mov [eax+4*2], dword 0x0000FFFF
	mov [eax+4*3], dword 0x00CF9300

	;2
	mov [eax+4*4], dword 0x0000FFFF
	mov [eax+4*5], dword 0x00AF9B00

	;3
	mov [eax+4*6], dword 0x0000FFFF
	mov [eax+4*7], dword 0x00CF9B00

	;4
	mov [eax+4*8], dword 0x0000FFFF
	mov [eax+4*9], dword 0x000F9300

	;5
	mov [eax+4*10], dword 0x0000FFFF
	mov [eax+4*11], dword 0x000F9B00

	;6
	mov [eax+4*12], dword 0x0000FFFF
	mov [eax+4*13], dword 0x00DFF300

	;7
	mov [eax+4*14], dword 0x00000000
	mov [eax+4*15], dword 0x00D0F700

	;8
	mov [eax+4*16], dword 0x0000FFFF
	mov [eax+4*17], dword 0x00AFFF00

	;9
	mov [eax+4*18], dword 0x0000FFFF
	mov [eax+4*19], dword 0x00CFFF00

	;10-11 : Space reversed for a TSS descriptor
	mov [eax+4*20], dword 0
	mov [eax+4*21], dword 0
	mov [eax+4*22], dword 0
	mov [eax+4*23], dword 0

	;12 : Space reserved for a TSS stack segment descriptor
	mov [eax+4*24], dword 0
	mov [eax+4*25], dword 0

	;13 : Space reserved for a GDTR and a word of padding, not a descriptor
	mov [eax+4*26+0], word 13*8+7
	mov [eax+4*26+2], dword GDT_ADDR
	mov [eax+4*27+2], word 0

	lgdt [eax+4*26]
	ret

	; Writes a given descriptor in the GDT
	; Parameters:
	; RDI: Segment base (in the 32 highest bits)
	;      and segment limit (in the 20 lowest bits),
	;      all the other bits must be clear
	; RSI: Access flags, other bits must be clear
	; RDX: Other flags, other bits must be clear
	; RCX: Segment descriptor index in the GDT
BITS 64
writeDescriptor:
	; Write bytes 0-3 of the descriptor
	mov rax, rdi
	shr rax, 16
	mov ax, di
	mov [rcx*8+GDT_ADDR], eax

	; Write bytes 4-5 of the descriptor
	rol rdi, 16
	shl rsi, 8
	or sil, dil
	mov [rcx*8+GDT_ADDR+4], si

	; Write bytes 6-7 of the descriptor
	shl rdx, 20
	mov dx, di
	shr rdi, 32
	ror rdx, 16
	or dl, dil
	mov [rcx*8+GDT_ADDR+6], dx

	ret
