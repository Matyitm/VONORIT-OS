#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then echo "Usage: $0 vX.Y.Z \"message\""; exit 1; fi
[ -z "$(git status --porcelain)" ] || { echo "Working tree not clean"; exit 1; }
bash scripts/build.sh test
git tag -a "$1" -m "$2"
git push origin "$1"
