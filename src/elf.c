#include <stdint.h>

#include "elf.h"
#include "gdt.h"
#include "ui.h"

#define ELF_MAGIC0 0x7Fu
#define ELF_CLASS64 2u
#define ELF_MACHINE_X86_64 62u
#define ELF_PT_LOAD 1u

typedef struct
{
	uint8_t ident[16];uint16_t type;uint16_t machine;uint32_t version;
	uint64_t entry;uint64_t phoff;uint64_t shoff;uint32_t flags;
	uint16_t ehsize;uint16_t phentsize;uint16_t phnum;uint16_t shentsize;
	uint16_t shnum;uint16_t shstrndx;
} __attribute__((packed)) elf64_header_t;

typedef struct
{
	uint32_t type;uint32_t flags;uint64_t offset;uint64_t vaddr;
	uint64_t paddr;uint64_t filesz;uint64_t memsz;uint64_t align;
} __attribute__((packed)) elf64_phdr_t;

uint8_t elf_load_and_run(const uint8_t* data,uint32_t size)
{
	if(data==0 || size<sizeof(elf64_header_t)) return 0u;
	const elf64_header_t* header=(const elf64_header_t*)data;
	if(header->ident[0]!=ELF_MAGIC0 || header->ident[1]!='E' || header->ident[2]!='L' || header->ident[3]!='F') return 0u;
	if(header->ident[4]!=ELF_CLASS64 || header->machine!=ELF_MACHINE_X86_64) return 0u;
	if(header->phoff+header->phentsize*header->phnum>size || header->phentsize<sizeof(elf64_phdr_t)) return 0u;
	for(uint16_t i=0u;i<header->phnum;++i)
	{
		const elf64_phdr_t* ph=(const elf64_phdr_t*)(data+header->phoff+i*header->phentsize);
		if(ph->type!=ELF_PT_LOAD) continue;
		if(ph->vaddr<0x00400000u || ph->vaddr+ph->memsz>0x00800000u) return 0u;
		if(ph->offset+ph->filesz>size) return 0u;
		volatile uint8_t* destination=(volatile uint8_t*)(uintptr_t)ph->vaddr;
		const uint8_t* source=data+(uint32_t)ph->offset;
		for(uint64_t b=0u;b<ph->filesz;++b) destination[b]=source[b];
		for(uint64_t b=ph->filesz;b<ph->memsz;++b) destination[b]=0u;
	}
	volatile uint8_t* stack=(volatile uint8_t*)0x007FF000u;
	for(uint32_t i=0u;i<4096u;++i) stack[i]=0u;
	enter_usermode(header->entry,0x007FFFF0u);
	return 1u;
}
