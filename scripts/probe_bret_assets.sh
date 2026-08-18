#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
if [ ! -f "$ORIG/IMG/BRET.LOD" ] || [ ! -f "$ORIG/IMG/HRT_WLK.IMG" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi
python3 "$ROOT/tools/bret_manifest.py" --lod "$ORIG/IMG/BRET.LOD" \
    --require H4ST4A01 --require H4ST4A08 --require H2WL1A01 --require H2WL1A16
python3 "$ROOT/tools/wimpimg.py" probe "$ORIG/IMG/HRT_WLK.IMG" \
    --require H4ST4A01 --require H4ST4A08 --require H2WL1A01 --require H2WL1A16
