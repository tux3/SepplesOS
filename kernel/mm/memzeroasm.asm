global memzeroasm

; NOTE: This file may be called from boot.asm, it must stay both valid x86 and x86_64 code

; Expects an address in RDI and a size in bytes in RCX
BITS 64 ; Also BITS 32
memzeroasm:
	mov edx, edi
	cld
	xor eax, eax
	rep stosb
	mov edi, edx
	ret
