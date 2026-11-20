#ifndef VONORIT_GDT_H
#define VONORIT_GDT_H
#include <stdint.h>
void gdt_init(void);
void enter_usermode(uint64_t entry_rip,uint64_t user_rsp);
void gdt_restore_kernel_context(void);
void gdt_map_user_range(void);
void gdt_set_rsp0(uint64_t rsp0);
uint64_t gdt_default_rsp0(void);
#endif
