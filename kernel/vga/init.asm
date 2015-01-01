global initVGA
extern memzeroasm
%include "kernel/mm/memmap.h"

; NOTE: This file may be called from boot.asm, it must stay both valid x86 and x86_64 code

;; TODO: This code should switch to text mode manually
;; It fails on real hardware because grub leaves us in linear graphics mode
;; If we put a EBFE after initVGA, it freezes, if we let it boot, it triple faults
;; And can't log anything since we're not in text mode
;; => Switch to text mode, ask grub to set us in linear graphics mode so we can text with bochs
;; => On real hardware grub sometime doesn't comply, but on bochs it does

initVGA:
	push rdi

	; Set the VGA Input/Output Address Select to 1
	mov dx, 0x3CC
	in al, dx
	and al, 1
	mov dx, 0x3C2
	out dx, al

	; Set the VGA framebuffer at 0xB8000
	mov dx, 0x3CE
	mov al, 0x6
	out dx, al
	mov dx, 0x3CF
	in al, dx
	and al, 0xF3
	or al, 0b11<<2
	out dx, al

	; Set the VGA text mode character height to 16px
	mov dx, 0x3D4
	mov al, 0x9
	out dx, al
	mov dx, 0x3D5
	in al, dx
	and al, 0xE0
	or al, 0xF
	out dx, al

	; Clear the VGA text framebuffer
	mov edi, VGATEXT_FRAMEBUFFER_PADDR
	mov ecx, PAGESIZE
	call memzeroasm

	pop rdi
	ret
