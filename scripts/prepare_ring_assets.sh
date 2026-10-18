#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
SRC="$ROOT/original/wwf-wrestlemania"

if [ ! -d "$SRC/IMG" ]; then
    "$ROOT/scripts/fetch_original.sh"
fi

# MAIN.LOD names NEWRINGB as the shipped wrestling-ring BLIMP bundle.
grep -Eiq 'BBB>[[:space:]]+NEWRINGB' "$SRC/IMG/MAIN.LOD"

python3 "$ROOT/tools/stream_ring_assets.py" \
    --bdd "$SRC/IMG/NEWRINGB.BDD" \
    --bdb "$SRC/IMG/NEWRINGB.BDB" \
    --bgndtbl "$SRC/BGNDTBL.ASM" \
    --region ring \
    --module ringBMOD \
    --out-fs "$ROOT/filesystem/ring"
