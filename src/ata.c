#include <stdint.h>

#include "ata.h"

#define ATA_PRIMARY_DATA 0x1F0u
#define ATA_PRIMARY_ERROR 0x1F1u
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2u
#define ATA_PRIMARY_LBA_LO 0x1F3u
#define ATA_PRIMARY_LBA_MID 0x1F4u
#define ATA_PRIMARY_LBA_HI 0x1F5u
#define ATA_PRIMARY_DRIVE 0x1F6u
#define ATA_PRIMARY_STATUS 0x1F7u
#define ATA_PRIMARY_COMMAND 0x1F7u
#define ATA_PRIMARY_CONTROL 0x3F6u

#define ATA_SR_BSY 0x80u
#define ATA_SR_DRQ 0x08u
#define ATA_SR_ERR 0x01u
#define ATA_CMD_READ_PIO 0x20u
#define ATA_CMD_WRITE_PIO 0x30u

static uint8_t ata_present;

static inline void outb(uint16_t port,uint8_t value){__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));}
static inline uint8_t inb(uint16_t port){uint8_t value;__asm__ volatile("inb %1,%0":"=a"(value):"Nd"(port));return value;}
static inline uint16_t inw(uint16_t port){uint16_t value;__asm__ volatile("inw %1,%0":"=a"(value):"Nd"(port));return value;}
static inline void outw(uint16_t port,uint16_t value){__asm__ volatile("outw %0,%1"::"a"(value),"Nd"(port));}

static uint8_t ata_wait_bsy(uint32_t timeout)
{
	while(timeout--!=0u)
	{
		uint8_t status=inb(ATA_PRIMARY_STATUS);
		if((status&ATA_SR_BSY)==0u) return 1u;
	}
	return 0u;
}

static uint8_t ata_wait_drq(uint32_t timeout)
{
	while(timeout--!=0u)
	{
		uint8_t status=inb(ATA_PRIMARY_STATUS);
		if((status&ATA_SR_BSY)==0u)
		{
			if((status&ATA_SR_ERR)!=0u) return 0u;
			if((status&ATA_SR_DRQ)!=0u) return 1u;
		}
	}
	return 0u;
}

static void ata_select_lba(uint32_t lba)
{
	outb(ATA_PRIMARY_DRIVE,(uint8_t)(0xE0u|((lba>>24)&0x0Fu)));
	outb(ATA_PRIMARY_SECTOR_COUNT,1u);
	outb(ATA_PRIMARY_LBA_LO,(uint8_t)lba);
	outb(ATA_PRIMARY_LBA_MID,(uint8_t)(lba>>8));
	outb(ATA_PRIMARY_LBA_HI,(uint8_t)(lba>>16));
}

uint8_t ata_init(void)
{
	ata_present=0u;
	outb(ATA_PRIMARY_CONTROL,0x02u);
	for(uint32_t i=0u;i<1000u;++i) (void)inb(ATA_PRIMARY_STATUS);
	outb(ATA_PRIMARY_DRIVE,0xA0u);
	outb(ATA_PRIMARY_SECTOR_COUNT,0u);
	outb(ATA_PRIMARY_LBA_LO,0u);
	outb(ATA_PRIMARY_LBA_MID,0u);
	outb(ATA_PRIMARY_LBA_HI,0u);
	outb(ATA_PRIMARY_COMMAND,0xECu);
	if(!ata_wait_bsy(100000u)) return 0u;
	uint8_t status=inb(ATA_PRIMARY_STATUS);
	if(status==0u || (status&ATA_SR_ERR)!=0u) return 0u;
	if(!ata_wait_drq(100000u)) return 0u;
	for(uint32_t i=0u;i<256u;++i) (void)inw(ATA_PRIMARY_DATA);
	ata_present=1u;
	return 1u;
}

uint8_t ata_read_sector(uint32_t lba,uint8_t* buffer)
{
	if(!ata_present || buffer==0) return 0u;
	if(!ata_wait_bsy(100000u)) return 0u;
	ata_select_lba(lba);
	outb(ATA_PRIMARY_COMMAND,ATA_CMD_READ_PIO);
	if(!ata_wait_drq(100000u)) return 0u;
	for(uint32_t i=0u;i<256u;++i)
	{
		uint16_t word=inw(ATA_PRIMARY_DATA);
		buffer[i*2u]=(uint8_t)word;
		buffer[i*2u+1u]=(uint8_t)(word>>8);
	}
	return 1u;
}

uint8_t ata_write_sector(uint32_t lba,const uint8_t* buffer)
{
	if(!ata_present || buffer==0) return 0u;
	if(!ata_wait_bsy(100000u)) return 0u;
	ata_select_lba(lba);
	outb(ATA_PRIMARY_COMMAND,ATA_CMD_WRITE_PIO);
	if(!ata_wait_drq(100000u)) return 0u;
	for(uint32_t i=0u;i<256u;++i)
		outw(ATA_PRIMARY_DATA,(uint16_t)buffer[i*2u]|((uint16_t)buffer[i*2u+1u]<<8));
	if(!ata_wait_bsy(100000u)) return 0u;
	inb(ATA_PRIMARY_STATUS);
	return 1u;
}

uint8_t ata_attached(void)
{
	return ata_present;
}
