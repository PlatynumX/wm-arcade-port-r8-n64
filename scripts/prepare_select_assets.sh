#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"

if [ ! -f "$ORIG/SELECT.ASM" ] || [ ! -f "$ORIG/BGNDTBL.ASM" ] || \
   [ ! -f "$ORIG/BGNDPAL.ASM" ] || [ ! -f "$ORIG/IMGPAL.ASM" ] || \
   [ ! -d "$ORIG/IMG" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi

: "${WWFMANIA_ZIP:?WWFMANIA_ZIP must point to the user's wwfmania.zip}"
[ -f "$WWFMANIA_ZIP" ] || {
    echo "ERROR: WWF arcade ROM zip not found: $WWFMANIA_ZIP" >&2
    exit 2
}

python3 "$ROOT/tools/select_bundle.py" \
    --img-dir "$ORIG/IMG" \
    --out "$ROOT/src/generated/select_sprites.c"

python3 "$ROOT/tools/select_background_bundle.py" \
    --romzip "$WWFMANIA_ZIP" \
    --bgndtbl "$ORIG/BGNDTBL.ASM" \
    --bgndpal "$ORIG/BGNDPAL.ASM" \
    --imgpal "$ORIG/IMGPAL.ASM" \
    --out-main "$ROOT/src/generated/select_background_main.c" \
    --out-choice "$ROOT/src/generated/select_background_choice.c"

python3 "$ROOT/tools/bmod_source.py" \
    --source "$ORIG/BGNDTBL.ASM" \
    --module NTITLESCBMOD \
    --module SPORTBKBMOD \
    --module wwfselbkBMOD \
    --module choiceBMOD \
    --out "$ROOT/src/generated/bmod_tables.c"
