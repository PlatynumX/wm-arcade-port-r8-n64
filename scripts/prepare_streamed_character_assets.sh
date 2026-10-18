#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"

if [ ! -f "$ORIG/IMG/BRET.LOD" ] || [ ! -f "$ORIG/IMG/LEX.LOD" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi
if [ ! -s "$ROOT/src/generated/frame_geometry.c" ]; then
    sh "$ROOT/scripts/regenerate_source_data.sh"
fi

python3 "$ROOT/tools/stream_roster_assets.py" \
    --geometry "$ROOT/src/generated/frame_geometry.c" \
    --img-dir "$ORIG/IMG" \
    --out-fs "$ROOT/filesystem/wrestlers"

test -s "$ROOT/filesystem/wrestlers/STREAMED_ROSTER.txt"
