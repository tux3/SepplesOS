extern isr_default_int, isr_reserved_int, isr_exc_DIV0, isr_exc_DEBUG, isr_exc_BP, isr_exc_NOMATH, isr_exc_MF, isr_exc_TSS, isr_exc_SWAP, isr_exc_AC, isr_exc_MC, isr_exc_XM, isr_exc_NMI, isr_exc_OVRFLW, isr_exc_BOUNDS, isr_exc_OPCODE, isr_exc_DOUBLEF, isr_exc_STACKF, isr_exc_GP, isr_exc_PF, isr_clock_int, isr_kbd_int, isr_spurious_int, isr_rtc_int, isr_hd_op_complete, doSyscalls
global _asm_default_int, _asm_reserved_int, _asm_exc_DIV0, _asm_exc_DEBUG, _asm_exc_BP, _asm_exc_NOMATH, _asm_exc_MF, _asm_exc_TSS, _asm_exc_SWAP, _asm_exc_AC, _asm_exc_MC, _asm_exc_XM, _asm_exc_NMI, _asm_exc_OVRFLW, _asm_exc_BOUNDS, _asm_exc_OPCODE, _asm_exc_DOUBLEF, _asm_exc_STACKF, _asm_exc_GP, _asm_exc_PF, _asm_irq_0, _asm_irq_1, _asm_irq_7, _asm_irq_8, _asm_irq_hd_op_complete, _asm_syscalls

%macro	SAVE_REGS 0
	push rax
	push rcx
	push rdx
	push rbx
	;push rsp
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro	RESTORE_REGS 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	;pop rsp
	pop rbx
	pop rdx
	pop rcx
	pop rax
%endmacro

_asm_default_int:
	SAVE_REGS
	call isr_default_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iretq

_asm_reserved_int:
	SAVE_REGS
	call isr_reserved_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iretq

_asm_exc_DIV0:
	SAVE_REGS
	call isr_exc_DIV0
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_DEBUG:
	push rdi
	mov rdi, [rsp+8]
	SAVE_REGS
	call isr_exc_DEBUG
	RESTORE_REGS
	pop rdi
	;add esp,8
	iretq

_asm_exc_BP:
	SAVE_REGS
	call isr_exc_BP
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_NOMATH:
	SAVE_REGS
	call isr_exc_NOMATH
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_MF:
	SAVE_REGS
	call isr_exc_MF
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_TSS:
	SAVE_REGS
	call isr_exc_TSS
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_SWAP:
	SAVE_REGS
	call isr_exc_SWAP
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_AC:
	SAVE_REGS
	call isr_exc_AC
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_MC:
	SAVE_REGS
	call isr_exc_MC
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_XM:
	SAVE_REGS
	call isr_exc_XM
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_NMI:
	SAVE_REGS
	call isr_exc_NMI
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_OVRFLW:
	SAVE_REGS
	call isr_exc_OVRFLW
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_BOUNDS:
	SAVE_REGS
	call isr_exc_BOUNDS
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_OPCODE:
	push rdi
	mov rdi, [rsp+8]
	SAVE_REGS
	call isr_exc_OPCODE
	RESTORE_REGS
	pop rdi
	add esp,8
	iretq

_asm_exc_DOUBLEF:
	SAVE_REGS
	call isr_exc_DOUBLEF
	RESTORE_REGS
	add esp,8
	iretq

_asm_exc_STACKF:
	push rsi
	push rdi
	mov rsi, [rsp+16]
	mov rdi, [rsp+16+8]
	SAVE_REGS
	call isr_exc_STACKF
	RESTORE_REGS
	pop rdi
	pop rsi
	add esp,8
	iretq

_asm_exc_GP:
	push rsi
	push rdi
	mov rsi, [rsp+16]
	mov rdi, [rsp+16+8]
	SAVE_REGS
	call isr_exc_GP
	RESTORE_REGS
	pop rdi
	pop rsi
	add esp,8
	iretq

_asm_exc_PF:
	push rsi
	push rdi
	mov rsi, [rsp+16]
	mov rdi, [rsp+16+8]
	SAVE_REGS
	call isr_exc_PF
	RESTORE_REGS
	pop rdi
	pop rsi
	add esp,8
	iretq

_asm_irq_0:
	SAVE_REGS
	mov rbp, rsp ; For the task switch
	call isr_clock_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iretq

_asm_irq_1:
	SAVE_REGS
	call isr_kbd_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iretq

_asm_irq_7:
	SAVE_REGS
	call isr_spurious_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iretq

_asm_irq_8:
	SAVE_REGS
	call isr_rtc_int
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iretq

_asm_irq_hd_op_complete:
	SAVE_REGS
	call isr_hd_op_complete
	mov al,0x20
	out 0x20,al
	RESTORE_REGS
	iretq

_asm_syscalls:
	push rax ; Room for the return value
	push rbp
	mov rbp, rsp ; Pointer to saved regs (-8)
	SAVE_REGS
	mov r9, rdi		; arg 5
	mov r8, rsi		; arg 4
	xchg rdx, rcx	; arg 2/3
	mov rsi, rbx	; arg 1
	mov rdi, rax    ; syscall number
	call doSyscalls
	mov [rbp+8], rax
	RESTORE_REGS
	pop rbp
	pop rax ; Return value
	iretq

