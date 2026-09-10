#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
SPORTS_SOURCE="$ORIG/IMG/SPORTLO8.IMG"
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

python3 "$ROOT/tools/frontend_bundle.py" \
    --source "$SPORTS_SOURCE" \
    --out "$SPORTS_OUT"

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

# NOTE: src/generated/bmod_tables.c has TWO producers -- this script and
# scripts/prepare_select_assets.sh -- and bmod_source.py numbers its
# bmod_words_N arrays by the order the modules arrive in. The two lists
# below and there must therefore stay IDENTICAL, or whichever script ran
# last silently renumbers the file and the checked-in copy stops
# reproducing. They disagreed (LADDER and wwfselbk were swapped) until
# this comment was written; a source-tool test now holds them together.
python3 "$ROOT/tools/bmod_source.py" \
    --source "$ORIG/BGNDTBL.ASM" \
    --module NTITLESCBMOD \
    --module SPORTBKBMOD \
    --module wwfselbkBMOD \
    --module choiceBMOD \
    --module LADDERBMOD \
    --out "$BMOD_OUT"


python3 "$ROOT/tools/select_bundle.py" \
    --img-dir "$ORIG/IMG" \
    --imgpal "$ORIG/IMGPAL.ASM" \
    --progress "$ORIG/PROGRESS.ASM" \
    --out "$SELECT_OUT"
