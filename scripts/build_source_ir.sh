#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
if [ ! -f "$ORIG/ATTRACT.ASM" ]; then sh "$ROOT/scripts/fetch_original.sh"; fi
mkdir -p "$ROOT/build"
python3 "$ROOT/tools/source_ir.py" \
  --root "$ORIG" \
  --json "$ROOT/build/source_ir.json" \
  --md "$ROOT/build/SOURCE_FRONTIER.md" \
  --root-routine attract_mode \
  --root-routine start_match
cat "$ROOT/build/SOURCE_FRONTIER.md"
