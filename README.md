# VONORIT-OS

VONORIT-OS is an experimental, modern-first hybrid operating-system platform written primarily in C. Its long-term goal is to run native VONORIT software, modern Linux/POSIX applications, and Windows applications, while supporting older systems through emulation and complete operating systems through virtualization.

## Current milestone

- Freestanding 32-bit bootstrap and x86_64 long-mode kernel
- Multiboot-compatible GRUB boot and 32-bit/x86_64 ISO targets
- 800x600 RAM backbuffer GUI with interrupt-safe presentation
- Chicago, Whistler, and Longhorn beta desktop themes
- Draggable, focusable, Z-ordered Terminal, Files, About, Notepad, and WINVER windows
- IDT/PIC exception and hardware IRQ handling
- PS/2 keyboard and mouse support
- PIT 100 Hz uptime timer and CMOS RTC live clock
- In-memory RAM VFS with interactive Files browser and Notepad
- Optional primary ATA PIO disk persistence for VFS data
- 32-bit VGA fallback remains available

The repository includes a usable kernel desktop prototype, not a production-complete compatibility OS. Windows, Linux, POSIX, macOS, executable loading, processes, user mode, networking, and complete application compatibility remain staged work. The compatibility architecture is documented in [docs/architecture.md](docs/architecture.md), and target status is tracked in [docs/compatibility-matrix.md](docs/compatibility-matrix.md).

The primary x86_64 path enables long mode with identity-mapped pages and a framebuffer desktop. The 32-bit kernel remains available for bootstrap and legacy validation.

Applications are currently kernel-rendered demonstrations: Terminal, Files, Notepad, About, and WINVER. Real user-space applications require memory protection, scheduling, system calls, an executable loader, and IPC.

Terminal commands include `help`, `ver`, `clear`/`cls`, `uptime`/`time`, `ls`/`dir`, `cat`, `write`, `notepad`, `sync`, `diskinfo`, `winver`, and theme commands.

## Toolchain

The recommended development environment on Windows is WSL2 with a Linux distribution. Install:

- GNU Make
- NASM
- An `i686-elf-gcc` cross-compiler for the current bootstrap milestone
- GRUB utilities, including `grub-mkrescue` and `grub-file`
- QEMU
- `xorriso`, used by `grub-mkrescue`

A native Linux environment can use the same tools.

## Build

From the repository root:

```sh
make
make check
make run

# x86_64 path
make x86_64
make check64
make iso64
make run64
```

The 32-bit ISO is written to `build/vonoritos.iso`; the x86_64 ISO is written to `build/vonoritos-x86_64.iso`.

Run the graphical prototype with `make run64`. QEMU should open a framebuffer desktop when GRUB and the firmware provide the requested graphics mode. If no direct-color framebuffer is available, the kernel falls back to VGA text output with a diagnostic message.

If the cross-compiler uses a different prefix, override it:

```sh
make CROSS=i686-elf-
```

## Roadmap

1. Complete automated x86_64 boot, serial, disk, and desktop smoke tests.
2. Implement physical/virtual memory, heap allocation, scheduling, processes, user mode, IPC, and a stable system-call ABI.
3. Expand storage and partition/filesystem support beyond the RAM VFS and ATA PIO prototype.
4. Add PCI enumeration, keyboard/mouse abstraction, display, networking, and security services.
5. Convert kernel demo applications into ELF-based user-space programs with GUI IPC.
6. Implement a measured POSIX/Linux compatibility subset and conformance tests.
7. Implement PE loading and a Windows API runtime, beginning with console applications.
8. Add graphics, audio, networking, and installer/runtime integration.
9. Add DOS emulation and authorized Windows/Linux/macOS guest virtualization.

This milestone remains experimental and does not claim complete Windows, Linux, POSIX, or macOS compatibility. Native compatibility work uses public specifications and clean-room implementations. Compatibility claims require repeatable automated validation on a documented hardware profile.

The compatibility architecture is documented in [docs/architecture.md](docs/architecture.md), and target status is tracked in [docs/compatibility-matrix.md](docs/compatibility-matrix.md).
