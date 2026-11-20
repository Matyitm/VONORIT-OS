CROSS ?= i686-elf-
CC := $(CROSS)gcc
LD := $(CROSS)gcc
AS := nasm
GRUB_FILE := grub-file
GRUB_MKRESCUE := grub-mkrescue
QEMU := qemu-system-i386
CC64 := gcc
LD64 := gcc
QEMU64 := qemu-system-x86_64
WINDOWS_QEMU := $(shell if command -v qemu-system-i386.exe >/dev/null 2>&1; then command -v qemu-system-i386.exe; elif test -f '/mnt/c/Program Files/qemu/qemu-system-i386.exe'; then printf '%s' '/mnt/c/Program Files/qemu/qemu-system-i386.exe'; fi)
WINDOWS_QEMU64 := $(shell if command -v qemu-system-x86_64.exe >/dev/null 2>&1; then command -v qemu-system-x86_64.exe; elif test -f '/mnt/c/Program Files/qemu/qemu-system-x86_64.exe'; then printf '%s' '/mnt/c/Program Files/qemu/qemu-system-x86_64.exe'; fi)

CFLAGS := -std=gnu11 -ffreestanding -O2 -Wall -Wextra -Werror -m32 -fno-stack-protector -fno-pie -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables
CFLAGS64 := -std=gnu11 -ffreestanding -O2 -Wall -Wextra -Werror -m64 -mno-red-zone -mcmodel=kernel -mno-sse -mno-sse2 -mno-mmx -mno-80387 -fno-stack-protector -fno-pie -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables
LDFLAGS := -T linker.ld -ffreestanding -O2 -nostdlib -m32 -no-pie -Wl,--build-id=none

ifneq ($(strip $(WINDOWS_QEMU64)),)
QEMU_RUN64 := "$(WINDOWS_QEMU64)"
QEMU_RUN64_ISO = C:\Users\Public\vonoritos-x86_64.iso
QEMU_RUN64_STAGE = mkdir -p /mnt/c/Users/Public; rm -f /mnt/c/Users/Public/vonoritos-x86_64.iso; cp -f $(ISO64) /mnt/c/Users/Public/vonoritos-x86_64.iso
else
QEMU_RUN64 := qemu-system-x86_64
QEMU_RUN64_ISO = $(ISO64)
QEMU_RUN64_STAGE = @true
endif

ifneq ($(strip $(WINDOWS_QEMU)),)
QEMU_RUN := "$(WINDOWS_QEMU)"
QEMU_RUN_ISO = C:\Users\Public\vonoritos.iso
QEMU_RUN_STAGE = mkdir -p /mnt/c/Users/Public; rm -f /mnt/c/Users/Public/vonoritos.iso; cp -f $(ISO) /mnt/c/Users/Public/vonoritos.iso
else
QEMU_RUN := qemu-system-i386
QEMU_RUN_ISO = $(ISO)
QEMU_RUN_STAGE = @true
endif

BUILD := build
ISO_ROOT := $(BUILD)/isodir
KERNEL := $(BUILD)/vonoritos.kernel
ISO := $(BUILD)/vonoritos.iso
KERNEL64 := $(BUILD)/vonoritos-x86_64.kernel
ISO64_ROOT := $(BUILD)/isodir-x86_64
ISO64 := $(BUILD)/vonoritos-x86_64.iso
WINDOWS_ISO := /mnt/c/Users/Public/vonoritos.iso
WINDOWS_ISO64 := /mnt/c/Users/Public/vonoritos-x86_64.iso
WINDOWS_ISO_QEMU := C:\Users\Public\vonoritos.iso
WINDOWS_ISO64_QEMU := C:\Users\Public\vonoritos-x86_64.iso

.PHONY: all kernel iso check run x86_64 iso64 check64 run64 test64 clean

all: iso

kernel: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/boot.o: src/boot.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/kernel.o: src/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(BUILD)/boot.o $(BUILD)/kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/boot.o $(BUILD)/kernel.o

iso: $(ISO)

$(ISO): $(KERNEL) grub/grub.cfg
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(KERNEL) $(ISO_ROOT)/boot/vonoritos.kernel
	cp grub/grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

check: $(KERNEL)
	$(GRUB_FILE) --is-x86-multiboot $(KERNEL)

run: $(ISO)
	$(QEMU_RUN_STAGE)
	$(QEMU_RUN) -boot order=d -drive file="$(QEMU_RUN_ISO)",format=raw,media=cdrom

x86_64: $(KERNEL64)

$(BUILD)/boot64.o: src/boot64.asm | $(BUILD)
	$(AS) -f elf64 $< -o $@

