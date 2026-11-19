#include <stdint.h>
#include "pe.h"
#include "gdt.h"
#include "syscall.h"

typedef struct { uint16_t e_magic; uint8_t e_res[58]; uint32_t e_lfanew; } __attribute__((packed)) dos_hdr_t;
typedef struct { uint16_t Machine, NumberOfSections; uint32_t TimeDateStamp, PtrSym, NumSym; uint16_t SizeOfOptionalHeader, Characteristics; } __attribute__((packed)) file_hdr_t;
typedef struct { uint32_t VirtualAddress, Size; } __attribute__((packed)) data_dir_t;
typedef struct {
    uint16_t Magic; uint8_t MajorLinker, MinorLinker;
    uint32_t SizeOfCode, SizeOfInitData, SizeOfUninitData, AddressOfEntryPoint, BaseOfCode;
    uint64_t ImageBase; uint32_t SectionAlignment, FileAlignment;
    uint16_t MajorOS, MinorOS, MajorImg, MinorImg, MajorSub, MinorSub;
    uint32_t Win32Version, SizeOfImage, SizeOfHeaders, CheckSum;
    uint16_t Subsystem, DllCharacteristics;
    uint64_t StackReserve, StackCommit, HeapReserve, HeapCommit;
    uint32_t LoaderFlags, NumberOfRvaAndSizes;
    data_dir_t DataDirectory[16];
} __attribute__((packed)) opt_hdr64_t;
typedef struct { uint32_t Signature; file_hdr_t FileHeader; opt_hdr64_t OptionalHeader; } __attribute__((packed)) nt_hdr64_t;
typedef struct { uint8_t Name[8]; uint32_t VirtualSize, VirtualAddress, SizeOfRawData, PointerToRawData, PtrReloc, PtrLine; uint16_t NumReloc, NumLine; uint32_t Characteristics; } __attribute__((packed)) sec_hdr_t;
typedef struct { uint32_t OriginalFirstThunk, TimeDateStamp, ForwarderChain, Name, FirstThunk; } __attribute__((packed)) imp_desc_t;

static int str_eq(const char* a, const char* b) {
    uint32_t i = 0u; while (a[i] && b[i]) { if (a[i] != b[i]) return 0; ++i; } return a[i] == 0 && b[i] == 0;
}

static void install_kernel32_stubs(uint64_t stub_base) {
    volatile uint8_t* m = (volatile uint8_t*)(uintptr_t)stub_base;
    /* +0x00: GetStdHandle(rcx) -> rax = rcx; ret */
    const uint8_t s_std[] = { 0x48,0x89,0xC8, 0xC3 };
    /* +0x20: WriteConsoleA(rcx=h, rdx=buf, r8=len, r9=written) -> SYS_WRITE(1, rdx, r8) */
    const uint8_t s_wri[] = { 0x57,0x56, 0x48,0x89,0xD7, 0x4C,0x89,0xC6, 0x48,0xC7,0xC0,1,0,0,0, 0xCD,0x80, 0x5E,0x5F, 0x48,0xC7,0xC0,1,0,0,0, 0xC3 };
    /* +0x50: GetTickCount() -> SYS_GET_TICKS(2) * 10 */
    const uint8_t s_tik[] = { 0x48,0xC7,0xC0,2,0,0,0, 0xCD,0x80, 0x48,0x6B,0xC0,10, 0xC3 };
    /* +0x70: ExitProcess(rcx=code) -> SYS_EXIT(60, rcx) */
    const uint8_t s_ext[] = { 0x48,0x89,0xCF, 0x48,0xC7,0xC0,60,0,0,0, 0xCD,0x80, 0xC3 };
    for (uint32_t i = 0u; i < sizeof(s_std); ++i) m[0x00u + i] = s_std[i];
    for (uint32_t i = 0u; i < sizeof(s_wri); ++i) m[0x20u + i] = s_wri[i];
    for (uint32_t i = 0u; i < sizeof(s_tik); ++i) m[0x50u + i] = s_tik[i];
    for (uint32_t i = 0u; i < sizeof(s_ext); ++i) m[0x70u + i] = s_ext[i];
}

