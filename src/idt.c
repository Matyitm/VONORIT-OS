#include <stdint.h>
#include <stddef.h>

#include "idt.h"
#include "ui.h"

#define PIC1_COMMAND 0x20u
#define PIC1_DATA 0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA 0xA1u

struct idt_entry
{
	uint16_t offset_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t type_attributes;
	uint16_t offset_middle;
	uint32_t offset_high;
	uint32_t reserved;
} __attribute__((packed));

typedef struct
{
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) idtr_t;

static struct idt_entry idt[256];
static idtr_t idtr;
static framebuffer_t* bsod_framebuffer;

extern void exception_stub_0(void); extern void exception_stub_1(void);
extern void exception_stub_2(void); extern void exception_stub_3(void);
extern void exception_stub_4(void); extern void exception_stub_5(void);
extern void exception_stub_6(void); extern void exception_stub_7(void);
extern void exception_stub_8(void); extern void exception_stub_9(void);
extern void exception_stub_10(void); extern void exception_stub_11(void);
extern void exception_stub_12(void); extern void exception_stub_13(void);
extern void exception_stub_14(void); extern void exception_stub_15(void);
extern void exception_stub_16(void); extern void exception_stub_17(void);
extern void exception_stub_18(void); extern void exception_stub_19(void);
extern void exception_stub_20(void); extern void exception_stub_21(void);
extern void exception_stub_22(void); extern void exception_stub_23(void);
extern void exception_stub_24(void); extern void exception_stub_25(void);
extern void exception_stub_26(void); extern void exception_stub_27(void);
extern void exception_stub_28(void); extern void exception_stub_29(void);
extern void exception_stub_30(void); extern void exception_stub_31(void);
extern void irq0_stub(void); extern void irq1_stub(void); extern void irq12_stub(void);

static void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));
}

static void io_wait(void){outb(0x80u,0u);}

static void set_gate(uint8_t vector, void (*handler)(void))
{
	uint64_t address=(uint64_t)(uintptr_t)handler;
	struct idt_entry* entry=&idt[vector];
	entry->offset_low=(uint16_t)address;
	entry->selector=0x10u;
	entry->ist=0u;
	entry->type_attributes=0x8Eu;
	entry->offset_middle=(uint16_t)(address>>16);
	entry->offset_high=(uint32_t)(address>>32);
	entry->reserved=0u;
}

static const char* exception_name(uint64_t vector)
{
	static const char* names[32]={"Divide error","Debug","NMI","Breakpoint","Overflow","BOUND range","Invalid opcode","Device unavailable","Double fault","COPROCESSOR segment","Invalid TSS","Segment not present","Stack fault","General protection fault","Page fault","Reserved","x87 fault","Alignment check","Machine check","SIMD exception","Virtualization","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Security exception","Reserved"};
	return vector<32u?names[vector]:"Unknown exception";
}

static void hex32(uint64_t value)
{
	static const char digits[]="0123456789ABCDEF";
	char text[9];
	text[8]=0;
	for(int i=7;i>=0;--i){text[i]=digits[value&15u];value>>=4;}
	ui_draw_text(bsod_framebuffer,350u,150u,text,2u,255u,255u,255u);
}

void idt_exception_dispatch(uint64_t* frame)
{
	uint64_t vector=frame[17];
	uint64_t error=frame[16];
	uint64_t rip=frame[18];
	uint64_t rsp=frame[21];
	uint64_t cr2;
	__asm__ volatile("mov %%cr2,%0":"=r"(cr2));
	if(bsod_framebuffer==NULL) for(;;){__asm__ volatile("cli; hlt");}
	ui_fill_rect(bsod_framebuffer,0u,0u,bsod_framebuffer->width,bsod_framebuffer->height,0u,40u,180u);
	ui_draw_text(bsod_framebuffer,40u,32u,"VONORIT OS :(",4u,255u,255u,255u);
	ui_draw_text(bsod_framebuffer,40u,88u,exception_name(vector),3u,255u,255u,255u);
	ui_draw_text(bsod_framebuffer,40u,142u,"RIP",2u,255u,255u,255u);
	hex32(rip);
	ui_draw_text(bsod_framebuffer,40u,190u,"CR2",2u,255u,255u,255u);hex32(cr2);
	ui_draw_text(bsod_framebuffer,40u,238u,"RSP",2u,255u,255u,255u);hex32(rsp);
	ui_draw_text(bsod_framebuffer,40u,286u,"ERR",2u,255u,255u,255u);hex32(error);
	for(;;){__asm__ volatile("cli; hlt");}
}

void idt_initialize(framebuffer_t* framebuffer)
{
	bsod_framebuffer=framebuffer;
	void (*exceptions[32])(void)= {exception_stub_0,exception_stub_1,exception_stub_2,exception_stub_3,exception_stub_4,exception_stub_5,exception_stub_6,exception_stub_7,exception_stub_8,exception_stub_9,exception_stub_10,exception_stub_11,exception_stub_12,exception_stub_13,exception_stub_14,exception_stub_15,exception_stub_16,exception_stub_17,exception_stub_18,exception_stub_19,exception_stub_20,exception_stub_21,exception_stub_22,exception_stub_23,exception_stub_24,exception_stub_25,exception_stub_26,exception_stub_27,exception_stub_28,exception_stub_29,exception_stub_30,exception_stub_31};
	for(uint16_t i=0;i<256u;++i) set_gate((uint8_t)i, exceptions[i<32u?i:0u]);
	for(uint8_t i=0;i<32u;++i) set_gate(i,exceptions[i]);
	set_gate(32u,irq0_stub); set_gate(33u,irq1_stub); set_gate(44u,irq12_stub);
	idtr.limit=(uint16_t)(sizeof(idt)-1u); idtr.base=(uint64_t)(uintptr_t)idt;
	__asm__ volatile("cli; lidt %0"::"m"(idtr));

	outb(PIC1_COMMAND,0x11u); io_wait(); outb(PIC2_COMMAND,0x11u); io_wait();
	outb(PIC1_DATA,32u); io_wait(); outb(PIC2_DATA,40u); io_wait();
	outb(PIC1_DATA,4u); io_wait(); outb(PIC2_DATA,2u); io_wait();
	outb(PIC1_DATA,1u); io_wait(); outb(PIC2_DATA,1u); io_wait();
	outb(PIC1_DATA,0xF8u); outb(PIC2_DATA,0xEFu);
}
