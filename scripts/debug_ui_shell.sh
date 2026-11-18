#!/usr/bin/env bash
set -euo pipefail

echo "=== [1/3] Parancsok ellenorzese az input.c-ben ==="
grep -n "lspci\|netinfo\|winver\|diskinfo\|sync" src/input.c || true

echo "=== [2/3] Terminal gorgetes (utolso 14 sor) javitasa az ui.c-ben ==="
python3 - << 'PY'
p = "src/ui.c"
s = open(p).read()

helper = """static const char* terminal_visible_tail(const char* text)
{
\tint lines = 0;
\tint i = 0;
\twhile (text[i] != 0) { if (text[i] == '\\n') ++lines; ++i; }
\tif (lines <= 13) return text;
\tint skip = lines - 13;
\ti = 0;
\twhile (text[i] != 0 && skip > 0) { if (text[i] == '\\n') --skip; ++i; }
\treturn text + i;
}
"""

if "terminal_visible_tail" not in s:
    s = s.replace("static void ui_draw_window_state(", helper + "\nstatic void ui_draw_window_state(", 1)
    s = s.replace("(window==&ui_windows[UI_WINDOW_TERMINAL])?terminal_text:window->body;",
                  "(window==&ui_windows[UI_WINDOW_TERMINAL])?terminal_visible_tail(terminal_text):window->body;", 1)
    open(p, "w").write(s)
    print("Terminal gorgetes beepitve!")
else:
    print("Terminal gorgetes mar benne van.")
PY

echo "=== [3/3] Tiszta forditas es QEMU inditas ==="
make clean && make iso64
cp -f build/vonoritos-x86_64.iso /mnt/c/Users/Public/vonoritos-x86_64.iso
"/mnt/c/Program Files/qemu/qemu-system-x86_64.exe" -m 512M -serial stdio -no-reboot -cdrom "C:\Users\Public\vonoritos-x86_64.iso" -drive file="C:\Users\Public\vonoritos-disk.img",format=raw,if=ide -netdev user,id=n0 -device e1000,netdev=n0,mac=52:54:00:12:34:56
