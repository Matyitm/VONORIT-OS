#ifndef VONORIT_SYSCALL_H
#define VONORIT_SYSCALL_H

#include <stdint.h>

#define SYS_WRITE 1u
#define SYS_GET_TICKS 2u
#define SYS_GET_UPTIME 3u
#define SYS_CREATE_WINDOW 10u
#define SYS_EXIT 60u

void syscall_initialize(void);
uint64_t syscall_exit_was_requested(void);
uint64_t syscall_exit_code_value(void);
uint64_t syscall_dispatch(uint64_t number,uint64_t arg1,uint64_t arg2,uint64_t arg3);

#endif
