#!/usr/bin/env bash
set -euo pipefail
CMD="${1:-iso}"
make clean && make iso64
mkdir -p /mnt/c/Users/Public
cp -f build/vonoritos-x86_64.iso /mnt/c/Users/Public/vonoritos-x86_64.iso
case "$CMD" in
	iso) echo "ISO READY: /mnt/c/Users/Public/vonoritos-x86_64.iso" ;;
	test) make test64 && grep -q "KTEST ALL PASS" build/test64.log && echo "BUILD+TEST PASS" ;;
	run) ./scripts/run_gui.sh ;;
	fresh) dd if=/dev/zero of=/mnt/c/Users/Public/vonoritos-disk.img bs=1M count=16 status=none && ./scripts/run_gui.sh ;;
	*) echo "Usage: $0 {iso|test|run|fresh}"; exit 1 ;;
esac
