#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
if [ ! -f "$ORIG/ATTRACT.ASM" ]; then sh "$ROOT/scripts/fetch_original.sh"; fi
mkdir -p "$ROOT/build"
python3 "$ROOT/tools/animation_ir.py" \
  --root "$ORIG" \
  --json "$ROOT/build/animation_ir.json" \
  --md "$ROOT/build/ANIMATION_FRONTIER.md"
cat "$ROOT/build/ANIMATION_FRONTIER.md"
