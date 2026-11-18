#ifndef VONORIT_ELF_H
#define VONORIT_ELF_H

#include <stdint.h>

uint8_t elf_load_and_run(const uint8_t* data,uint32_t size);

#endif
