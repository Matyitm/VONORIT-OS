#ifndef VONORIT_ATA_H
#define VONORIT_ATA_H

#include <stdint.h>

uint8_t ata_init(void);
uint8_t ata_read_sector(uint32_t lba, uint8_t* buffer);
uint8_t ata_write_sector(uint32_t lba, const uint8_t* buffer);
uint8_t ata_attached(void);

#endif
