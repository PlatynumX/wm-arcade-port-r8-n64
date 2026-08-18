#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
OUT="${1:-$ROOT/wm-original-source-text-r8h5.zip}"
if [ ! -f "$ORIG/ATTRACT.ASM" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi
python3 "$ROOT/tools/source_text_bundle.py" --root "$ORIG" --out "$OUT"
