# VONORIT-OS Hybrid Architecture

## Product direction

VONORIT-OS is a modern-first x86_64 platform with several compatibility modes. It does not attempt to reimplement every historical operating-system kernel inside one kernel. Each workload uses the lowest-cost mode that provides the required compatibility.

## Execution modes

### 1. Native VONORIT mode

Applications use the VONORIT kernel ABI and VONORIT system services directly. This is the preferred mode for core services, drivers, and new applications.

### 2. POSIX/Linux compatibility mode

Linux and POSIX applications run through a Linux-compatible system-call ABI, ELF loader, filesystem contract, signals, threads, and process model. The implementation target is application compatibility, not the claim that a Linux kernel is embedded in VONORIT-OS.

The first practical target is a small, testable POSIX subset, expanded according to real application test results.

### 3. Windows compatibility mode

Windows PE applications run through a user-space compatibility runtime. The runtime includes a PE loader, Win32 API libraries, synchronization primitives, registry/configuration services, graphics/audio adapters, and a Windows-like filesystem view.

Kernel-mode Windows drivers are not a first-class compatibility target. Hardware access must use VONORIT drivers or a controlled virtual-device interface.

### 4. Emulation and virtualization mode

- DOS families, Windows 3.x/9x, older CPUs, DOS extenders, and unusual legacy software use CPU/device emulation. The DOS target includes PC DOS, MS-DOS, DR-DOS/Novell DOS/OpenDOS, FreeDOS, DOS Plus/Concurrent DOS, and compatible DOS environments where their software and hardware assumptions can be reproduced.
- Complete Linux, Windows, and macOS guest operating systems use virtualization when the host hardware and licensing permit it.
- Virtual machines provide the strongest compatibility boundary because the guest retains its original kernel and system libraries.

## System layers

1. **Boot and kernel** - x86_64 long-mode boot, memory management, scheduling, interrupts, processes, system calls, and security. The current x86_64 milestone uses identity-mapped 2 MiB pages and VGA output.
2. **Hardware abstraction** - x86_64 CPU support, PCI, storage, graphics, input, networking, audio, and power management.
3. **Native services** - VONORIT init, IPC, filesystem services, device management, networking, and security policy.
4. **Compatibility runtimes** - POSIX/Linux ABI, Windows PE/Win32 runtime, and legacy emulation services.
5. **Virtualization** - virtual CPU, memory, storage, network, graphics, and guest lifecycle management.
6. **Desktop and developer tools** - shell, GUI, package manager, diagnostics, SDK, and compatibility test harness.

## Development rules

- Keep compatibility layers in user space whenever possible.
- Define stable interfaces between the kernel, drivers, and runtimes.
- Add an automated conformance test before declaring a compatibility target supported.
- Prefer virtualization for complete guest operating systems instead of copying their private kernel interfaces.
- Use open specifications and clean-room interfaces; do not redistribute proprietary operating-system binaries.
