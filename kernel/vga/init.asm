global initVGA, initVGABIOS, print16VGA
extern memzeroasm
%include "kernel/mm/memmap.h"

BITS 16
; Expects a zero-terminated string in DI
; Prints the string using BIOS interrupts
; Does NOT preserve DI
print16VGA:
.printloop:
	mov al, [di]
	test al, al
	jz .done
	mov ah, 0x0e
	mov bh, 0x00
	mov bl, 0x07
	int 0x10
	inc di
	jmp .printloop
.done:
	ret

BITS 16
initVGABIOS:
	; Set current page to 0
	mov ah, 0x05
	xor al, al
	int 0x10

	; Set standard 80x25 mode
	mov al, 0x03
	mov ah, 0x00
	int 0x10

	; Set cursor pos to top left
	xor bh, bh
	xor dx, dx
	mov ah, 0x02
	int 0x10

	; Print some text and return (the print16 will return for us)
	mov di, .str
	jmp print16VGA
	.str: db "Early 80x25 console ready",10,13,0
	; This point is never reached

BITS 32
initVGA:
	push esi
	push edi
	mov esi, regs80x25
	call setRegs

	; Disable text cursor
	mov dx, 0x3D4
	mov ax, 0x0A
	out dx, ax
	mov dx, 0x3D5
	in al, dx
	or al, 0b10000
	out dx, al

	; Clear the framebuffer
	mov edi, VGATEXT_FRAMEBUFFER_PADDR
	mov ecx, PAGESIZE
	call memzeroasm

	pop edi
	pop esi
	ret

BITS 32
setRegs:
		 mov     dx, 0x3C2
		 lodsb
		 out     dx, al

		 mov     dx, 0x3DA
		 lodsb
		 out     dx, al

		 xor     ecx, ecx
		 mov     dx, 0x3C4
	.l1:
		 lodsb
		 xchg    al, ah
		 mov     al, cl
		 out     dx, ax
		 inc     ecx
		 cmp     cl, 4
		 jbe     .l1

		 mov     dx, 0x3D4
		 mov     ax, 0x0E11
		 out     dx, ax

		 xor     ecx, ecx
		 mov     dx, 0x3D4
	.l2:
		 lodsb
		 xchg    al, ah
		 mov     al, cl
		 out     dx, ax
		 inc     ecx
		 cmp     cl, 0x18
		 jbe     .l2

		 xor     ecx, ecx
		 mov     dx, 0x3CE
	.l3:
		 lodsb
		 xchg    al, ah
		 mov     al, cl
		 out     dx, ax
		 inc     ecx
		 cmp     cl, 8
		 jbe     .l3

		 mov     dx, 0x3DA
		 in      al, dx

		 xor     ecx, ecx
		 mov     dx, 0x3C0
	.l4:
		 in      ax, dx
		 mov     al, cl
		 out     dx, al
		 lodsb
		 out     dx, al
		 inc     ecx
		 cmp     cl, 0x14
		 jbe     .l4

		 mov     al, 0x20
		 out     dx, al
		 ret

regs80x25:
; MISC
db	0x67, 0x00
; SEQ
db	0x03, 0x00, 0x03, 0x00, 0x02,
; CRTC
db	0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
db	0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
db	0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
db	0xFF,
; GC
db	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
db	0xFF,
; AC
db	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
db	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
db	0x0C, 0x00, 0x0F, 0x08, 0x00
