#include <stddef.h>
#include <stdint.h>

#include "interrupts64.h"
#include "framebuffer.h"
#include "ui.h"
#include "pci.h"
#include "net.h"
#include "idt.h"
#include "input.h"
#include "timer.h"
#include "ata.h"
#include "vfs.h"
#include "vfs.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define MULTIBOOT_INFO_FLAG_FRAMEBUFFER  (1u << 12)

typedef struct __attribute__((packed))
{
	uint32_t flags;
	uint8_t reserved[84];
	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
	uint8_t framebuffer_type;
	uint8_t red_position;
	uint8_t red_mask;
	uint8_t green_position;
	uint8_t green_mask;
	uint8_t blue_position;
	uint8_t blue_mask;
} multiboot_info_t;

extern volatile uint64_t pdpt_table[];
extern volatile uint64_t high_pd_tables[];

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

static inline void serial_out(uint16_t port, uint8_t value)
{
	__asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t serial_in(uint16_t port)
{
	uint8_t value;
	__asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

static void serial_initialize(void)
{
	serial_out(0x3F9, 0x00);
	serial_out(0x3FB, 0x80);
	serial_out(0x3F8, 0x03);
	serial_out(0x3F9, 0x00);
	serial_out(0x3FB, 0x03);
	serial_out(0x3FA, 0xC7);
	serial_out(0x3FC, 0x0B);
}

static void serial_putchar(char character)
{
	for (uint32_t attempts = 0; attempts < 100000; attempts++)
	{
		if ((serial_in(0x3FD) & 0x20) != 0)
		{
			serial_out(0x3F8, (uint8_t)character);
			return;
		}
	}
}

static void serial_write_hex32(uint32_t value)
{
	static const char digits[] = "0123456789ABCDEF";
	for (int shift = 28; shift >= 0; shift -= 4)
	{
		serial_putchar(digits[(value >> shift) & 0x0F]);
	}
}

static void serial_write_hex64(uint64_t value)
{
	serial_write_hex32((uint32_t)(value >> 32));
	serial_write_hex32((uint32_t)value);
}

static void serial_flush(void)
{
	for (uint32_t attempts = 0; attempts < 100000; attempts++)
	{
		if ((serial_in(0x3FD) & 0x40) != 0)
		{
			return;
		}
	}
}

static void serial_write(const char* text)
{
	for (size_t index = 0; text[index] != '\0'; index++)
	{
		serial_putchar(text[index]);
	}
	serial_flush();
}

static uint8_t vga_entry_color(uint8_t foreground, uint8_t background)
{
	return (uint8_t)(foreground | (background << 4));
}

static uint16_t vga_entry(unsigned char character, uint8_t color)
{
	return (uint16_t)character | ((uint16_t)color << 8);
}

static void terminal_write(const char* text)
{
	for (size_t index = 0; text[index] != '\0'; index++)
	{
		char character = text[index];

		if (character == '\n')
		{
			terminal_column = 0;
			terminal_row++;
			continue;
		}

		VGA_MEMORY[terminal_row * VGA_WIDTH + terminal_column] =
			vga_entry((unsigned char)character, terminal_color);

		if (++terminal_column == VGA_WIDTH)
		{
			terminal_column = 0;
			terminal_row++;
		}
	}
}

static void terminal_fallback(const char* reason)
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(15, 1);

	for (size_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < VGA_WIDTH; x++)
		{
			VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
		}
	}

	terminal_write("VONORIT-OS x86_64\n");
	terminal_write("GUI framebuffer unavailable.\n\n");
	terminal_write(reason);
}

static int find_framebuffer(uint32_t information, framebuffer_t* result)
{
	if (information == 0)
	{
		return 0;
	}

	multiboot_info_t* multiboot = (multiboot_info_t*)(uintptr_t)information;
	if ((multiboot->flags & MULTIBOOT_INFO_FLAG_FRAMEBUFFER) == 0 ||
		multiboot->framebuffer_type != 1 ||
		(multiboot->framebuffer_bpp != 24 && multiboot->framebuffer_bpp != 32) ||
		multiboot->framebuffer_width == 0 ||
		multiboot->framebuffer_height == 0)
	{
		return 0;
	}

	result->address = (volatile uint8_t*)(uintptr_t)multiboot->framebuffer_addr;
	result->physical_address = multiboot->framebuffer_addr;
	result->pitch = multiboot->framebuffer_pitch;
	result->width = multiboot->framebuffer_width;
	result->height = multiboot->framebuffer_height;
	result->bits_per_pixel = multiboot->framebuffer_bpp;
	result->red_position = multiboot->red_position;
	result->red_mask = multiboot->red_mask;
	result->green_position = multiboot->green_position;
	result->green_mask = multiboot->green_mask;
	result->blue_position = multiboot->blue_position;
	result->blue_mask = multiboot->blue_mask;
	return 1;
}