uint8_t pe_info(const uint8_t* data, uint32_t size) {
    if (!data || size < sizeof(dos_hdr_t)) return 0u;
    const dos_hdr_t* dos = (const dos_hdr_t*)data;
    if (dos->e_magic != 0x5A4Du || dos->e_lfanew + sizeof(nt_hdr64_t) > size) return 0u;
    const nt_hdr64_t* nt = (const nt_hdr64_t*)(data + dos->e_lfanew);
    return (nt->Signature == 0x00004550u && nt->FileHeader.Machine == 0x8664u && nt->OptionalHeader.Magic == 0x020Bu) ? 1u : 0u;
}

uint8_t pe_load_and_run(const uint8_t* data, uint32_t size) {
    if (!pe_info(data, size)) return 0u;
    const dos_hdr_t* dos = (const dos_hdr_t*)data;
    const nt_hdr64_t* nt = (const nt_hdr64_t*)(data + dos->e_lfanew);
    uint64_t base = nt->OptionalHeader.ImageBase;
    if (base < 0x00400000u || base + nt->OptionalHeader.SizeOfImage > 0x00780000u) base = 0x00500000u;

    volatile uint8_t* dst = (volatile uint8_t*)(uintptr_t)base;
    for (uint32_t i = 0u; i < nt->OptionalHeader.SizeOfImage; ++i) dst[i] = 0u;
    uint32_t hdr_copy = nt->OptionalHeader.SizeOfHeaders < size ? nt->OptionalHeader.SizeOfHeaders : size;
    for (uint32_t i = 0u; i < hdr_copy; ++i) dst[i] = data[i];

    const sec_hdr_t* secs = (const sec_hdr_t*)((const uint8_t*)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
    for (uint16_t i = 0u; i < nt->FileHeader.NumberOfSections; ++i) {
        if (secs[i].PointerToRawData + secs[i].SizeOfRawData > size) return 0u;
        for (uint32_t b = 0u; b < secs[i].SizeOfRawData; ++b) dst[secs[i].VirtualAddress + b] = data[secs[i].PointerToRawData + b];
    }

    uint64_t stub_base = 0x00790000u;
    install_kernel32_stubs(stub_base);

    uint32_t imp_rva = nt->OptionalHeader.DataDirectory[1].VirtualAddress;
    if (imp_rva != 0u && imp_rva + sizeof(imp_desc_t) <= nt->OptionalHeader.SizeOfImage) {
        const imp_desc_t* imp = (const imp_desc_t*)(uintptr_t)(base + imp_rva);
        while (imp->Name != 0u && imp->FirstThunk != 0u) {
            uint64_t* iat = (uint64_t*)(uintptr_t)(base + imp->FirstThunk);
            uint32_t lookup_rva = imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk;
            const uint64_t* ilt = (const uint64_t*)(uintptr_t)(base + lookup_rva);
            for (uint32_t k = 0u; ilt[k] != 0u; ++k) {
                const char* fn = (const char*)(uintptr_t)(base + (uint32_t)(ilt[k] & 0x7FFFFFFFu) + 2u);
                if (str_eq(fn, "GetStdHandle")) iat[k] = stub_base + 0x00u;
                else if (str_eq(fn, "WriteConsoleA")) iat[k] = stub_base + 0x20u;
                else if (str_eq(fn, "GetTickCount")) iat[k] = stub_base + 0x50u;
                else iat[k] = stub_base + 0x70u;
            }
            ++imp;
        }
    }

    volatile uint8_t* stack = (volatile uint8_t*)0x007FF000u;
    for (uint32_t i = 0u; i < 4096u; ++i) stack[i] = 0u;
    gdt_init();
    syscall_initialize();
    enter_usermode(base + nt->OptionalHeader.AddressOfEntryPoint, 0x007FFFF0u);
    return 1u;
}
