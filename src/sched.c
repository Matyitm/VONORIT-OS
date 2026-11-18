#include <stdint.h>
#include "sched.h"
#include "gdt.h"

typedef struct
{
    uint8_t ident[16]; uint16_t type; uint16_t machine; uint32_t version;
    uint64_t entry; uint64_t phoff; uint64_t shoff; uint32_t flags;
    uint16_t ehsize; uint16_t phentsize; uint16_t phnum; uint16_t shentsize;
    uint16_t shnum; uint16_t shstrndx;
} __attribute__((packed)) elf64_hdr_t;

typedef struct
{
    uint32_t type; uint32_t flags; uint64_t offset; uint64_t vaddr;
    uint64_t paddr; uint64_t filesz; uint64_t memsz; uint64_t align;
} __attribute__((packed)) elf64_ph_t;

static sched_proc_t procs[SCHED_MAX_PROCS];
static uint8_t kstacks[SCHED_MAX_PROCS][8192] __attribute__((aligned(16)));
static uint8_t current_idx;
static uint8_t sched_ready;

static void copy_proc_name(char* dst, const char* src)
{
    uint8_t i = 0u;
    while (src && src[i] != 0 && i < 23u) { dst[i] = src[i]; ++i; }
    dst[i] = 0;
}

void sched_init(void)
{
    for (uint8_t i = 0u; i < SCHED_MAX_PROCS; ++i)
    {
        procs[i].used = 0u; procs[i].state = 0u; procs[i].pid = i;
        procs[i].ticks = 0u; procs[i].rsp = 0u; procs[i].name[0] = 0;
    }
    procs[0].used = 1u; procs[0].state = 1u; procs[0].pid = 0u;
    copy_proc_name(procs[0].name, "DESKTOP");
    current_idx = 0u;
    sched_ready = 1u;
}

int sched_spawn_elf(const char* name, const uint8_t* data, uint32_t size)
{
    if (!sched_ready || data == 0 || size < sizeof(elf64_hdr_t)) return -1;
    const elf64_hdr_t* hdr = (const elf64_hdr_t*)data;
    if (hdr->ident[0] != 0x7Fu || hdr->ident[1] != 'E' || hdr->ident[2] != 'L' || hdr->ident[3] != 'F') return -1;
    if (hdr->ident[4] != 2u || hdr->machine != 62u) return -1;

    uint8_t slot = SCHED_MAX_PROCS;
    for (uint8_t i = 1u; i < SCHED_MAX_PROCS; ++i) if (!procs[i].used) { slot = i; break; }
    if (slot == SCHED_MAX_PROCS) return -1;

    uint64_t base_offset = (uint64_t)slot * 0x10000u;
    for (uint16_t i = 0u; i < hdr->phnum; ++i)
    {
        const elf64_ph_t* ph = (const elf64_ph_t*)(data + hdr->phoff + i * hdr->phentsize);
        if (ph->type != 1u) continue;
        uint64_t target = ph->vaddr + base_offset;
        if (target < 0x00400000u || target + ph->memsz > 0x00800000u || ph->offset + ph->filesz > size) return -1;
        volatile uint8_t* dst = (volatile uint8_t*)(uintptr_t)target;
        const uint8_t* src = data + (uint32_t)ph->offset;
        for (uint64_t b = 0u; b < ph->filesz; ++b) dst[b] = src[b];
        for (uint64_t b = ph->filesz; b < ph->memsz; ++b) dst[b] = 0u;
    }

    uint64_t user_stack_top = 0x007F0000u - (uint64_t)slot * 0x4000u - 16u;
    uint64_t* sp = (uint64_t*)(uintptr_t)&kstacks[slot][8192u];
    *(--sp) = 0x23u;
    *(--sp) = user_stack_top;
    *(--sp) = 0x202u;
    *(--sp) = 0x2Bu;
    *(--sp) = hdr->entry + base_offset;
    for (int r = 0; r < 15; ++r) *(--sp) = 0u;

    __asm__ volatile("cli");
    procs[slot].rsp = (uint64_t)(uintptr_t)sp;
    procs[slot].ticks = 0u;
    procs[slot].state = 1u;
    copy_proc_name(procs[slot].name, name);
    procs[slot].used = 1u;
    __asm__ volatile("sti");
    return (int)slot;
}

int sched_kill(uint16_t pid)
{
    if (pid == 0u || pid >= SCHED_MAX_PROCS || !procs[pid].used) return 0;
    __asm__ volatile("cli");
    procs[pid].used = 0u; procs[pid].state = 0u;
    if (current_idx == pid) current_idx = 0u;
    __asm__ volatile("sti");
    return 1;
}

void sched_exit_current(void)
{
    if (current_idx != 0u && current_idx < SCHED_MAX_PROCS)
    {
        procs[current_idx].used = 0u;
        procs[current_idx].state = 0u;
    }
}

uint64_t sched_on_tick(uint64_t current_rsp)
{
    if (!sched_ready) return current_rsp;
    if (procs[current_idx].used)
    {
        procs[current_idx].rsp = current_rsp;
        ++procs[current_idx].ticks;
    }
    for (uint8_t step = 1u; step <= SCHED_MAX_PROCS; ++step)
    {
        uint8_t next = (uint8_t)((current_idx + step) % SCHED_MAX_PROCS);
        if (procs[next].used && procs[next].state == 1u)
        {
            current_idx = next;
            return procs[next].rsp;
        }
    }
    current_idx = 0u;
    return procs[0].rsp;
}

const sched_proc_t* sched_get_proc(uint8_t index)
{
    if (index >= SCHED_MAX_PROCS || !procs[index].used) return 0;
    return &procs[index];
}
