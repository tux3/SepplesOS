
global doTaskSwitch

; Arguments:
; RDI - Process* to switch to
; RSI - KSS to use
; RDX - KRSP to use
doTaskSwitch:
	; Prepare the stack for iret
	mov ss, rsi
	mov rsp, rdx
	movzx rax, word [rdi+146]
	push rax ; SS
	push qword [rdi+32] ; RSP
	xor rax,rax
	mov eax, [rdi+136]
	or rax, 0x200
	and eax, 0xFFFFBFFF
	push rax ; EFLAGS
	movzx rax, word [rdi+144]
	push rax ; CS
	push qword [rdi+128] ; RIP

	; unmask the PIC
	mov al, 0x20
	out 0x20, al

	; load CR3
	mov rax, [rdi+160]
	mov cr3, rax

	; Load the registers from current
	mov r11, rdi
	mov ds, word [r11+148]
	mov es, word [r11+150]
	mov fs, word [r11+152]
	mov gs, word [r11+154]
	mov rax, [r11+0]
	mov rcx, [r11+8]
	mov rdx, [r11+16]
	mov rbx, [r11+24]
	mov rbp, [r11+40]
	mov rsi, [r11+48]
	mov rdi, [r11+56]
	mov r8, [r11+64]
	mov r9, [r11+72]
	mov r10, [r11+80]
	mov r11, [r11+88]

	; Boom
	xchg bx,bx
	iretq

