#include <stdint.h>
#include "ktest.h"
#include "mm.h"
#include "vfs.h"
#include "sched.h"
#include "pci.h"
#include "vmx.h"
static void kt_app(char* out, uint16_t max, uint16_t* p, const char* s){
	if(!out) return;
	while(*s!=0 && (*p+1u)<max) out[(*p)++]=*s++;
}
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
int ktest_run_all(char* out, uint16_t max_len){
	int r[6], pass=0; uint16_t pos=0u;
	r[0]=kt_pmm(); r[1]=kt_heap(); r[2]=kt_vfs(); r[3]=kt_sched(); r[4]=kt_pci(); r[5]=kt_vmx();
	for(int i=0;i<6;++i) pass+=r[i];
	if(out && max_len>1u){
		if(pass==6) kt_app(out,max_len,&pos,"\nKTEST ALL PASS (6/6)");
		else{
			kt_app(out,max_len,&pos,"\nKTEST FAIL");
			for(int i=0;i<6;++i) if(!r[i]){ char d[4]={' ','t',(char)('1'+i),0}; kt_app(out,max_len,&pos,d); }
			char e[8]={' ','(',(char)('0'+pass),'/','6',')',0,0}; kt_app(out,max_len,&pos,e);
		}
		out[pos]=0;
	}
	return pass;
}
