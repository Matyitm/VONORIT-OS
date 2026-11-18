BITS 32

section .multiboot
align 4
multiboot_header:
	dd 0x1BADB002
	dd 0x00000003
	dd -(0x1BADB002 + 0x00000003)

section .bss
align 16
stack_bottom:
	resb 16384
stack_top:

section .text
extern kernel_main
global _start
_start:
	cli
	mov esp, stack_top
	push ebx
	push eax
	call kernel_main

.hang:
	cli
	hlt
	jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
