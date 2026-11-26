#include <stdint.h>
#include "sched.h"
#include "gdt.h"
#include "mm.h"
typedef struct { uint8_t ident[16]; uint16_t type, machine; uint32_t version; uint64_t entry, phoff, shoff; uint32_t flags; uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx; } __attribute__((packed)) elf64_hdr_t;
typedef struct { uint32_t type, flags; uint64_t offset, vaddr, paddr, filesz, memsz, align; } __attribute__((packed)) elf64_ph_t;
extern uint64_t pml4_table[]; extern uint64_t pdpt_table[]; extern uint64_t pd_tables[];
static sched_proc_t procs[SCHED_MAX_PROCS];
static uint8_t kstacks[SCHED_MAX_PROCS][8192] __attribute__((aligned(16)));
static uint8_t fx_areas[SCHED_MAX_PROCS][512] __attribute__((aligned(16)));
static uint8_t current_idx, sched_ready;
static void copy_proc_name(char* dst, const char* src){ uint8_t i=0u; while(src&&src[i]!=0&&i<23u){dst[i]=src[i];++i;} dst[i]=0; }
static void zero_page(uint64_t phys){ volatile uint64_t* p=(volatile uint64_t*)(uintptr_t)phys; for(uint32_t i=0u;i<512u;++i) p[i]=0u; }
static uint64_t map_upage(uint64_t* pt, uint32_t idx){ if(pt[idx]&1u) return pt[idx]&~0xFFFULL; uint64_t pg=pmm_alloc_page(); if(!pg) return 0u; zero_page(pg); pt[idx]=pg|0x07u; return pg; }
static void free_proc_vm(uint8_t slot){
	uint64_t cr3=procs[slot].cr3; if(!cr3 || cr3==(uint64_t)(uintptr_t)pml4_table) return;
	uint64_t* pml4=(uint64_t*)(uintptr_t)cr3; uint64_t pdpt_p=pml4[0]&~0xFFFULL;
	if(pdpt_p){ uint64_t* pdpt=(uint64_t*)(uintptr_t)pdpt_p; uint64_t pd0_p=pdpt[0]&~0xFFFULL;
		if(pd0_p){ uint64_t* pd0=(uint64_t*)(uintptr_t)pd0_p;
			for(uint32_t d=2u;d<4u;++d){ uint64_t pt_p=pd0[d]&~0xFFFULL;
				if(pt_p && (pd0[d]&0x80u)==0u){ uint64_t* pt=(uint64_t*)(uintptr_t)pt_p; for(uint32_t t=0u;t<512u;++t) if(pt[t]&1u) pmm_free_page(pt[t]&~0xFFFULL); pmm_free_page(pt_p); }
			} pmm_free_page(pd0_p);
		} pmm_free_page(pdpt_p);
	} pmm_free_page(cr3); procs[slot].cr3=0u;
}
void sched_init(void){
	for(uint8_t i=0u;i<SCHED_MAX_PROCS;++i){ procs[i].used=0u;procs[i].state=0u;procs[i].pid=i;procs[i].ticks=0u;procs[i].rsp=0u;procs[i].cr3=0u;procs[i].name[0]=0; }
	for(uint16_t j=0u;j<512u;++j) fx_areas[0][j]=0u;
	fx_areas[0][0]=0x7Fu;fx_areas[0][1]=0x03u;fx_areas[0][24]=0x80u;fx_areas[0][25]=0x1Fu;
	procs[0].used=1u;procs[0].state=1u;procs[0].pid=0u;procs[0].cr3=(uint64_t)(uintptr_t)pml4_table;
	copy_proc_name(procs[0].name,"DESKTOP"); current_idx=0u; sched_ready=1u;
}
int sched_spawn_elf(const char* name, const uint8_t* data, uint32_t size){
	if(!sched_ready||data==0||size<sizeof(elf64_hdr_t)) return -1;
	const elf64_hdr_t* hdr=(const elf64_hdr_t*)data;
	if(hdr->ident[0]!=0x7Fu||hdr->ident[1]!='E'||hdr->ident[2]!='L'||hdr->ident[3]!='F'||hdr->ident[4]!=2u||hdr->machine!=62u) return -1;
	uint8_t slot=SCHED_MAX_PROCS; for(uint8_t i=1u;i<SCHED_MAX_PROCS;++i) if(!procs[i].used){slot=i;break;}
	if(slot==SCHED_MAX_PROCS) return -1;
	uint64_t pml4_p=pmm_alloc_page(), pdpt_p=pmm_alloc_page(), pd0_p=pmm_alloc_page(), pt_c_p=pmm_alloc_page(), pt_s_p=pmm_alloc_page();
	if(!pml4_p||!pdpt_p||!pd0_p||!pt_c_p||!pt_s_p){ if(pml4_p)pmm_free_page(pml4_p); if(pdpt_p)pmm_free_page(pdpt_p); if(pd0_p)pmm_free_page(pd0_p); if(pt_c_p)pmm_free_page(pt_c_p); if(pt_s_p)pmm_free_page(pt_s_p); return -1; }
	zero_page(pml4_p);zero_page(pdpt_p);zero_page(pd0_p);zero_page(pt_c_p);zero_page(pt_s_p);
	uint64_t *pml4=(uint64_t*)(uintptr_t)pml4_p, *pdpt=(uint64_t*)(uintptr_t)pdpt_p, *pd0=(uint64_t*)(uintptr_t)pd0_p, *ptc=(uint64_t*)(uintptr_t)pt_c_p, *pts=(uint64_t*)(uintptr_t)pt_s_p;
	pml4[0]=pdpt_p|0x07u; pdpt[0]=pd0_p|0x07u; for(uint32_t i=1u;i<4u;++i) pdpt[i]=pdpt_table[i]&~0x04ULL;
	for(uint32_t i=0u;i<512u;++i) pd0[i]=pd_tables[i]&~0x04ULL;
	pd0[2]=pt_c_p|0x07u; pd0[3]=pt_s_p|0x07u; procs[slot].cr3=pml4_p;
	for(uint16_t i=0u;i<hdr->phnum;++i){
		const elf64_ph_t* ph=(const elf64_ph_t*)(data+hdr->phoff+i*hdr->phentsize); if(ph->type!=1u) continue;
		if(ph->vaddr<0x00400000u||ph->vaddr+ph->memsz>0x007FC000u||ph->offset+ph->filesz>size){free_proc_vm(slot);return -1;}
		for(uint64_t va=ph->vaddr&~0xFFFULL;va<ph->vaddr+ph->memsz;va+=4096u){
			uint64_t pg=(va<0x00600000u)?map_upage(ptc,(uint32_t)((va-0x00400000u)>>12)):map_upage(pts,(uint32_t)((va-0x00600000u)>>12));
			if(!pg){free_proc_vm(slot);return -1;}
		}
		for(uint64_t b=0u;b<ph->filesz;++b){
			uint64_t va=ph->vaddr+b; uint64_t pg=(va<0x00600000u)?(ptc[(va-0x00400000u)>>12]&~0xFFFULL):(pts[(va-0x00600000u)>>12]&~0xFFFULL);
			((volatile uint8_t*)(uintptr_t)(pg+(va&0xFFFu)))[0]=data[ph->offset+b];
		}
	}
	for(uint32_t si=508u;si<512u;++si) if(!map_upage(pts,si)){free_proc_vm(slot);return -1;}
	for(uint16_t j=0u;j<512u;++j) fx_areas[slot][j]=0u;
	fx_areas[slot][0]=0x7Fu;fx_areas[slot][1]=0x03u;fx_areas[slot][24]=0x80u;fx_areas[slot][25]=0x1Fu;
	uint64_t* sp=(uint64_t*)(uintptr_t)&kstacks[slot][8192u];
	*(--sp)=0x23u; *(--sp)=0x007FFFF0u; *(--sp)=0x202u; *(--sp)=0x2Bu; *(--sp)=hdr->entry;
	for(int r=0;r<15;++r) *(--sp)=0u;
	uint64_t rf; __asm__ volatile("pushfq; pop %0; cli":"=r"(rf)::"memory");
	procs[slot].rsp=(uint64_t)(uintptr_t)sp; procs[slot].ticks=0u; procs[slot].state=1u;
	copy_proc_name(procs[slot].name,name); procs[slot].used=1u;
	__asm__ volatile("push %0; popfq"::"r"(rf):"memory","cc");
	return (int)slot;
}
int sched_kill(uint16_t pid){
	if(pid==0u||pid>=SCHED_MAX_PROCS||!procs[pid].used) return 0;
	uint64_t rf; __asm__ volatile("pushfq; pop %0; cli":"=r"(rf)::"memory");
	if(current_idx==pid) procs[pid].state=2u;
	else{ free_proc_vm((uint8_t)pid); procs[pid].used=0u; procs[pid].state=0u; }
	__asm__ volatile("push %0; popfq"::"r"(rf):"memory","cc"); return 1;
}
uint16_t sched_current_pid(void){return (uint16_t)current_idx;}
void sched_exit_current(void){ if(current_idx!=0u&&current_idx<SCHED_MAX_PROCS) procs[current_idx].state=2u; }
uint64_t sched_on_tick(uint64_t current_rsp){
	if(!sched_ready) return current_rsp;
	if(procs[current_idx].used){ procs[current_idx].rsp=current_rsp; ++procs[current_idx].ticks; }
	uint8_t prev=current_idx, next=0u;
	for(uint8_t step=1u;step<=SCHED_MAX_PROCS;++step){ uint8_t cand=(uint8_t)((current_idx+step)%SCHED_MAX_PROCS); if(procs[cand].used&&procs[cand].state==1u){next=cand;break;} }
	if(next!=prev){
		__asm__ volatile("fxsave (%0)"::"r"(fx_areas[prev]):"memory");
		current_idx=next;
		gdt_set_rsp0(next==0u?gdt_default_rsp0():(uint64_t)(uintptr_t)&kstacks[next][8192u]);
		uint64_t ncr3=procs[next].cr3; __asm__ volatile("mov %0,%%cr3"::"r"(ncr3):"memory");
		__asm__ volatile("fxrstor (%0)"::"r"(fx_areas[next]):"memory");
		if(procs[prev].used&&procs[prev].state==2u){ free_proc_vm(prev); procs[prev].used=0u; procs[prev].state=0u; }
	}
	return procs[next].rsp;
}
const sched_proc_t* sched_get_proc(uint8_t index){ if(index>=SCHED_MAX_PROCS||!procs[index].used) return 0; return &procs[index]; }
