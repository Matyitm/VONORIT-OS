# VONORIT-OS

VONORIT-OS is an experimental, freestanding 64-bit (x86_64 long mode) hybrid operating-system prototype written from scratch in C11 and NASM. It boots through GRUB (Multiboot1), draws a Windows-beta-style desktop, runs Ring 3 programs (ELF64 and a minimal Win32 PE32+ loader), schedules them preemptively and has a small live TCP/IP stack.

**Status: hobby prototype.** It is not a production OS and it is not yet Windows- or Linux-compatible. Everything below is described as it actually is today.

*Last updated: 2026-11-27*

## What is implemented

| Area | Details | Source |
|---|---|---|
| Boot | GRUB Multiboot1, long-mode switch, 4 GiB identity map, 800x600x24 framebuffer, serial log | `src/boot64.asm`, `src/kernel64.c` |
| Memory | 128 MB bitmap page allocator (4 KiB pages) and a 16 MB kernel heap; `kfree` tracks exact block counts, allocator paths are IRQ-safe | `src/mm.c` |
| Protection | 64-bit GDT/TSS, Ring 0 / Ring 3, per-task `TSS.RSP0` | `src/gdt.c`, `src/gdt_asm.asm` |
| Syscalls | `INT 0x80`: `WRITE=1`, `GET_TICKS=2`, `GET_UPTIME=3`, `CREATE_WINDOW=10`, `SEND_MSG=20`, `RECV_MSG=21`, `EXIT=60` | `src/syscall.c` |
| Executables | ELF64 loader; Win32 PE32+ loader with a few `KERNEL32.DLL` import stubs | `src/elf.c`, `src/pe.c` |
| Compiler | Tiny built-in C/C++ compiler (`cc`, `cpp`) that emits ELF64 binaries | `src/cc.c` |
| Scheduler | 100 Hz PIT round-robin, up to 8 processes; per-process page tables (CR3), `fxsave`/`fxrstor` state, per-task kernel stack | `src/sched.c` |
| GUI | RAM back-buffered desktop, Chicago / Whistler / Longhorn themes, draggable Z-ordered windows, PS/2 mouse, Hungarian/US keyboard layouts | `src/ui.c`, `src/input.c` |
| Storage | 16-slot RAM file system with optional ATA PIO disk persistence | `src/vfs.c`, `src/ata.c` |
| Network | PCI scan, Intel E1000 (DMA rings), ARP, ICMP ping, UDP DNS, TCP/HTTP GET, UDP chat | `src/pci.c`, `src/net.c`, `src/net_inet.c` |
| Hardware probe | `vmxinfo`: CPUID/MSR Intel VT-x detection, SATA AHCI and USB controller detection (detection only) | `src/vmx.c` |
| Self-tests | `ktest`: 7 kernel self-tests (page allocator, heap, VFS, scheduler state, PCI, `vmxinfo`, process isolation and page-leak check) | `src/ktest.c` |

## Verification status

| Item | State |
|---|---|
| Clean build with `-Wall -Wextra -Werror` | Passing |
| Boot test in QEMU (`make test64`, reaches "Desktop initialized") | Passing |
| `ktest` 7/7 at boot (includes per-process page isolation and no page leak after `kill`) | Passing |
| Manual QEMU test: `spawn worker.elf`, `ps`, `kill 1`, `meminfo` with per-process CR3 (all RAM free again after `kill`) | Passing (2026-11-26, interactive) |
| Scheduler-aware `SYS_EXIT`: `spawn hello.elf` exits by itself (`[EXIT CODE 0]`), process disappears from `ps`, RAM shows fully free | Passing (2026-11-27, interactive) |

## Known limitations

- `run` (the direct ELF/PE path) still loads into the shared user range and re-initialises the GDT on every launch. Only `spawn` uses per-process address spaces. Fixing this is next.
- Syscall pointers are checked by address range only (no `copy_from_user`); a bad user pointer can still fault the kernel.
- The PE loader uses a fixed image base, applies no relocations, and stubs only `GetStdHandle`, `WriteConsoleA`, `GetTickCount` and `ExitProcess`.
- The page allocator and heap assume RAM up to `0x0A000000` (160 MB). The Multiboot memory map is not parsed yet, so run QEMU with at least `-m 256`.
- The VFS holds 16 files of at most 1023 bytes each.
- The network stack assumes QEMU user-mode networking (static `10.0.2.x` addresses).
- No USB or AHCI driver and no VT-x guest engine yet (`vmxinfo` only detects hardware).
- The compatibility matrix in `docs/` is planning data, not a feature list.

## Build and run

Recommended environment: Windows 11 + WSL2 (Ubuntu), or native Linux.

Requirements: GNU Make, `gcc` (64-bit kernel), NASM, GRUB tools (`grub-mkrescue`, `grub-file`), `xorriso`, QEMU. The legacy 32-bit bootstrap additionally needs `i686-elf-gcc`.

```bash
bash scripts/build.sh iso    # build the ISO and copy it to C:\Users\Public
bash scripts/build.sh test   # build + headless QEMU boot test + ktest
bash scripts/build.sh run    # build + start the QEMU GUI
bash scripts/build.sh fresh  # zero the disk image, then start the QEMU GUI
```

After adding default files to `src/vfs.c`, reset the persistent disk image (`fresh` does this) because an old image overrides the defaults.

## Terminal commands

`help`, `ver`, `clear`, `uptime`, `ls`, `cat`, `write`, `notepad`, `sync`, `diskinfo`, `meminfo`, `winver`, `run`, `peinfo`, `cc`, `cpp`, `ps`, `spawn`, `kill`, `lspci`, `netinfo`, `ping`, `nslookup`, `curl`, `wget`, `chat`, `vmxinfo`, `ktest`, plus the theme commands.

## Documentation

- [docs/architecture.md](docs/architecture.md): compatibility architecture
- [docs/compatibility-matrix.md](docs/compatibility-matrix.md): target tracking

## Roadmap

1. Run many processes under load; make `meminfo` report exact pages, not only MB.
2. Move `run` onto the scheduler and remove the repeated `gdt_init()` calls.
3. `copy_from_user` style syscall pointer validation and clean process exit.
4. Parse the Multiboot memory map; make the allocator RAM-aware.
5. PE relocations and more `KERNEL32` functions.
6. SATA AHCI driver, then a VT-x guest engine (long term).

## Versions

Tags are created only for states that were verified (`bash scripts/tag.sh vX.Y.Z "message"` builds, runs the boot self-tests, then tags).

| Tag | Milestone |
|---|---|
| `v0.1.0` | Ring 3, ELF64/PE32+ loaders, built-in `cc`/`cpp` |
| `v0.2.0` | Live network (`wget`, `chat`) and `vmxinfo` |
| `v0.3.0` | `ktest` self-tests and the `build.sh` workflow |
| `v0.4.0` | Per-process page tables (CR3), per-task kernel stack, `fxsave`/`fxrstor`; verified by `ktest` t7 and a manual QEMU `spawn`/`ps`/`kill` run |

## License

Released under the [MIT License](LICENSE).
