#include <stdint.h>

#include "syscall.h"
#include "timer.h"
#include "ui.h"
#include "gdt.h"

static uint64_t syscall_exit_requested;
static uint64_t syscall_exit_code;

uint64_t syscall_dispatch(uint64_t number,uint64_t arg1,uint64_t arg2,uint64_t arg3)
{
	(void)arg3;
	if(number==SYS_WRITE)
	{
		if(arg1<0x00400000u || arg1>=0x00800000u) return (uint64_t)-1;
		if(arg2>0x00400000u || arg1+arg2>0x00800000u) return (uint64_t)-1;
		const char* text=(const char*)(uintptr_t)arg1;
		for(uint64_t i=0u;i<arg2;++i) ui_terminal_append(text[i]);
		return arg2;
	}
	if(number==SYS_GET_TICKS) return timer_get_ticks();
	if(number==SYS_GET_UPTIME) return timer_get_seconds();
	if(number==SYS_EXIT)
	{
		syscall_exit_code=arg1;
		syscall_exit_requested=1u;
		ui_terminal_append('\n');ui_terminal_append('[');ui_terminal_append('E');ui_terminal_append('X');ui_terminal_append('I');ui_terminal_append('T');ui_terminal_append(' ');ui_terminal_append('C');ui_terminal_append('O');ui_terminal_append('D');ui_terminal_append('E');ui_terminal_append(' ');
		if(arg1==0u) ui_terminal_append('0');
		else
		{
			char digits[32];uint64_t pos=0u;uint64_t value=arg1;
			while(value!=0u && pos<32u){digits[pos++]=(char)('0'+value%10u);value/=10u;}
			while(pos>0u)ui_terminal_append(digits[--pos]);
		}
		ui_terminal_append(']');
		gdt_restore_kernel_context();
		return arg1;
	}
	return (uint64_t)-1;
}

void syscall_initialize(void){syscall_exit_requested=0u;syscall_exit_code=0u;}
uint64_t syscall_exit_was_requested(void){return syscall_exit_requested;}
uint64_t syscall_exit_code_value(void){return syscall_exit_code;}
