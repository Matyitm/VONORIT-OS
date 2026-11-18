#include <stdint.h>

#include "pe.h"
#include "ui.h"

typedef struct
{
	uint16_t magic;uint8_t link_major;uint8_t link_minor;
	uint32_t text_size;uint32_t init_size;uint32_t uninit_size;
	uint32_t entry_rva;uint32_t code_rva;
	uint64_t image_base;uint32_t section_align;uint32_t file_align;
} __attribute__((packed)) pe_optional_min_t;

uint8_t pe_info(const uint8_t* data,uint32_t size)
{
	if(data==0 || size<0x40u) return 0u;
	if(data[0]!='M'||data[1]!='Z') return 0u;
	uint32_t pe_offset=(uint32_t)data[0x3C]|((uint32_t)data[0x3D]<<8)|((uint32_t)data[0x3E]<<16)|((uint32_t)data[0x3F]<<24);
	if(pe_offset+24u>size) return 0u;
	const uint8_t* pe=data+pe_offset;
	if(pe[0]!='P'||pe[1]!='E'||pe[2]!=0||pe[3]!=0) return 0u;
	uint16_t machine=(uint16_t)pe[4]|((uint16_t)pe[5]<<8);
	uint16_t sections=(uint16_t)pe[6]|((uint16_t)pe[7]<<8);
	uint16_t optional_size=(uint16_t)pe[20]|((uint16_t)pe[21]<<8);
	if(optional_size<0xF0u || pe_offset+24u+optional_size>size) return 0u;
	const pe_optional_min_t* opt=(const pe_optional_min_t*)(pe+24u);
	ui_terminal_append('\n');
	ui_terminal_append('P');ui_terminal_append('E');ui_terminal_append('3');ui_terminal_append('2');ui_terminal_append('+');ui_terminal_append(' ');
	ui_terminal_append(machine==0x8664u?'X':'?');ui_terminal_append(' ');
	ui_terminal_append(sections>9u?'M':'S');ui_terminal_append(' ');ui_terminal_append('R');ui_terminal_append('V');ui_terminal_append('A');
	if(opt->magic!=0x20Bu) ui_terminal_append(' ');
	else ui_terminal_append('+');
	return opt->magic==0x20Bu;
}
