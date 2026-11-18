BITS 32
default abs

global pdpt_table
global high_pd_tables

section .multiboot
align 4
multiboot_header:
	dd 0x1BADB002
	dd 0x00000007
	dd -(0x1BADB002 + 0x00000007)
	dd 0
	dd 1024
	dd 768
	dd 32

section .bss
align 4096
pml4_table:
	resb 4096
pdpt_table:
	resb 4096
pd_tables:
	resb 16384
high_pd_tables:
	resb 16384

multiboot_magic_saved:
	resd 1
multiboot_information_saved:
	resd 1

alignb 16
stack_bottom:
	resb 16384
stack_top:

section .text
extern kernel_main64
global _start64
_start64:
	cli
	mov dword [abs multiboot_magic_saved], eax
	mov dword [abs multiboot_information_saved], ebx
	mov esp, stack_top

	xor eax, eax
	mov edi, pml4_table
	mov ecx, 10240
	rep stosd

	mov edi, pml4_table
	mov eax, pdpt_table
	or eax, 0x03
	mov [edi], eax

	mov edi, pdpt_table
	mov eax, pd_tables
	mov ecx, 4
.link_pd_tables:
	mov ebx, eax
	or ebx, 0x03
	mov [edi], ebx
	add eax, 4096
	add edi, 8
	loop .link_pd_tables

	mov edi, pd_tables
	xor eax, eax
	xor edx, edx
	mov ecx, 2048
.map_2mb:
	mov ebx, eax
	or ebx, 0x83
	mov [edi], ebx
	mov [edi + 4], edx
	add eax, 0x00200000
	adc edx, 0
	add edi, 8
	loop .map_2mb

	mov eax, cr4
	or eax, (1 << 5) | (1 << 9) | (1 << 10)
	mov cr4, eax

	mov eax, pml4_table
	mov cr3, eax

	lgdt [gdt_descriptor]

	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	mov eax, cr0
	and eax, ~(1 << 2)
	or eax, (1 << 1) | 0x80000001
	mov cr0, eax

	jmp 0x10:long_mode_entry

BITS 64
long_mode_entry:
	mov edi, dword [abs multiboot_magic_saved]
	mov esi, dword [abs multiboot_information_saved]

	mov ax, 0x18
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov rsp, stack_top

	call kernel_main64

.hang:
	cli
	hlt
	jmp .hang

section .rodata
align 8
gdt:
	dq 0x0000000000000000
	dq 0x0000000000000000
	dq 0x00AF9A000000FFFF
	dq 0x00AF92000000FFFF
gdt_descriptor:
	dw gdt_descriptor - gdt - 1
	dq gdt

section .note.GNU-stack noalloc noexec nowrite progbits
