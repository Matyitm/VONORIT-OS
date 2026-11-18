BITS 64

global isr_default
global isr_exception
global irq1_stub

isr_default:
	iretq

isr_exception:
	cli
	hlt
	jmp isr_exception

section .text

global isr128_stub
extern syscall_dispatch
extern syscall_exit_was_requested
extern gdt_restore_asm

isr128_stub:
 push r11
 push r10
 push r9
 push r8
 push rdi
 push rsi
 push rdx
 push rcx
 push rax
 mov rcx,[rsp+16]
 mov rdx,[rsp+24]
 mov rsi,[rsp+32]
 mov rdi,[rsp]
 call syscall_dispatch
 mov [rsp],rax
 call syscall_exit_was_requested
 test rax,rax
 jnz gdt_restore_asm
 pop rax
 pop rcx
 pop rdx
 pop rsi
 pop rdi
 pop r8
 pop r9
 pop r10
 pop r11
 iretq

global irq12_stub
%assign vector 0
%rep 32
global exception_stub_%[vector]
%assign vector vector+1
%endrep

extern timer_tick_handler
extern input_irq1_handler
extern input_irq12_handler
extern idt_exception_dispatch

%macro EXCEPTION_STUB 2
global exception_stub_%1
exception_stub_%1:
	push qword %2
	push qword %1
	jmp exception_common
%endmacro

exception_common:
	push rax
	push rcx
	push rdx
	push rbx
	push rsp
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

	mov rdi, rsp
	call idt_exception_dispatch
.exception_halt:
	cli
	hlt
	jmp .exception_halt

EXCEPTION_STUB 0,0
EXCEPTION_STUB 1,0
EXCEPTION_STUB 2,0
EXCEPTION_STUB 3,0
EXCEPTION_STUB 4,0
EXCEPTION_STUB 5,0
EXCEPTION_STUB 6,0
EXCEPTION_STUB 7,0
EXCEPTION_STUB 8,1
EXCEPTION_STUB 9,0
EXCEPTION_STUB 10,1
EXCEPTION_STUB 11,1
EXCEPTION_STUB 12,1
EXCEPTION_STUB 13,1
EXCEPTION_STUB 14,1
EXCEPTION_STUB 15,0
EXCEPTION_STUB 16,0
EXCEPTION_STUB 17,1
EXCEPTION_STUB 18,0
EXCEPTION_STUB 19,0
EXCEPTION_STUB 20,0
EXCEPTION_STUB 21,0
EXCEPTION_STUB 22,0
EXCEPTION_STUB 23,0
EXCEPTION_STUB 24,0
EXCEPTION_STUB 25,0
EXCEPTION_STUB 26,0
EXCEPTION_STUB 27,0
EXCEPTION_STUB 28,0
EXCEPTION_STUB 29,1
EXCEPTION_STUB 30,0
EXCEPTION_STUB 31,0

global irq0_stub
irq0_stub:
	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	call timer_tick_handler
	mov al,0x20
	out 0x20,al
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	iretq

irq1_stub:
	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	call input_irq1_handler
	mov al,0x20
	out 0x20,al
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	iretq

irq12_stub:
	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	call input_irq12_handler
	mov al,0x20
	out 0xA0,al
	out 0x20,al
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	iretq

section .note.GNU-stack noalloc noexec nowrite progbits
