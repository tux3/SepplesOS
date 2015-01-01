global memzeroasm

; NOTE: This file may be called from boot.asm, it must stay both valid x86 and x86_64 code

; Expects an address in RDI and a size in bytes in RCX
memzeroasm:
	mov edx, edi
	cld
	xor eax, eax
	rep stosd
	mov edi, edx
	ret
