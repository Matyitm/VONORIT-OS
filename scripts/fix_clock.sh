#!/usr/bin/env bash
set -euo pipefail
WIN="$(powershell.exe -NoProfile -Command "[DateTime]::UtcNow.ToString('yyyy-MM-dd HH:mm:ss')" | tr -d '\r')"
echo "Windows UTC: $WIN"
echo "WSL UTC:     $(date -u '+%Y-%m-%d %H:%M:%S')"
sudo date -u -s "$WIN" > /dev/null
echo "Synced:      $(date -u '+%Y-%m-%d %H:%M:%S')"
