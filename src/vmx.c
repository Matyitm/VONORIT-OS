#include <stdint.h>
#include "vmx.h"
#include "pci.h"
static void cpuid_q(uint32_t leaf, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d){
	uint32_t ra, rb, rc, rd;
	__asm__ volatile("cpuid" : "=a"(ra), "=b"(rb), "=c"(rc), "=d"(rd) : "a"(leaf), "c"(0u));
	*a = ra; *b = rb; *c = rc; *d = rd;
}
static uint64_t rdmsr_u64(uint32_t msr){
	uint32_t lo, hi;
	__asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return ((uint64_t)hi << 32) | (uint64_t)lo;
}
static void ap_str(char* out, uint16_t max, uint16_t* pos, const char* s){
	while (*s != 0 && (*pos + 1u) < max) out[(*pos)++] = *s++;
}
static void ap_hex16(char* out, uint16_t max, uint16_t* pos, uint16_t v){
	static const char hx[] = "0123456789ABCDEF";
	for (int s = 12; s >= 0 && (*pos + 1u) < max; s -= 4) out[(*pos)++] = hx[(v >> s) & 0xFu];
}
static void ap_u8(char* out, uint16_t max, uint16_t* pos, uint8_t v){
	char tmp[4]; uint8_t n = 0u;
	if (v == 0u) tmp[n++] = '0';
	while (v > 0u && n < 4u) { tmp[n++] = (char)('0' + (v % 10u)); v /= 10u; }
	while (n > 0u && (*pos + 1u) < max) out[(*pos)++] = tmp[--n];
}
void vmx_format_info(char* out, uint16_t max_len){
	if (out == 0 || max_len < 2u) return;
	uint16_t p = 0u; uint32_t max_leaf = 0u, ebx = 0u, ecx = 0u, edx = 0u; char ven[13];
	cpuid_q(0u, &max_leaf, &ebx, &ecx, &edx);
	for (uint8_t i = 0u; i < 4u; ++i) {
		ven[i] = (char)((ebx >> (i * 8u)) & 0xFFu);
		ven[4u + i] = (char)((edx >> (i * 8u)) & 0xFFu);
		ven[8u + i] = (char)((ecx >> (i * 8u)) & 0xFFu);
	}
	ven[12] = 0;
	uint8_t is_intel = (ebx == 0x756E6547u && edx == 0x49656E69u && ecx == 0x6C65746Eu) ? 1u : 0u;
	uint8_t vmx_sup = 0u;
	if (max_leaf >= 1u) {
		uint32_t a1 = 0u, b1 = 0u, c1 = 0u, d1 = 0u;
		cpuid_q(1u, &a1, &b1, &c1, &d1);
		if ((c1 & (1u << 5)) != 0u) vmx_sup = 1u;
	}
	ap_str(out, max_len, &p, "\nCPU: "); ap_str(out, max_len, &p, ven);
	ap_str(out, max_len, &p, "\nVT-X: ");
	if (is_intel && vmx_sup) {
		uint64_t fc = rdmsr_u64(0x3Au);
		ap_str(out, max_len, &p, "YES L="); ap_str(out, max_len, &p, (fc & 1u) ? "1" : "0");
		ap_str(out, max_len, &p, " EN="); ap_str(out, max_len, &p, (fc & 4u) ? "1" : "0");
	} else { ap_str(out, max_len, &p, vmx_sup ? "NON-INTEL" : "NO"); }
	uint8_t ahci_ok = 0u, usb_cnt = 0u, usb_pif = 0u;
	uint16_t ahci_vid = 0u, ahci_did = 0u; uint32_t ahci_bar5 = 0u;
	for (uint8_t i = 0u; i < pci_count(); ++i) {
		const pci_device_t* d = pci_device(i); if (d == 0) continue;
		if (d->class_code == 0x01u && d->subclass == 0x06u && !ahci_ok) {
			ahci_ok = 1u; ahci_vid = d->vendor_id; ahci_did = d->device_id;
			ahci_bar5 = pci_read32(d->bus, d->slot, d->func, 0x24u);
		}
		if (d->class_code == 0x0Cu && d->subclass == 0x03u) {
			++usb_cnt; usb_pif = (uint8_t)(pci_read32(d->bus, d->slot, d->func, 0x08u) >> 8);
		}
	}
	ap_str(out, max_len, &p, "\nAHCI: ");
	if (ahci_ok) {
		ap_hex16(out, max_len, &p, ahci_vid); ap_str(out, max_len, &p, ":");
		ap_hex16(out, max_len, &p, ahci_did); ap_str(out, max_len, &p, " B=");
		ap_hex16(out, max_len, &p, (uint16_t)(ahci_bar5 >> 16));
		ap_hex16(out, max_len, &p, (uint16_t)(ahci_bar5 & 0xFFFFu));
	} else { ap_str(out, max_len, &p, "NONE"); }
	ap_str(out, max_len, &p, "\nUSB: "); ap_u8(out, max_len, &p, usb_cnt);
	if (usb_cnt > 0u) ap_str(out, max_len, &p, usb_pif == 0x30u ? " (xHCI)" : (usb_pif == 0x20u ? " (EHCI)" : " (U/OHCI)"));
	out[p] = 0;
}
