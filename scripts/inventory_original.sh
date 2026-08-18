#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
if [ ! -f "$ORIG/BRET.ASM" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi
mkdir -p "$ROOT/build"
python3 "$ROOT/tools/source_inventory.py" \
    --root "$ORIG" \
    --json "$ROOT/build/source_inventory.json" \
    --md "$ROOT/build/SOURCE_INVENTORY.md" \
    --strict-roster
cat "$ROOT/build/SOURCE_INVENTORY.md"
