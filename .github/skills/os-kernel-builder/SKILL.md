---
name: os-kernel-builder
description: Describe what this skill does and when to use it. Include keywords that help agents identify relevant tasks.
---

Define the functionality provided by this skill, including detailed instructions and examples.

---
name: vonorit-os-builder
description: Systems programming and low-level kernel development rules for VONORIT-OS (x86_64 freestanding C, NASM assembly, GRUB Multiboot, QEMU validation).
---

# VONORIT-OS Kernel Builder Guidelines

You are acting as a Principal Systems Software Engineer developing **VONORIT-OS**, a freestanding hybrid operating system targeting `x86_64` (with a preserved `i686` 32-bit bootstrap fallback).

## 1. Bootloader & Multiboot Consistency (CRITICAL)
- **Never mix Multiboot1 and Multiboot2 constants or structures.**
  - If `boot64.asm` uses a Multiboot1 header (`0x1BADB002`), the bootloader passes magic `0x2BADB002` in `EAX` and a flat `multiboot_info_t` struct in `EBX`.
  - If `boot64.asm` uses a Multiboot2 header (`0xE85250D6`), the bootloader passes magic `0x36D76289` in `EAX` and a tag-based info structure in `EBX`.
- Always preserve the Multiboot magic (`EAX`) and info pointer (`EBX`) at the very beginning of the 32-bit assembly entry before executing `CPUID` or zeroing page tables.

## 2. x86_64 Memory & Paging Safety
- **Prevent Triple Faults on MMIO / Framebuffer access:** Before dereferencing any hardware address provided by GRUB or ACPI (such as the linear framebuffer address, often located around `0xE0000000`–`0xFD000000`), verify that the page tables in `boot64.asm` identity-map (or higher-half map) that physical address range.
- When mapping memory with 2 MiB huge pages, ensure proper alignment and `Present | Writable` flags (`0x83`).
- Maintain a valid, 16-byte aligned stack (`RSP`) before calling into C (`kernel_main64`).

## 3. Freestanding C & Compiler Rules
- Do not include standard library headers that require an OS runtime (only use freestanding headers: `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<stdarg.h>`).
- All x86_64 kernel C files must be compiled with:
  `-ffreestanding -mno-red-zone -mcmodel=kernel -nostdlib -fno-builtin -Wall -Wextra`
- **Red Zone:** Never omit `-mno-red-zone` in kernel code, or hardware interrupts will silently corrupt stack data below `RSP`.
- For framebuffer rendering, explicitly validate `bits_per_pixel == 32` before casting pixel addresses to `uint32_t*`.

## 4. Architecture & Incremental Development
- **One subsystem per milestone:** Build and verify one layer at a time (e.g., Serial COM1 logging -> IDT & PIC -> PIT Timer -> PS/2 Keyboard/Mouse -> PMM/VMM).
- **Never break existing fallbacks:** Keep the 32-bit bootstrap (`make`, `make check`, `make run`) and the VGA text-mode fallback (`0xB8000`) functional at all times.
- Implement COM1 serial port (`0x3F8`) logging early so kernel diagnostics can be read directly from QEMU stdout/logs during automated checks.

## 5. Mandatory Verification Workflow
After modifying assembly, C kernel, linker scripts, or GRUB configs, always validate the build:
1. Run `make x86_64` to verify zero compiler/linker errors.
2. Run `make check64` (via `grub-file`) to confirm a valid Multiboot header.
3. Run `make iso64` to generate `build/vonoritos-x86_64.iso`.
4. Perform a QEMU smoke test (and inspect serial output or display initialization) before marking a milestone complete.
