extern isr_default_int, isr_reserved_int, isr_exc_DIV0, isr_exc_DEBUG, isr_exc_BP, isr_exc_NOMATH, isr_exc_MF, isr_exc_TSS, isr_exc_SWAP, isr_exc_AC, isr_exc_MC, isr_exc_XM, isr_exc_NMI, isr_exc_OVRFLW, isr_exc_BOUNDS, isr_exc_OPCODE, isr_exc_DOUBLEF, isr_exc_STACKF, isr_exc_GP, isr_exc_PF, isr_clock_int, isr_kbd_int, isr_spurious_int, isr_rtc_int, isr_14_int, do_syscalls
global _asm_default_int, _asm_reserved_int, _asm_exc_DIV0, _asm_exc_DEBUG, _asm_exc_BP, _asm_exc_NOMATH, _asm_exc_MF, _asm_exc_TSS, _asm_exc_SWAP, _asm_exc_AC, _asm_exc_MC, _asm_exc_XM, _asm_exc_NMI, _asm_exc_OVRFLW, _asm_exc_BOUNDS, _asm_exc_OPCODE, _asm_exc_DOUBLEF, _asm_exc_STACKF, _asm_exc_GP, _asm_exc_PF, _asm_irq_0, _asm_irq_1, _asm_irq_7, _asm_irq_8, _asm_irq_14, _asm_syscalls

%macro	SAVE_REGS 0
	pushad
	push ds
	push es
	push fs
	push gs
	push ebx
	mov bx,0x10
	mov ds,bx
	pop ebx
%endmacro

%macro	RESTORE_REGS 0
	pop gs
	pop fs
	pop es
	pop ds
	popad
%endmacro

_asm_default_int:
	SAVE_REGS
	call isr_default_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iret

_asm_reserved_int:
	SAVE_REGS
	call isr_reserved_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iret

_asm_exc_DIV0:
	SAVE_REGS
	call isr_exc_DIV0
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_DEBUG:
	SAVE_REGS
	call isr_exc_DEBUG
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_BP:
	SAVE_REGS
	call isr_exc_BP
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_NOMATH:
	SAVE_REGS
	call isr_exc_NOMATH
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_MF:
	SAVE_REGS
	call isr_exc_MF
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_TSS:
	SAVE_REGS
	call isr_exc_TSS
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_SWAP:
	SAVE_REGS
	call isr_exc_SWAP
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_AC:
	SAVE_REGS
	call isr_exc_AC
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_MC:
	SAVE_REGS
	call isr_exc_MC
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_XM:
	SAVE_REGS
	call isr_exc_XM
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_NMI:
	SAVE_REGS
	call isr_exc_NMI
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_OVRFLW:
	SAVE_REGS
	call isr_exc_OVRFLW
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_BOUNDS:
	SAVE_REGS
	call isr_exc_BOUNDS
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_OPCODE:
	SAVE_REGS
	call isr_exc_OPCODE
	RESTORE_REGS
	add esp,4
	iret


_asm_exc_DOUBLEF:
	SAVE_REGS
	call isr_exc_DOUBLEF
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_STACKF:
	SAVE_REGS
	call isr_exc_STACKF
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_GP:
	SAVE_REGS
	call isr_exc_GP
	RESTORE_REGS
	add esp,4
	iret

_asm_exc_PF:
	SAVE_REGS
	call isr_exc_PF
	RESTORE_REGS
	add esp,4
	iret

_asm_irq_0:
	SAVE_REGS
	call isr_clock_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iret

_asm_irq_1:
	SAVE_REGS
	call isr_kbd_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iret

_asm_irq_7:
	SAVE_REGS
	call isr_spurious_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iret

_asm_irq_8:
	SAVE_REGS
	call isr_rtc_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iret

_asm_irq_14:
	SAVE_REGS
	call isr_14_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iret

_asm_syscalls:
	SAVE_REGS
	push eax                 ; syscall number
	call do_syscalls
	pop eax
	RESTORE_REGS
	iret

