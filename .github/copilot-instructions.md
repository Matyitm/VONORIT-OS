# Copilot Instructions

## Project Guidelines
- For VONORIT-OS x86_64 boot code, enable EFER.LME with `or eax, 1 << 8` (0x100), never bit 0; validate PML4/PDPT/PD links and use `-mno-red-zone` for 64-bit freestanding C.
- The VONORIT-OS project UI, terminal, system messages, documentation, and likely assistant-facing implementation text should be in English; no Hungarian localization unless explicitly requested.