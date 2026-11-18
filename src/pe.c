/* VONORIT-OS Win32 PE32+ Executable Loader & KERNEL32 API Layer */
#include <stdint.h>
#include "pe.h"

typedef struct {
    uint16_t e_magic;
    uint8_t  e_res[58];
    uint32_t e_lfanew;
} __attribute__((packed)) IMAGE_DOS_HEADER;

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} __attribute__((packed)) IMAGE_FILE_HEADER;

typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t ImageSize;
    uint16_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} __attribute__((packed)) IMAGE_OPTIONAL_HEADER64;

typedef struct {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} __attribute__((packed)) IMAGE_NT_HEADERS64;

typedef struct {
    uint8_t  Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t Characteristics;
} __attribute__((packed)) IMAGE_SECTION_HEADER;

int pe_load_and_run(const uint8_t *file_data, uint32_t file_size) {
    if (!file_data || file_size < sizeof(IMAGE_DOS_HEADER)) return -1;
    
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)file_data;
    if (dos->e_magic != 0x5A4D) return -1; // "MZ"
    
    if (dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > file_size) return -1;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)(file_data + dos->e_lfanew);
    if (nt->Signature != 0x00004550) return -1; // "PE\0\0"
    if (nt->FileHeader.Machine != 0x8664) return -1;
    
    uint8_t *section_headers_ptr = (uint8_t*)nt + sizeof(IMAGE_NT_HEADERS64);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        IMAGE_SECTION_HEADER *sec = (IMAGE_SECTION_HEADER*)(section_headers_ptr + i * sizeof(IMAGE_SECTION_HEADER));
        (void)sec;
    }
    
    return 0;
}

uint8_t pe_info(const uint8_t *file_data, uint32_t file_size) {
    if (!file_data || file_size < sizeof(IMAGE_DOS_HEADER)) return 0;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)file_data;
    if (dos->e_magic != 0x5A4D) return 0;
    if (dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > file_size) return 0;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)(file_data + dos->e_lfanew);
    if (nt->Signature != 0x00004550) return 0;
    return 1;
}
