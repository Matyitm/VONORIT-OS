#include <stdint.h>
#include <stddef.h>
#include "mm.h"

#define PMM_START 0x01000000u
#define PMM_SIZE (128u*1024u*1024u)
#define PMM_PAGE_SIZE 4096u
#define PMM_PAGE_COUNT (PMM_SIZE/PMM_PAGE_SIZE)
#define HEAP_START 0x09000000u
#define HEAP_SIZE (16u*1024u*1024u)
#define HEAP_BLOCK_SIZE 16u
#define HEAP_TOTAL_BLOCKS (HEAP_SIZE/HEAP_BLOCK_SIZE)
#define HEAP_MAGIC 0x564D4D3148454150ULL

static uint8_t page_bitmap[PMM_PAGE_COUNT/8u];
static uint64_t used_pages;
static uint8_t heap_bitmap[HEAP_TOTAL_BLOCKS/8u];

static inline uint64_t mm_irq_save(void){uint64_t rf;__asm__ volatile("pushfq; pop %0; cli":"=r"(rf)::"memory");return rf;}
static inline void mm_irq_restore(uint64_t rf){__asm__ volatile("push %0; popfq"::"r"(rf):"memory","cc");}
static void bitmap_set(uint8_t* bitmap,uint64_t index){bitmap[index>>3]|=(uint8_t)(1u<<(index&7u));}
static void bitmap_clear(uint8_t* bitmap,uint64_t index){bitmap[index>>3]&=(uint8_t)~(1u<<(index&7u));}
static uint8_t bitmap_test(uint8_t* bitmap,uint64_t index){return (bitmap[index>>3]&(uint8_t)(1u<<(index&7u)))!=0u;}

void pmm_init(void){
	uint64_t rf=mm_irq_save();
	for(uint64_t i=0u;i<sizeof(page_bitmap);++i) page_bitmap[i]=0u;
	for(uint64_t i=0u;i<sizeof(heap_bitmap);++i) heap_bitmap[i]=0u;
	used_pages=0u;
	mm_irq_restore(rf);
}
uint64_t pmm_alloc_page(void){
	uint64_t rf=mm_irq_save();
	for(uint64_t i=0u;i<PMM_PAGE_COUNT;++i){
		if(!bitmap_test(page_bitmap,i)){bitmap_set(page_bitmap,i);++used_pages;mm_irq_restore(rf);return PMM_START+i*PMM_PAGE_SIZE;}
	}
	mm_irq_restore(rf);return 0u;
}
void pmm_free_page(uint64_t physical_address){
	if(physical_address<PMM_START || physical_address>=PMM_START+PMM_SIZE) return;
	uint64_t index=(physical_address-PMM_START)/PMM_PAGE_SIZE;
	uint64_t rf=mm_irq_save();
	if(bitmap_test(page_bitmap,index)){bitmap_clear(page_bitmap,index);if(used_pages>0u)--used_pages;}
	mm_irq_restore(rf);
}
uint64_t pmm_total_pages(void){return PMM_PAGE_COUNT;}
uint64_t pmm_used_pages(void){return used_pages;}
static uint64_t heap_align(size_t size){return (uint64_t)((size+HEAP_BLOCK_SIZE-1u)/HEAP_BLOCK_SIZE);}
void* kmalloc(size_t size){
	if(size==0u) return 0;
	uint64_t blocks=heap_align(size)+1u;
	if(blocks>=HEAP_TOTAL_BLOCKS) return 0;
	uint64_t rf=mm_irq_save();
	for(uint64_t start=0u;start+blocks<=HEAP_TOTAL_BLOCKS;++start){
		uint64_t found=1u;
		for(uint64_t i=0u;i<blocks;++i) if(bitmap_test(heap_bitmap,start+i)){found=0u;break;}
		if(found){
			for(uint64_t i=0u;i<blocks;++i) bitmap_set(heap_bitmap,start+i);
			uint64_t* hdr=(uint64_t*)(uintptr_t)(HEAP_START+start*HEAP_BLOCK_SIZE);
			hdr[0]=HEAP_MAGIC;hdr[1]=blocks;mm_irq_restore(rf);
			return (void*)(uintptr_t)(HEAP_START+(start+1u)*HEAP_BLOCK_SIZE);
		}
	}
	mm_irq_restore(rf);return 0;
}
void kfree(void* ptr){
	if(ptr==0) return;
	uint64_t address=(uint64_t)(uintptr_t)ptr;
	if(address<HEAP_START+HEAP_BLOCK_SIZE || address>=HEAP_START+HEAP_SIZE || ((address-HEAP_START)%HEAP_BLOCK_SIZE)!=0u) return;
	uint64_t start=(address-HEAP_START)/HEAP_BLOCK_SIZE-1u;
	uint64_t rf=mm_irq_save();
	if(!bitmap_test(heap_bitmap,start)){mm_irq_restore(rf);return;}
	uint64_t* hdr=(uint64_t*)(uintptr_t)(HEAP_START+start*HEAP_BLOCK_SIZE);
	if(hdr[0]==HEAP_MAGIC && hdr[1]>1u && start+hdr[1]<=HEAP_TOTAL_BLOCKS){
		uint64_t blocks=hdr[1];hdr[0]=0u;hdr[1]=0u;
		for(uint64_t i=0u;i<blocks;++i) bitmap_clear(heap_bitmap,start+i);
	}
	mm_irq_restore(rf);
}
