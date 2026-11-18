# VONORIT-OS

VONORIT-OS is an experimental, modern-first hybrid operating-system platform written primarily in C. Its long-term goal is to run native VONORIT software, modern Linux/POSIX applications, and Windows applications, while supporting older systems through emulation and complete operating systems through virtualization.

## Current milestone

- Freestanding C kernel entry point for 32-bit bootstrap and x86_64 paths
- Multiboot-compatible GRUB boot
- Linker script and kernel stack
- 32-bit and x86_64 ISO image generation
- 32-bit and x86_64 QEMU launch targets
- First x86_64 framebuffer desktop prototype

This repository currently contains only the initial 32-bit x86 kernel foundation. It is not yet compatible with Windows, Linux, or macOS applications. The compatibility architecture is documented in [docs/architecture.md](docs/architecture.md), and target status is tracked in [docs/compatibility-matrix.md](docs/compatibility-matrix.md).

The primary production target is now x86_64. The x86_64 path enables long mode with identity-mapped 2 MiB pages and currently displays kernel startup text. The 32-bit kernel remains available as a bootstrap and legacy validation target.

The first graphical milestone draws a desktop, taskbar, and three built-in demo program windows: Terminal, Files, and About. These are currently kernel-rendered demonstrations; real applications require user mode, processes, a filesystem, keyboard/mouse input, and an executable loader.

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

1. Complete the x86_64 bootable kernel milestone and add automated image checks.
2. Add x86_64 interrupts, timers, keyboard/mouse input, and a hardware-abstraction layer.
3. Implement physical/virtual memory, scheduling, processes, user mode, IPC, and a stable system-call ABI.
4. Add storage, filesystems, PCI, input, display, networking, and security services.
5. Convert the demo windows into user-space programs with an ELF loader and GUI IPC protocol.
6. Implement a measured POSIX/Linux compatibility subset and its conformance tests.
7. Implement PE loading and a user-space Windows API runtime, beginning with console applications.
8. Add graphics, audio, networking, and installer/runtime integration for larger application classes.
9. Add broad DOS emulation—including PC DOS, MS-DOS, DR-DOS/Novell DOS/OpenDOS, FreeDOS, DOS Plus/Concurrent DOS, DOS/V, DOS extenders, and common DOS gaming hardware—then add legacy Windows emulation and virtualization for complete Linux, Windows, and authorized macOS guest environments.

Compatibility claims require automated tests on a defined hardware profile. A loader or emulator that starts an application is not, by itself, full operating-system compatibility.
