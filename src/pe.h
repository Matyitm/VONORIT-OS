#ifndef VONORIT_PE_H
#define VONORIT_PE_H

#include <stdint.h>

uint8_t pe_info(const uint8_t* data, uint32_t size);
uint8_t pe_load_and_run(const uint8_t* data, uint32_t size);

#endif
