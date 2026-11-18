# Compatibility Matrix

Status values: **Current foundation**, **Planned**, **Long-term**, or **Virtualized/emulated**.

| Target | Format or interface | Initial strategy | Status |
|---|---|---|---|
| VONORIT applications | VONORIT ABI | Native loader and system calls | Planned |
| Modern Linux applications | ELF, POSIX/Linux system calls | Native compatibility runtime | Planned |
| Linux distributions | Linux kernel and user space | Virtual machine or future hosted environment | Virtualized/emulated |
| Modern Windows applications | PE, Win32, selected NT APIs | User-space Windows compatibility runtime | Planned |
| Windows system drivers | WDM/NT kernel drivers | VONORIT driver model or virtual devices; no direct loading initially | Long-term/limited |
| PC DOS and IBM DOS | DOS, BIOS, x86 real mode | DOS-compatible CPU, BIOS, storage, and device emulation | Long-term |
| MS-DOS 1.x-6.x | DOS, BIOS, x86 real mode | DOS-compatible CPU, BIOS, storage, and device emulation | Long-term |
| DR-DOS, Novell DOS, and OpenDOS | DOS, BIOS, x86 real mode | DOS-compatible environment with variant-specific services | Long-term |
| FreeDOS | DOS, BIOS, x86 real mode | DOS-compatible environment with FreeDOS testing | Long-term |
| DOS Plus and Concurrent DOS | DOS, BIOS, x86 real mode | DOS-compatible environment with multitasking extensions | Long-term |
| DOS/V and regional DOS variants | DOS, BIOS, code pages, and text modes | DOS-compatible environment with locale and display emulation | Long-term |
| DOS extenders | DPMI, VCPI, XMS, EMS, and protected-mode DOS | CPU, memory, interrupt, and extender emulation | Long-term |
| DOS games and demoscene software | VGA, Sound Blaster, AdLib, MIDI, joystick, and timing APIs | Full DOS device and timing emulation | Long-term |
| DOS applications | DOS, BIOS, x86 real mode | Shared DOS compatibility test suite | Long-term |
| Windows 3.x/9x | Win16, Win9x APIs | Emulator and compatibility runtime | Long-term |
| Windows NT/2000/XP | PE, Win32, older DirectX | Windows runtime plus legacy shims | Long-term |
| Modern Windows guests | Windows kernel and drivers | Virtual machine, subject to licensing | Virtualized/emulated |
| macOS applications | Mach-O and Apple frameworks | Not a native ABI target; use an authorized guest environment where applicable | Virtualized/emulated |
| macOS guests | Darwin/XNU and macOS user space | Virtual machine only where hardware and licensing permit | Virtualized/emulated |
| Other Unix systems | ELF/Mach-O/POSIX variants | ABI adapters, emulation, or virtual machines per system | Long-term |

## Compatibility policy

“Supported” means the target has automated tests, documented limitations, and repeatable results on a defined hardware profile. A file-format loader alone does not provide operating-system compatibility.

Proprietary operating-system binaries, SDKs, firmware, and protected components are not included in this repository. Users must provide legally obtained guest media or application files where required.
