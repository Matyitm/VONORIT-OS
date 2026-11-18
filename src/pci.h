#ifndef VONORIT_PCI_H
#define VONORIT_PCI_H

#include <stdint.h>

#define PCI_MAX_DEVICES 32u

typedef struct
{
	uint8_t bus;
	uint8_t slot;
	uint8_t func;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t class_code;
	uint8_t subclass;
	uint32_t bar0;
	uint8_t irq_line;
} pci_device_t;

void pci_init(void);
uint8_t pci_count(void);
const pci_device_t* pci_device(uint8_t index);
uint32_t pci_read32(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset);
void pci_write32(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset,uint32_t value);
uint16_t pci_read16(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset);
void pci_write16(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset,uint16_t value);
const char* pci_class_name(uint8_t class_code,uint8_t subclass);

#endif
