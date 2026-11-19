#include <stdint.h>
#include "cc.h"
#include "vfs.h"

typedef struct {
    uint8_t ident[16]; uint16_t type, machine; uint32_t version;
    uint64_t entry, phoff, shoff; uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} __attribute__((packed)) elf_ehdr_t;

typedef struct {
    uint32_t type, flags; uint64_t offset, vaddr, paddr, filesz, memsz, align;
} __attribute__((packed)) elf_phdr_t;

static int match_kw(const char* s, uint32_t pos, const char* kw) {
    uint32_t i = 0u;
    while (kw[i]) { if (s[pos + i] != kw[i]) return 0; ++i; }
    return 1;
}

int cc_compile_source(const char* src, const char* out_name) {
    if (!src || !out_name) return 0;
    uint8_t bin[1000];
    for (uint32_t i = 0u; i < sizeof(bin); ++i) bin[i] = 0u;

    uint32_t code_pos = 120u;
    uint32_t data_pos = 480u;
    uint32_t i = 0u;
    uint8_t has_stmt = 0u;

    while (src[i] != 0 && code_pos + 32u < 470u && data_pos + 64u < 990u) {
        if (match_kw(src, i, "printf(\"") || match_kw(src, i, "cout<<\"")) {
            i += (src[i] == 'p') ? 8u : 7u;
            uint32_t str_vaddr = 0x00400000u + data_pos;
            uint32_t len = 0u;
            while (src[i] != 0 && src[i] != '"' && data_pos + 2u < 990u) {
                if (src[i] == '\\' && src[i + 1u] == 'n') { bin[data_pos++] = 10u; i += 2u; ++len; }
                else { bin[data_pos++] = (uint8_t)src[i++]; ++len; }
            }
            if (src[i] == '"') ++i;
            if (data_pos < 995u) bin[data_pos++] = 0u;

            /* mov rax, 1 (SYS_WRITE); mov rdi, str_vaddr; mov rsi, len; int 0x80 */
            bin[code_pos++] = 0x48u; bin[code_pos++] = 0xC7u; bin[code_pos++] = 0xC0u;
            bin[code_pos++] = 1u; bin[code_pos++] = 0u; bin[code_pos++] = 0u; bin[code_pos++] = 0u;
            bin[code_pos++] = 0x48u; bin[code_pos++] = 0xC7u; bin[code_pos++] = 0xC7u;
            bin[code_pos++] = (uint8_t)str_vaddr; bin[code_pos++] = (uint8_t)(str_vaddr >> 8);
            bin[code_pos++] = (uint8_t)(str_vaddr >> 16); bin[code_pos++] = (uint8_t)(str_vaddr >> 24);
            bin[code_pos++] = 0x48u; bin[code_pos++] = 0xC7u; bin[code_pos++] = 0xC6u;
            bin[code_pos++] = (uint8_t)len; bin[code_pos++] = (uint8_t)(len >> 8);
            bin[code_pos++] = 0u; bin[code_pos++] = 0u;
            bin[code_pos++] = 0xCDu; bin[code_pos++] = 0x80u;
            has_stmt = 1u;
        } else if (match_kw(src, i, "window(\"")) {
            i += 8u;
            uint32_t t_vaddr = 0x00400000u + data_pos;
            while (src[i] != 0 && src[i] != '"' && data_pos + 2u < 990u) bin[data_pos++] = (uint8_t)src[i++];
            if (src[i] == '"') ++i;
            if (data_pos < 995u) bin[data_pos++] = 0u;
            while (src[i] != 0 && src[i] != '"') ++i;
            if (src[i] == '"') ++i;
            uint32_t b_vaddr = 0x00400000u + data_pos;
            while (src[i] != 0 && src[i] != '"' && data_pos + 2u < 990u) {
                if (src[i] == '\\' && src[i + 1u] == 'n') { bin[data_pos++] = 10u; i += 2u; }
                else bin[data_pos++] = (uint8_t)src[i++];
            }
            if (src[i] == '"') ++i;
            if (data_pos < 995u) bin[data_pos++] = 0u;

            /* mov rax, 10 (SYS_CREATE_WINDOW); mov rdi, t_vaddr; mov rsi, b_vaddr; int 0x80 */
            bin[code_pos++] = 0x48u; bin[code_pos++] = 0xC7u; bin[code_pos++] = 0xC0u;
            bin[code_pos++] = 10u; bin[code_pos++] = 0u; bin[code_pos++] = 0u; bin[code_pos++] = 0u;
            bin[code_pos++] = 0x48u; bin[code_pos++] = 0xC7u; bin[code_pos++] = 0xC7u;
            bin[code_pos++] = (uint8_t)t_vaddr; bin[code_pos++] = (uint8_t)(t_vaddr >> 8);
            bin[code_pos++] = (uint8_t)(t_vaddr >> 16); bin[code_pos++] = (uint8_t)(t_vaddr >> 24);
            bin[code_pos++] = 0x48u; bin[code_pos++] = 0xC7u; bin[code_pos++] = 0xC6u;
            bin[code_pos++] = (uint8_t)b_vaddr; bin[code_pos++] = (uint8_t)(b_vaddr >> 8);
            bin[code_pos++] = (uint8_t)(b_vaddr >> 16); bin[code_pos++] = (uint8_t)(b_vaddr >> 24);
            bin[code_pos++] = 0xCDu; bin[code_pos++] = 0x80u;
            has_stmt = 1u;
        } else {
            ++i;
        }
    }
    if (!has_stmt) return 0;

    /* mov rax, 60 (SYS_EXIT); xor rdi, rdi; int 0x80 */
    bin[code_pos++] = 0x48u; bin[code_pos++] = 0xC7u; bin[code_pos++] = 0xC0u;
    bin[code_pos++] = 60u; bin[code_pos++] = 0u; bin[code_pos++] = 0u; bin[code_pos++] = 0u;
    bin[code_pos++] = 0x48u; bin[code_pos++] = 0x31u; bin[code_pos++] = 0xFFu;
    bin[code_pos++] = 0xCDu; bin[code_pos++] = 0x80u;

    elf_ehdr_t* eh = (elf_ehdr_t*)bin;
    eh->ident[0] = 0x7Fu; eh->ident[1] = 'E'; eh->ident[2] = 'L'; eh->ident[3] = 'F';
    eh->ident[4] = 2u; eh->ident[5] = 1u; eh->ident[6] = 1u;
    eh->type = 2u; eh->machine = 62u; eh->version = 1u;
    eh->entry = 0x00400078u; eh->phoff = 64u; eh->ehsize = 64u; eh->phentsize = 56u; eh->phnum = 1u;

    elf_phdr_t* ph = (elf_phdr_t*)(bin + 64u);
    ph->type = 1u; ph->flags = 5u; ph->offset = 0u;
    ph->vaddr = 0x00400000u; ph->paddr = 0x00400000u;
    ph->filesz = data_pos; ph->memsz = data_pos; ph->align = 0x1000u;

    return vfs_write_bytes(out_name, bin, (uint16_t)data_pos) ? 1 : 0;
}
