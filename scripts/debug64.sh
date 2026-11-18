#!/usr/bin/env bash
set -euo pipefail

ISO_LOCAL="build/vonoritos-x86_64.iso"
ISO_WIN="/mnt/c/Users/Public/vonoritos-x86_64.iso"
DISK_WIN="/mnt/c/Users/Public/vonoritos-disk.img"
QEMU_BIN="/mnt/c/Program Files/qemu/qemu-system-x86_64.exe"
LOG_FILE="build/qemu-debug.log"

echo "[1/4] Forditas ellenorzese..."
make clean && make iso64
cp -f "$ISO_LOCAL" "$ISO_WIN"

echo "[2/4] Kernel szimbolumok ellenorzese..."
nm build/vonoritos-x86_64.kernel | grep -E "kernel_main64|ata_init|vfs_init|ui_initialize|idt_initialize"

if [ ! -f "$DISK_WIN" ]; then
    echo "[3/4] 16 MB virtualis merevlemez letrehozasa..."
    dd if=/dev/zero of="$DISK_WIN" bs=1M count=16 status=none
fi

echo "[4/4] QEMU automata soros es lemez-teszt (3 mp)..."
rm -f "$LOG_FILE"
timeout 4s "$QEMU_BIN" -m 512M -display none -serial file:"$LOG_FILE" -no-reboot -cdrom "C:\Users\Public\vonoritos-x86_64.iso" -drive file="C:\Users\Public\vonoritos-disk.img",format=raw,if=ide || true

echo "--- SOROS NAPLO ---"
cat "$LOG_FILE" || true
echo "-------------------"

if grep -q "EXCEPTION" "$LOG_FILE"; then
    echo "HIBA: CPU kivetel (BSOD) tortent inditaskor!"
    exit 1
fi

MAGIC=$(head -c 8 "$DISK_WIN" | tr -d '\0')
echo "Lemez LBA0 fejlec: '${MAGIC}'"

if [ "${1:-}" = "--gui" ]; then
    echo "Grafikus QEMU inditasa..."
    "$QEMU_BIN" -m 512M -serial stdio -no-reboot -cdrom "C:\Users\Public\vonoritos-x86_64.iso" -drive file="C:\Users\Public\vonoritos-disk.img",format=raw,if=ide
fi