$(BUILD)/kernel64.o: src/kernel64.c src/interrupts64.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/interrupts64.o: src/interrupts64.c src/interrupts64.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/interrupts64_asm.o: src/interrupts64.asm | $(BUILD)
	$(AS) -f elf64 $< -o $@

$(BUILD)/ui.o: src/ui.c src/ui.h src/framebuffer.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/idt.o: src/idt.c src/idt.h src/ui.h src/framebuffer.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/input.o: src/input.c src/input.h src/ui.h src/framebuffer.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/timer.o: src/timer.c src/timer.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/vfs.o: src/vfs.c src/vfs.h src/ata.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/ata.o: src/ata.c src/ata.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/pci.o: src/pci.c src/pci.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/net.o: src/net.c src/net.h src/pci.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/net_inet.o: src/net_inet.c src/net_inet.h src/net.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/mm.o: src/mm.c src/mm.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/gdt.o: src/gdt.c src/gdt.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/gdt_asm.o: src/gdt_asm.asm | $(BUILD)
	$(AS) -f elf64 $< -o $@

$(BUILD)/syscall.o: src/syscall.c src/syscall.h src/timer.h src/ui.h src/gdt.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/elf.o: src/elf.c src/elf.h src/gdt.h src/ui.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/pe.o: src/pe.c src/pe.h src/ui.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/sched.o: src/sched.c src/sched.h src/gdt.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/cc.o: src/cc.c src/cc.h src/vfs.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(BUILD)/vmx.o: src/vmx.c src/vmx.h src/pci.h | $(BUILD)
	$(CC64) $(CFLAGS64) -c $< -o $@

$(KERNEL64): $(BUILD)/boot64.o $(BUILD)/kernel64.o $(BUILD)/interrupts64.o $(BUILD)/interrupts64_asm.o $(BUILD)/ui.o $(BUILD)/idt.o $(BUILD)/input.o $(BUILD)/timer.o $(BUILD)/vfs.o $(BUILD)/ata.o $(BUILD)/pci.o $(BUILD)/net.o $(BUILD)/net_inet.o $(BUILD)/mm.o $(BUILD)/gdt.o $(BUILD)/gdt_asm.o $(BUILD)/syscall.o $(BUILD)/elf.o $(BUILD)/pe.o $(BUILD)/sched.o $(BUILD)/cc.o $(BUILD)/vmx.o linker64.ld
	$(LD64) -T linker64.ld -ffreestanding -O2 -nostdlib -m64 -no-pie -Wl,--build-id=none -o $@ $(BUILD)/boot64.o $(BUILD)/kernel64.o $(BUILD)/interrupts64.o $(BUILD)/interrupts64_asm.o $(BUILD)/ui.o $(BUILD)/idt.o $(BUILD)/input.o $(BUILD)/timer.o $(BUILD)/vfs.o $(BUILD)/ata.o $(BUILD)/pci.o $(BUILD)/net.o $(BUILD)/net_inet.o $(BUILD)/mm.o $(BUILD)/gdt.o $(BUILD)/gdt_asm.o $(BUILD)/syscall.o $(BUILD)/elf.o $(BUILD)/pe.o $(BUILD)/sched.o $(BUILD)/cc.o $(BUILD)/vmx.o

iso64: $(ISO64)

$(ISO64): $(KERNEL64) grub/grub64.cfg
	mkdir -p $(ISO64_ROOT)/boot/grub
	cp $(KERNEL64) $(ISO64_ROOT)/boot/vonoritos-x86_64.kernel
	cp grub/grub64.cfg $(ISO64_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO64_ROOT)

check64: $(KERNEL64)
	$(GRUB_FILE) --is-x86-multiboot $(KERNEL64)

run64: $(ISO64)
	$(QEMU_RUN64_STAGE)
	$(QEMU_RUN64) -boot order=d -drive file="$(QEMU_RUN64_ISO)",format=raw,media=cdrom

test64: $(ISO64)
	$(QEMU_RUN64_STAGE)
	@timeout 3s $(QEMU_RUN64) -boot order=d -drive file="$(QEMU_RUN64_ISO)",format=raw,media=cdrom -display none -serial stdio -monitor none -no-reboot -no-shutdown >$(BUILD)/test64.log 2>&1; status=$$?; cat $(BUILD)/test64.log; if [ $$status -ne 0 ] && [ $$status -ne 124 ]; then exit $$status; fi; grep -q "Desktop initialized" $(BUILD)/test64.log

clean:
	rm -rf $(BUILD)
