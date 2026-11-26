#include <stdint.h>
#include "ktest.h"
#include "mm.h"
#include "vfs.h"
#include "sched.h"
#include "pci.h"
#include "vmx.h"
#define KT_TESTS 7
static void kt_app(char* out, uint16_t max, uint16_t* p, const char* s){
	if(!out) return;
	while(*s!=0 && (*p+1u)<max) out[(*p)++]=*s++;
}
static inline uint64_t kt_irq_save(void){uint64_t rf;__asm__ volatile("pushfq; pop %0; cli":"=r"(rf)::"memory");return rf;}
static inline void kt_irq_restore(uint64_t rf){__asm__ volatile("push %0; popfq"::"r"(rf):"memory","cc");}
static int kt_pmm(void){
	uint64_t u0=pmm_used_pages(), p1=pmm_alloc_page(), p2=pmm_alloc_page();
	int ok=0;
	if(p1 && p2 && p1!=p2 && (p1&0xFFFu)==0u && (p2&0xFFFu)==0u){
		volatile uint64_t* w=(volatile uint64_t*)(uintptr_t)p1; *w=0xCAFEBABE12345678ULL;
		ok=(*w==0xCAFEBABE12345678ULL);
	}
	if(p1) pmm_free_page(p1);
	if(p2) pmm_free_page(p2);
	return ok && pmm_used_pages()==u0;
}
static int kt_heap(void){
	uint8_t *a=(uint8_t*)kmalloc(32u), *b=(uint8_t*)kmalloc(32u);
	if(!(a && b && a!=b)){ if(a)kfree(a); if(b)kfree(b); return 0; }
	for(uint8_t i=0u;i<32u;++i){a[i]=0xAAu;b[i]=0xBBu;}
	kfree(a); kfree(a);
	if(b[0]!=0xBBu || b[31]!=0xBBu){ kfree(b); return 0; }
	kfree(b);
	uint8_t* c=(uint8_t*)kmalloc(32u); int ok=(c==a);
	if(c) kfree(c);
	return ok;
}
static int kt_vfs(void){
	uint8_t elf[64]; uint16_t el=vfs_read_bytes("HELLO.ELF",elf,64u);
	const char* rd=vfs_read("README.TXT");
	return vfs_count()>=1u && rd && rd[0]!=0 && el>=4u && elf[0]==0x7Fu && elf[1]=='E' && elf[2]=='L' && elf[3]=='F';
}
static int kt_sched(void){
	const sched_proc_t* d0=sched_get_proc(0u);
	return d0 && d0->pid==0u && d0->cr3!=0u && d0->name[0]=='D';
}
static int kt_pci(void){ return pci_count()>=1u && pci_device(0u)!=0; }
static int kt_vmx(void){
	char vb[160]; vmx_format_info(vb,(uint16_t)sizeof(vb));
	return vb[0]=='\n' && vb[1]=='C' && vb[2]=='P' && vb[3]=='U';
}
/* Physical address of the page mapped at virtual 0x00400000 in a process address space. */
static uint64_t kt_user_phys(uint64_t cr3){
	uint64_t* pml4=(uint64_t*)(uintptr_t)cr3;
	if(!(pml4[0]&1u)) return 0u;
	uint64_t* pdpt=(uint64_t*)(uintptr_t)(pml4[0]&~0xFFFULL);
	if(!(pdpt[0]&1u)) return 0u;
	uint64_t* pd=(uint64_t*)(uintptr_t)(pdpt[0]&~0xFFFULL);
	if(!(pd[2]&1u) || (pd[2]&0x80u)) return 0u;
	uint64_t* pt=(uint64_t*)(uintptr_t)(pd[2]&~0xFFFULL);
	if(!(pt[0]&1u)) return 0u;
	return pt[0]&~0xFFFULL;
}
/* Spawns WORKER.ELF twice with interrupts off, checks separate CR3 and separate
   physical pages for the same virtual address, then kills both and checks that
   every page was returned (no leak). Does not test execution or preemption. */
static int kt_proc(void){
	uint8_t elf[256]; uint16_t n=vfs_read_bytes("WORKER.ELF",elf,256u);
	if(n<64u) return 0;
	uint64_t rf=kt_irq_save();
	uint64_t base=pmm_used_pages();
	int a=sched_spawn_elf("KT1",elf,n), b=sched_spawn_elf("KT2",elf,n);
	int ok=0;
	if(a>0 && b>0 && a!=b){
		const sched_proc_t* pa=sched_get_proc((uint8_t)a);
		const sched_proc_t* pb=sched_get_proc((uint8_t)b);
		if(pa && pb && pa->cr3!=0u && pb->cr3!=0u && pa->cr3!=pb->cr3){
			uint64_t fa=kt_user_phys(pa->cr3), fb=kt_user_phys(pb->cr3);
			ok=(fa!=0u && fb!=0u && fa!=fb);
		}
	}
	if(a>0) (void)sched_kill((uint16_t)a);
	if(b>0) (void)sched_kill((uint16_t)b);
	ok=ok && (pmm_used_pages()==base);
	kt_irq_restore(rf);
	return ok;
}
int ktest_run_all(char* out, uint16_t max_len){
	int r[KT_TESTS], pass=0; uint16_t pos=0u;
	r[0]=kt_pmm(); r[1]=kt_heap(); r[2]=kt_vfs(); r[3]=kt_sched(); r[4]=kt_pci(); r[5]=kt_vmx(); r[6]=kt_proc();
	for(int i=0;i<KT_TESTS;++i) pass+=r[i];
	if(out && max_len>1u){
		if(pass==KT_TESTS) kt_app(out,max_len,&pos,"\nKTEST ALL PASS (7/7)");
		else{
			kt_app(out,max_len,&pos,"\nKTEST FAIL");
			for(int i=0;i<KT_TESTS;++i) if(!r[i]){ char d[4]={' ','t',(char)('1'+i),0}; kt_app(out,max_len,&pos,d); }
			char e[8]={' ','(',(char)('0'+pass),'/','7',')',0,0}; kt_app(out,max_len,&pos,e);
		}
		out[pos]=0;
	}
	return pass;
}
