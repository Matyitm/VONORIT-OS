#!/usr/bin/env bash
set -euo pipefail

QEMU_BIN="/mnt/c/Program Files/qemu/qemu-system-x86_64.exe"
ISO_WIN="C:\\Users\\Public\\vonoritos-x86_64.iso"
DISK_WIN="C:\\Users\\Public\\vonoritos-disk.img"

if [ -f "build/vonoritos-x86_64.iso" ]; then
    cp -f build/vonoritos-x86_64.iso /mnt/c/Users/Public/vonoritos-x86_64.iso
fi

"$QEMU_BIN" -m 512M -serial stdio -no-reboot -cdrom "$ISO_WIN" -drive file="$DISK_WIN",format=raw,if=ide -netdev user,id=n0 -device e1000,netdev=n0,mac=52:54:00:12:34:56
