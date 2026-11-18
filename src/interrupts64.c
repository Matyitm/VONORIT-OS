#include <stdint.h>

#include "interrupts64.h"

#define PIC1_COMMAND 0x20u
#define PIC1_DATA 0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA 0xA1u
#define PIC_ICW1_INIT 0x10u
#define PIC_ICW1_ICW4 0x01u
#define PIC_ICW4_8086 0x01u

#define IDT_PRESENT 0x80u
#define IDT_INTERRUPT_GATE 0x0Eu

extern void isr_default(void);
extern void isr_exception(void);

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

typedef struct idt_entry idt_entry_t;

typedef struct
{
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) idtr_t;

static idt_entry_t idt[256];
static idtr_t idtr;

static inline void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
	uint8_t value;
	__asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static void io_wait(void)
{
	outb(0x80u, 0u);
}

static void set_idt_gate(uint8_t vector, void (*handler)(void))
{
	uint64_t address = (uint64_t)(uintptr_t)handler;
	idt_entry_t* entry = &idt[vector];

	entry->offset_low = (uint16_t)address;
	entry->selector = 0x10u;
	entry->ist = 0u;
	entry->type_attributes = IDT_PRESENT | IDT_INTERRUPT_GATE;
	entry->offset_middle = (uint16_t)(address >> 16);
	entry->offset_high = (uint32_t)(address >> 32);
	entry->reserved = 0u;
}

static void load_idt(void)
{
	idtr.limit = (uint16_t)(sizeof(idt) - 1u);
	idtr.base = (uint64_t)(uintptr_t)idt;
	__asm__ volatile ("lidt %0" : : "m"(idtr));
}

static void pic_remap(void)
{
	uint8_t master_mask = inb(PIC1_DATA);
	uint8_t slave_mask = inb(PIC2_DATA);

	outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, 32u);
	io_wait();
	outb(PIC2_DATA, 40u);
	io_wait();
	outb(PIC1_DATA, 4u);
	io_wait();
	outb(PIC2_DATA, 2u);
	io_wait();
	outb(PIC1_DATA, PIC_ICW4_8086);
	io_wait();
	outb(PIC2_DATA, PIC_ICW4_8086);
	io_wait();

	(void)master_mask;
	(void)slave_mask;
	/* Keep IRQ1 enabled and unmask IRQ2 so IRQ12 from the slave reaches CPU. */
	outb(PIC1_DATA, 0xF9u);
	outb(PIC2_DATA, 0xEFu);
}

void interrupts64_initialize(void)
{
	__asm__ volatile ("cli");

	for (uint16_t vector = 0u; vector < 256u; ++vector)
	{
		set_idt_gate((uint8_t)vector, isr_default);
	}
	for (uint8_t vector = 0u; vector < 32u; ++vector)
	{
		set_idt_gate(vector, isr_exception);
	}

	load_idt();
	pic_remap();
}
