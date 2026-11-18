#!/usr/bin/env bash
set -euo pipefail

ISO_WIN="/mnt/c/Users/Public/vonoritos-x86_64.iso"
DISK_WIN="/mnt/c/Users/Public/vonoritos-disk.img"
QEMU_BIN="/mnt/c/Program Files/qemu/qemu-system-x86_64.exe"
LOG1="build/boot1.log"
LOG2="build/boot2.log"

echo "=== [1/4] Tiszta forditas es ISO masolas ==="
make clean && make iso64
cp -f build/vonoritos-x86_64.iso "$ISO_WIN"

echo "=== [2/4] Friss 16 MB IDE lemezkep letrehozasa (1. boot: format + mentes) ==="
dd if=/dev/zero of="$DISK_WIN" bs=1M count=16 status=none
timeout 4s "$QEMU_BIN" -m 512M -display none -serial file:"$LOG1" -no-reboot -cdrom "C:\Users\Public\vonoritos-x86_64.iso" -drive file="C:\Users\Public\vonoritos-disk.img",format=raw,if=ide || true

grep -q "EXCEPTION" "$LOG1" && { echo "HIBA az 1. bootnal!"; cat "$LOG1"; exit 1; }
MAGIC=$(head -c 8 "$DISK_WIN" | tr -d '\0')
echo "LBA0 Magic az 1. boot utan: '$MAGIC'"
[ "$MAGIC" = "VONORFS1" ] || { echo "HIBA: Nem irta ki a VONORFS1 fejlecet!"; exit 1; }

echo "=== [3/4] Masodik boot teszt (visszatoltes a lemezrol) ==="
timeout 4s "$QEMU_BIN" -m 512M -display none -serial file:"$LOG2" -no-reboot -cdrom "C:\Users\Public\vonoritos-x86_64.iso" -drive file="C:\Users\Public\vonoritos-disk.img",format=raw,if=ide || true

grep -q "EXCEPTION" "$LOG2" && { echo "HIBA a 2. bootnal!"; cat "$LOG2"; exit 1; }
echo "A lemezrol valo visszatoltes is hiba nelkul lefutott!"
strings "$DISK_WIN" | head -n 12

echo "=== [4/4] Grafikus QEMU inditasa az ellenorzott lemezzel ==="
"$QEMU_BIN" -m 512M -serial stdio -no-reboot -cdrom "C:\Users\Public\vonoritos-x86_64.iso" -drive file="C:\Users\Public\vonoritos-disk.img",format=raw,if=ide
