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

python3 "$ROOT/tools/bmod_source.py" \
    --source "$ORIG/BGNDTBL.ASM" \
    --module NTITLESCBMOD \
    --module SPORTBKBMOD \
    --out "$BMOD_OUT"
