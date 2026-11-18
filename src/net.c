#include <stdint.h>

#include "net.h"
#include "pci.h"

#define NET_E1000_VENDOR 0x8086u
#define NET_E1000_DEVICE 0x100Eu
#define NET_RTL8139_VENDOR 0x10ECu
#define NET_RTL8139_DEVICE 0x8139u

#define RTL8139_IDR0 0x00u
#define RTL8139_MAR0 0x08u
#define E1000_RAL0 0x5400u
#define E1000_RAH0 0x5404u

static uint8_t nic_present;
static uint8_t nic_link;
static uint8_t mac_address[6];
static const char* nic_name="None";

static inline void outb(uint16_t port,uint8_t value){__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));}
static inline uint8_t inb(uint16_t port){uint8_t value;__asm__ volatile("inb %1,%0":"=a"(value):"Nd"(port));return value;}
static inline uint32_t inl(uint16_t port){uint32_t value;__asm__ volatile("inl %1,%0":"=a"(value):"Nd"(port));return value;}
static inline void mmio_outl(volatile uint32_t* address,uint32_t value){*address=value;}

static volatile uint32_t* map_e1000_mmio(uint32_t bar)
{
	if((bar&1u)!=0u) return 0;
	return (volatile uint32_t*)((uintptr_t)(bar&0xFFFFFFF0u));
}

void net_init(void)
{
	nic_present=0u;
nic_link=0u;
	nic_name="None";
	for(uint8_t i=0u;i<6u;++i) mac_address[i]=0u;
	for(uint8_t i=0u;i<pci_count();++i)
	{
		const pci_device_t* device=pci_device(i);
		if(device==0 || device->class_code!=0x02u) continue;
		if(device->vendor_id==NET_E1000_VENDOR && device->device_id==NET_E1000_DEVICE)
		{
			uint16_t command=pci_read16(device->bus,device->slot,device->func,4u);
			pci_write16(device->bus,device->slot,device->func,4u,(uint16_t)(command|0x06u));
			volatile uint32_t* mmio=map_e1000_mmio(device->bar0);
			if(mmio!=0)
			{
				uint32_t ral=(uint32_t)(mmio[E1000_RAL0/4u]);
				uint32_t rah=(uint32_t)(mmio[E1000_RAH0/4u]);
				mac_address[0]=(uint8_t)ral;
				mac_address[1]=(uint8_t)(ral>>8);
				mac_address[2]=(uint8_t)(ral>>16);
				mac_address[3]=(uint8_t)(ral>>24);
				mac_address[4]=(uint8_t)rah;
				mac_address[5]=(uint8_t)(rah>>8);
			}
			(void)mmio_outl;
			nic_present=1u;
			nic_name="Intel E1000";
			return;
		}
		if(device->vendor_id==NET_RTL8139_VENDOR && device->device_id==NET_RTL8139_DEVICE)
		{
			uint16_t command=pci_read16(device->bus,device->slot,device->func,4u);
			pci_write16(device->bus,device->slot,device->func,4u,(uint16_t)(command|0x05u));
			uint16_t io_base=(uint16_t)(device->bar0&0xFFFFFFFCu);
			if((device->bar0&1u)!=0u && io_base!=0u)
			{
				for(uint8_t j=0u;j<6u;++j) mac_address[j]=inb((uint16_t)(io_base+RTL8139_IDR0+j));
			}
			nic_present=1u;
			nic_name="Realtek RTL8139";
			return;
		}
	}
}

uint8_t net_is_present(void){return nic_present;}
const char* net_driver_name(void){return nic_name;}
const uint8_t* net_get_mac(void){return mac_address;}
uint8_t net_link_status(void){return nic_link;}
