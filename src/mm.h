#ifndef VONORIT_MM_H
#define VONORIT_MM_H

#include <stdint.h>
#include <stddef.h>

void pmm_init(void);
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t physical_address);
uint64_t pmm_total_pages(void);
uint64_t pmm_used_pages(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
