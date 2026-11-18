#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

static uint8_t vga_entry_color(uint8_t foreground, uint8_t background)
{
	return (uint8_t)(foreground | (background << 4));
}

static uint16_t vga_entry(unsigned char character, uint8_t color)
{
	return (uint16_t)character | ((uint16_t)color << 8);
}

static void terminal_initialize(void)
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
}

static void terminal_putchar(char character)
{
	if (character == '\n')
	{
		terminal_column = 0;
		terminal_row++;
		return;
	}

	VGA_MEMORY[terminal_row * VGA_WIDTH + terminal_column] =
		vga_entry((unsigned char)character, terminal_color);

	if (++terminal_column == VGA_WIDTH)
	{
		terminal_column = 0;
		terminal_row++;
	}
}

static void terminal_write(const char* text)
{
	for (size_t index = 0; text[index] != '\0'; index++)
	{
		terminal_putchar(text[index]);
	}
}

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_information)
{
	(void)multiboot_information;
	terminal_initialize();
	terminal_write("VONORIT-OS\n");
	terminal_write("Kernel started successfully.\n\n");

	if (multiboot_magic == 0x2BADB002)
	{
		terminal_write("Multiboot information received.\n");
	}
	else
	{
		terminal_write("Warning: unexpected bootloader signature.\n");
	}

	terminal_write("Next: interrupts, memory management, drivers, and compatibility layers.");
}
