#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
SPORTS_SOURCE="$ORIG/IMG/SPORTLO8.IMG"
DCS_SOURCE="$ORIG/IMG/DCSLOGO.IMG"
SPORTS_OUT="$ROOT/src/generated/sports_logo.c"
DCS_OUT="$ROOT/src/generated/dcs_logo.c"
TITLE_OUT="$ROOT/src/generated/title_screen.c"
BMOD_OUT="$ROOT/src/generated/bmod_tables.c"

if [ ! -f "$SPORTS_SOURCE" ] || [ ! -f "$DCS_SOURCE" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi

python3 "$ROOT/tools/frontend_bundle.py" \
    --source "$SPORTS_SOURCE" \
    --out "$SPORTS_OUT"

python3 "$ROOT/tools/dcs_bundle.py" \
    --source "$DCS_SOURCE" \
    --out "$DCS_OUT"

python3 "$ROOT/tools/background_crop.py" \
    --img-dir "$ORIG/IMG" \
    --bgndtbl "$ORIG/BGNDTBL.ASM" \
    --region NTITLESC \
    --module NTITLESCBMOD \
    --out "$TITLE_OUT"

python3 "$ROOT/tools/bmod_source.py" \
    --source "$ORIG/BGNDTBL.ASM" \
    --module NTITLESCBMOD \
    --module SPORTBKBMOD \
    --out "$BMOD_OUT"
