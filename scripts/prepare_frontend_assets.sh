#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
SPORTS_SOURCE="$ORIG/IMG/SPORTLO8.IMG"
# FIX39 SPORTS FOREGROUND OVERRIDE
FIX39_SPORTS_SOURCE="$ROOT/assets/fix39_sports_override/SPORTLO8.IMG"
if [ -f "$FIX39_SPORTS_SOURCE" ]; then
  SPORTS_SOURCE="$FIX39_SPORTS_SOURCE"
fi
DCS_SOURCE="$ORIG/IMG/DCSLOGO.IMG"
SPARKLE_SOURCE="$ORIG/IMG/SPARKLE.IMG"
SPORTS_OUT="$ROOT/src/generated/sports_logo.c"
DCS_OUT="$ROOT/src/generated/dcs_logo.c"
SPARKLE_OUT="$ROOT/src/generated/title_sparkle.c"
TITLE_BDB="$ORIG/IMG/BIGWWF.BDB"
TITLE_BDD="$ORIG/IMG/BIGWWF.BDD"
TITLE_OUT="$ROOT/src/generated/title_screen.c"
BMOD_OUT="$ROOT/src/generated/bmod_tables.c"
SELECT_OUT="$ROOT/src/generated/select_sprites.c"

if [ ! -f "$SPORTS_SOURCE" ] || [ ! -f "$DCS_SOURCE" ] || \
   [ ! -f "$SPARKLE_SOURCE" ] || [ ! -f "$TITLE_BDB" ] || [ ! -f "$TITLE_BDD" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi

# Be a Man replacement owns sports_logo.c; DCS/title regeneration must not
# overwrite the checked-in 17-piece replacement payload.
if [ ! -s "$SPORTS_OUT" ]; then
    echo "missing checked-in replacement: $SPORTS_OUT" >&2
    exit 2
fi

python3 "$ROOT/tools/dcs_bundle.py" \
    --source "$DCS_SOURCE" \
    --out "$DCS_OUT"

python3 "$ROOT/tools/sparkle_bundle.py" \
    --source "$SPARKLE_SOURCE" \
    --out "$SPARKLE_OUT"

python3 "$ROOT/tools/bdd_bundle.py" \
    --bdd "$TITLE_BDD" \
    --bdb "$TITLE_BDB" \
    --bgndtbl "$ORIG/BGNDTBL.ASM" \
    --region NTITLESC \
    --module NTITLESCBMOD \
    --out "$TITLE_OUT"

python3 "$ROOT/tools/bmod_source.py" \
    --source "$ORIG/BGNDTBL.ASM" \
    --module NTITLESCBMOD \
    --module SPORTBKBMOD \
    --module LADDERBMOD \
    --module slateBMOD \
    --module choiceBMOD \
    --module wwfselbkBMOD \
    --out "$BMOD_OUT"


python3 "$ROOT/tools/select_bundle.py" \
    --img-dir "$ORIG/IMG" \
    --imgpal "$ORIG/IMGPAL.ASM" \
    --progress "$ORIG/PROGRESS.ASM" \
    --out "$SELECT_OUT"

# FIX39 SPORTS BACKGROUND REGEN
if [ -f "$ROOT/assets/fix39_sports_override/SPORTBK.IMG" ]; then
  sh "$ROOT/scripts/prepare_sports_source_assets.sh"
fi
