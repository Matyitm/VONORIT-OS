#ifndef VONORIT_FRAMEBUFFER_H
#define VONORIT_FRAMEBUFFER_H

#include <stdint.h>

typedef struct
{
	volatile uint8_t* address;
	uint64_t physical_address;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bits_per_pixel;
	uint8_t red_position;
	uint8_t red_mask;
	uint8_t green_position;
	uint8_t green_mask;
	uint8_t blue_position;
	uint8_t blue_mask;
} framebuffer_t;

#endif
