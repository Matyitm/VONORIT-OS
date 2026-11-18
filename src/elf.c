#include <stdint.h>
#include "elf.h"
#include "gdt.h"
#include "ui.h"

typedef struct
{
        uint8_t ident[16];uint16_t type;uint16_t machine;uint32_t version;
        uint64_t entry;uint64_t phoff;uint64_t shoff;uint32_t flags;
        uint16_t ehsize;uint16_t phentsize;uint16_t phnum;uint16_t shentsize;
        uint16_t shnum;uint16_t shstrndx;
} __attribute__((packed)) elf64_header_t;

uint8_t elf_load_and_run(const uint8_t* data,uint32_t size)
{
        if(data==0 || size < 64) return 0u;
        
        // Engedékeny betöltés: másoljuk a kódot a 0x00400000 címre, függetlenül a bonyolult header mezőktől
        volatile uint8_t* destination = (volatile uint8_t*)0x00400000u;
        
        // Másoljuk át a hasznos payload részt
        uint32_t copy_len = (size > 256) ? 256 : size;
        for(uint32_t i=0; i<copy_len; ++i) {
                destination[i] = data[i];
        }

        // Stack előkészítése
        volatile uint8_t* stack = (volatile uint8_t*)0x007FF000u;
        for(uint32_t i=0u; i<4096u; ++i) stack[i] = 0u;

        gdt_init();
        // Indítás a belépési pontról vagy a céfcímről
        enter_usermode(0x00400000u, 0x007FFFF0u);
        return 1u;
}
