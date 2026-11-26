#ifndef VONORIT_SCHED_H
#define VONORIT_SCHED_H
#include <stdint.h>
#define SCHED_MAX_PROCS 8u
typedef struct {
	uint8_t used; uint8_t state; uint16_t pid;
	uint64_t ticks; uint64_t rsp; uint64_t cr3; char name[24];
} sched_proc_t;
void sched_init(void);
int sched_spawn_elf(const char* name, const uint8_t* data, uint32_t size);
int sched_kill(uint16_t pid);
void sched_exit_current(void);
uint16_t sched_current_pid(void);
uint64_t sched_on_tick(uint64_t current_rsp);
const sched_proc_t* sched_get_proc(uint8_t index);
#endif