static int map_high_framebuffer(const framebuffer_t* framebuffer)
{
	const uint64_t two_megabytes = 0x200000ull;
	const uint64_t four_gigabytes = 0x100000000ull;
	const uint64_t eight_gigabytes = 0x200000000ull;
	uint64_t framebuffer_end = framebuffer->physical_address +
		(uint64_t)framebuffer->pitch * framebuffer->height;

	if (framebuffer->physical_address < four_gigabytes)
	{
		return 1;
	}

	if (framebuffer_end <= framebuffer->physical_address || framebuffer_end > eight_gigabytes)
	{
		return 0;
	}

	uint64_t first_page = framebuffer->physical_address & ~(two_megabytes - 1);
	uint64_t last_page = (framebuffer_end - 1) & ~(two_megabytes - 1);

	for (uint64_t page = first_page; page <= last_page; page += two_megabytes)
	{
		uint64_t directory_index = (page >> 30) - 4;
		uint64_t entry_index = (page >> 21) & 0x1FF;
		volatile uint64_t* directory = high_pd_tables + directory_index * 512;

		pdpt_table[4 + directory_index] =
			(uint64_t)(uintptr_t)directory | 0x03;
		directory[entry_index] = page | 0x83;
	}

	uint64_t cr3;
	__asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
	__asm__ volatile ("mov %0, %%cr3" : : "r"(cr3) : "memory");
	return 1;
}

void kernel_main64(uint32_t multiboot_magic, uint32_t multiboot_information)
{
	framebuffer_t framebuffer;
	serial_initialize();
	serial_write("VONORIT-OS x86_64 kernel entered\r\n");
	interrupts64_initialize();
	serial_write("IDT loaded; PIC remapped to vectors 32-47 and masked\r\n");
	serial_write("magic=0x");
	serial_write_hex32(multiboot_magic);
	serial_write(" info=0x");
	serial_write_hex32(multiboot_information);
	serial_write("\r\n");

	if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		serial_write("ERROR: invalid Multiboot signature\r\n");
		terminal_fallback("Invalid Multiboot signature.");
		return;
	}

	serial_write("Parsing Multiboot1 framebuffer data\r\n");
	if (multiboot_information != 0)
	{
		multiboot_info_t* info = (multiboot_info_t*)(uintptr_t)multiboot_information;
		serial_write("flags=0x");
		serial_write_hex32(info->flags);
		serial_write(" framebuffer=0x");
		serial_write_hex64(info->framebuffer_addr);
		serial_write(" pitch=0x");
		serial_write_hex32(info->framebuffer_pitch);
		serial_write(" width=0x");
		serial_write_hex32(info->framebuffer_width);
		serial_write(" height=0x");
		serial_write_hex32(info->framebuffer_height);
		serial_write(" bpp=0x");
		serial_write_hex32(info->framebuffer_bpp);
		serial_write(" type=0x");
		serial_write_hex32(info->framebuffer_type);
		serial_write("\r\n");
	}
	if (!find_framebuffer(multiboot_information, &framebuffer))
	{
		serial_write("WARNING: framebuffer unavailable; using VGA fallback\r\n");
		terminal_fallback("GRUB did not provide a direct-color framebuffer.");
		return;
	}

	serial_write("Multiboot1 framebuffer data accepted\r\n");
	if (!map_high_framebuffer(&framebuffer))
	{
		serial_write("ERROR: framebuffer address is outside the mapped range\r\n");
		terminal_fallback("Framebuffer address is outside the mapped range.");
		return;
	}

	serial_write("Framebuffer found; initializing modular desktop\r\n");
	pci_init();
	vfs_init();
	net_init();
	ui_initialize(&framebuffer);
	idt_initialize(&framebuffer);
	timer_initialize();
	input_initialize(&framebuffer);
	__asm__ volatile("sti");
	serial_write("Desktop initialized; keyboard and mouse enabled\r\n");
	uint64_t last_clock_second=0u;
	for (;;)
	{
		uint64_t seconds=timer_get_seconds();
		if(seconds!=last_clock_second)
		{
			last_clock_second=seconds;
			ui_set_uptime(seconds);
			timer_datetime_t datetime=timer_get_datetime();
			ui_set_datetime(datetime.hour,datetime.minute,datetime.second);
			ui_update_clock();
		}
		__asm__ volatile("hlt");
	}
}