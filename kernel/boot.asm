global _boot
extern boot, fatalError

%define MULTIBOOT_HEADER_MAGIC			0x1BADB002
%define MULTIBOOT_HEADER_FLAGS			0x7
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

; Multiboot header
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

endOfCodeError:
db "End of code",10,0

; Entry point
_boot:
    ; We don't want interrupts
    cli

    ; Set up the initial stack frame
	mov esp, 0x9C7FF ; KERN_STACK
	mov ebp, esp

	; Call the kernel's boot
	push ebx ; Pass the adress of multiboot_info as arg 2
	push eax ; Pass the magic of multiboot_info as arg 1
	call boot

	; We shouldn't reach this point
	push endOfCodeError
	call fatalError
	cli
	hlt
