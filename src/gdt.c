#include <stdint.h>

#include "gdt.h"

struct gdt_ptr
{
        uint16_t limit;
        uint64_t base;
} __attribute__((packed));

struct tss_entry
{
        uint32_t reserved0;
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;
        uint64_t reserved1;
        uint64_t ist[7];
        uint64_t reserved2;
        uint16_t reserved3;
        uint16_t io_map_base;
} __attribute__((packed));

static uint64_t gdt[8];
static struct gdt_ptr gdt_pointer;
static struct tss_entry tss_block;
static uint8_t rsp0_stack[16384] __attribute__((aligned(16)));
uint64_t gdt_kernel_rsp;

extern void gdt_load(const struct gdt_ptr* pointer,uint16_t data_selector,uint16_t code_selector,uint16_t tss_selector);
extern void enter_usermode_asm(uint64_t entry_rip,uint64_t user_rsp);

void gdt_map_user_range(void)
{
        extern uint64_t pml4_table[];
        extern uint64_t pdpt_table[];
        extern uint64_t pdpt_table[];
	extern uint64_t pd_tables[];
	pml4_table[0]|=0x07u;
	pdpt_table[0]|=0x07u;
        pml4_table[0]|=0x07u;
        pdpt_table[0]|=0x07u;
        for(uint64_t index=2u;index<4u;++index) pd_tables[index]|=0x87u;
        __asm__ volatile("mov %0,%%cr3"::"r"((uint64_t)(uintptr_t)pml4_table):"memory");
}

void gdt_init(void)
{
        gdt[0]=0u;
        gdt[1]=0u;
	gdt[2]=0x00AF9A000000FFFFu;
        gdt[3]=0x00AF92000000FFFFu;
        gdt[4]=0x00AFF2000000FFFFu;
        gdt[5]=0x00AFFA000000FFFFu;

        tss_block.reserved0=0u;
        tss_block.rsp0=(uint64_t)(uintptr_t)&rsp0_stack[16384u-16u];
        tss_block.rsp1=0u;tss_block.rsp2=0u;tss_block.reserved1=0u;tss_block.reserved2=0u;tss_block.reserved3=0u;
        for(uint64_t i=0u;i<7u;++i)tss_block.ist[i]=0u;
        tss_block.io_map_base=(uint16_t)sizeof(tss_block);

        uint64_t tss=(uint64_t)(uintptr_t)&tss_block;
        uint64_t limit=(uint64_t)(sizeof(tss_block)-1u);
        gdt[6]=(limit&0xFFFFu)|((tss&0xFFFFFFu)<<16)|((uint64_t)0x89u<<40)|(((limit>>16)&0xFu)<<48)|(((tss>>24)&0xFFu)<<56);
        gdt[7]=(tss>>32);

        gdt_pointer.limit=(uint16_t)(sizeof(gdt)-1u);
        gdt_pointer.base=(uint64_t)(uintptr_t)gdt;
        gdt_load(&gdt_pointer,0x18u,0x10u,0x30u);
        gdt_map_user_range();
}

void enter_usermode(uint64_t entry_rip,uint64_t user_rsp)
{
        enter_usermode_asm(entry_rip,user_rsp);
}

void gdt_restore_kernel_context(void)
{
        extern void gdt_restore_asm(void);
        gdt_restore_asm();
}
