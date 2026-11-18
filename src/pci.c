#include <stdint.h>

#include "pci.h"

#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA 0xCFCu

static pci_device_t devices[PCI_MAX_DEVICES];
static uint8_t device_count;

static inline void outb(uint16_t port,uint8_t value){__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));}
static inline void outl(uint16_t port,uint32_t value){__asm__ volatile("outl %0,%1"::"a"(value),"Nd"(port));}
static inline uint8_t inb(uint16_t port){uint8_t value;__asm__ volatile("inb %1,%0":"=a"(value):"Nd"(port));return value;}
static inline uint32_t inl(uint16_t port){uint32_t value;__asm__ volatile("inl %1,%0":"=a"(value):"Nd"(port));return value;}

uint32_t pci_read32(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset)
{
	uint32_t address=(uint32_t)0x80000000u|((uint32_t)bus<<16)|((uint32_t)slot<<11)|((uint32_t)func<<8)|(uint32_t)(offset&0xFCu);
	outl(PCI_CONFIG_ADDRESS,address);
	return inl(PCI_CONFIG_DATA);
}

void pci_write32(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset,uint32_t value)
{
	uint32_t address=(uint32_t)0x80000000u|((uint32_t)bus<<16)|((uint32_t)slot<<11)|((uint32_t)func<<8)|(uint32_t)(offset&0xFCu);
	outl(PCI_CONFIG_ADDRESS,address);outl(PCI_CONFIG_DATA,value);
}

uint16_t pci_read16(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset)
{
	uint32_t value=pci_read32(bus,slot,func,offset);
	return (uint16_t)((value>>(8u*(offset&2u)))&0xFFFFu);
}

void pci_write16(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset,uint16_t value)
{
	uint32_t old=pci_read32(bus,slot,func,offset);
	uint32_t shift=(uint32_t)(8u*(offset&2u));
	uint32_t replacement=((uint32_t)value<<shift)| (old&~((uint32_t)0xFFFFu<<shift));
	pci_write32(bus,slot,func,offset,replacement);
}

void pci_init(void)
{
	device_count=0u;
	for(uint8_t bus=0u;bus<8u;++bus)
	{
		for(uint8_t slot=0u;slot<32u;++slot)
		{
			for(uint8_t func=0u;func<8u;++func)
			{
				uint32_t id=pci_read32(bus,slot,func,0u);
				uint16_t vendor=(uint16_t)(id&0xFFFFu);
				if(vendor==0xFFFFu) continue;
				if(device_count>=PCI_MAX_DEVICES) return;
				uint16_t device=(uint16_t)(id>>16);
				uint32_t class_rev=pci_read32(bus,slot,func,8u);
				uint8_t class_code=(uint8_t)(class_rev>>24);
				uint8_t subclass=(uint8_t)(class_rev>>16);
				uint32_t bar=pci_read32(bus,slot,func,16u);
				uint32_t irq=pci_read32(bus,slot,func,60u);
				pci_device_t* entry=&devices[device_count++];
				entry->bus=bus;entry->slot=slot;entry->func=func;
				entry->vendor_id=vendor;entry->device_id=device;
				entry->class_code=class_code;entry->subclass=subclass;
				entry->bar0=bar;entry->irq_line=(uint8_t)(irq&0xFFu);
			}
		}
	}
}

uint8_t pci_count(void){return device_count;}
const pci_device_t* pci_device(uint8_t index){if(index>=device_count)return 0;return &devices[index];}

const char* pci_class_name(uint8_t class_code,uint8_t subclass)
{
	if(class_code==0x00u) return "Host Bridge";
	if(class_code==0x01u) return subclass==0x01u?"IDE Controller":"Mass Storage";
	if(class_code==0x02u) return "Ethernet Controller";
	if(class_code==0x03u) return "VGA Controller";
	if(class_code==0x06u) return subclass==0x01u?"ISA Bridge":"PCI Bridge";
	if(class_code==0x0Cu) return "USB/Serial Bus";
	return "Unknown Device";
}
