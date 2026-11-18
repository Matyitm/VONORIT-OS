BITS 64
extern gdt_kernel_rsp
global gdt_load
global enter_usermode_asm
global gdt_restore_asm
section .text
gdt_load:
 lgdt [rdi]
 mov ax,si
 mov ds,ax
 mov es,ax
 mov fs,ax
 mov gs,ax
 mov ss,ax
 mov ax,cx
 ltr ax
 push rdx
 lea rax,[rel .rc]
 push rax
 retfq
.rc:
 ret
enter_usermode_asm:
 cli
 push rbx
 push rbp
 push r12
 push r13
 push r14
 push r15
 mov [gdt_kernel_rsp],rsp
 push 0x23
 push rsi
 pushfq
 or qword [rsp],0x200
 push 0x2B
 push rdi
 iretq
gdt_restore_asm:
 mov rsp,[gdt_kernel_rsp]
 pop r15
 pop r14
 pop r13
 pop r12
 pop rbp
 pop rbx
 sti
 ret
section .note.GNU-stack noalloc noexec nowrite progbits
