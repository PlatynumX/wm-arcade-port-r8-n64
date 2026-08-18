#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
LOD="$ORIG/IMG/BRET.LOD"
OUT="$ROOT/src/generated/bret_sprites.c"
if [ ! -f "$LOD" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi
# Keep generated visual tables synchronized before discovering their artwork.
sh "$ROOT/scripts/regenerate_source_data.sh"
python3 "$ROOT/tools/bret_bundle.py" \
    --lod "$LOD" \
    --img-dir "$ORIG/IMG" \
    --visual-source "$ROOT/src/generated/bret_visuals.c" \
    --visual-source "$ROOT/src/generated/bret_attacks.c" \
    --out "$OUT"
