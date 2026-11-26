#include <stdint.h>
#include "syscall.h"
#include "timer.h"
#include "ui.h"
#include "gdt.h"
#include "sched.h"
static uint64_t syscall_exit_requested, syscall_exit_code;
static char ipc_msg_buf[64];
static uint32_t ipc_msg_sender;
static void copy_user_str(char* dst, uint64_t uptr, uint32_t max_len){
	uint32_t i=0u;
	if(uptr>=0x00400000u && uptr<0x00800000u){
		const char* s=(const char*)(uintptr_t)uptr;
		while(i+1u<max_len && (uptr+i)<0x00800000u && s[i]!=0){ dst[i]=s[i]; ++i; }
	}
	dst[i]=0;
}
uint32_t syscall_ipc_pop(char* out,uint32_t max){
	if(!ipc_msg_sender) return 0u;
	uint32_t s=ipc_msg_sender, i=0u;
	while(ipc_msg_buf[i] && i+1u<max){ out[i]=ipc_msg_buf[i]; ++i; }
	out[i]=0; ipc_msg_sender=0u; return s;
}
uint64_t syscall_dispatch(uint64_t number,uint64_t arg1,uint64_t arg2,uint64_t arg3){
	(void)arg3;
	if(number==SYS_WRITE){
		if(arg1<0x00400000u || arg1>=0x00800000u || arg2>0x00400000u || arg1+arg2>0x00800000u) return (uint64_t)-1;
		const char* text=(const char*)(uintptr_t)arg1;
		for(uint64_t i=0u;i<arg2;++i) ui_terminal_append(text[i]);
		return arg2;
	}
	if(number==SYS_CREATE_WINDOW){
		if(arg1<0x00400000u || arg1>=0x00800000u || arg2<0x00400000u || arg2>=0x00800000u) return (uint64_t)-1;
		char t[32], b[256]; copy_user_str(t,arg1,32u); copy_user_str(b,arg2,256u);
		ui_open_user_window(t,b); return 0u;
	}
	if(number==SYS_SEND_MSG){
		if(arg2<0x00400000u || arg2>=0x00800000u) return (uint64_t)-1;
		copy_user_str(ipc_msg_buf,arg2,64u); ipc_msg_sender=(uint32_t)(arg1?arg1:1u); return 0u;
	}
	if(number==SYS_RECV_MSG){
		if(arg1<0x00400000u || arg1>=0x00800000u) return 0u;
		return (uint64_t)syscall_ipc_pop((char*)(uintptr_t)arg1,64u);
	}
	if(number==SYS_GET_TICKS) return timer_get_ticks();
	if(number==SYS_GET_UPTIME) return timer_get_seconds();
	if(number==SYS_EXIT){
		syscall_exit_code=arg1; syscall_exit_requested=1u;
		ui_terminal_append('\n');ui_terminal_append('[');ui_terminal_append('E');ui_terminal_append('X');ui_terminal_append('I');ui_terminal_append('T');ui_terminal_append(' ');ui_terminal_append('C');ui_terminal_append('O');ui_terminal_append('D');ui_terminal_append('E');ui_terminal_append(' ');
		if(arg1==0u) ui_terminal_append('0');
		else{ char digits[32]; uint64_t pos=0u, val=arg1; while(val!=0u && pos<32u){digits[pos++]=(char)('0'+val%10u);val/=10u;} while(pos>0u)ui_terminal_append(digits[--pos]); }
		ui_terminal_append(']');
		if(sched_current_pid()!=0u){ sched_exit_current(); __asm__ volatile("sti"); for(;;) __asm__ volatile("hlt"); }
		gdt_restore_kernel_context(); return arg1;
	}
	return (uint64_t)-1;
}
void syscall_initialize(void){syscall_exit_requested=0u;syscall_exit_code=0u;}
uint64_t syscall_exit_was_requested(void){return syscall_exit_requested;}
uint64_t syscall_exit_code_value(void){return syscall_exit_code;}
