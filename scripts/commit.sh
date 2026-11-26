#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -lt 2 ]; then echo "Usage: $0 \"message\" file..."; exit 1; fi
MSG="$1"; shift
git add -- "$@"
git commit -m "$MSG"
git push
