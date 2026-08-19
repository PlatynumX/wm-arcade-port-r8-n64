#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"

if [ ! -f "$ORIG/SELECT.ASM" ] || [ ! -f "$ORIG/PROGRESS.ASM" ] || \
   [ ! -f "$ORIG/BGNDTBL.ASM" ] || [ ! -f "$ORIG/BGNDPAL.ASM" ] || \
   [ ! -f "$ORIG/IMGPAL.ASM" ] || [ ! -f "$ORIG/LEXSEQ1.ASM" ] || \
   [ ! -d "$ORIG/IMG" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi

: "${WWFMANIA_ZIP:?WWFMANIA_ZIP must point to the user wwfmania.zip}"
[ -f "$WWFMANIA_ZIP" ] || {
    echo "ERROR: WWF arcade ROM zip not found: $WWFMANIA_ZIP" >&2
    exit 2
}

# Fix23: the historical repository contains LEXSEQ1'.ASM, an older archival
# snapshot that uses L4ST4A.  The production LEXSEQ1.ASM used by the shipped
# source uses L4ST4C for lex_stand4_anim.  The progression compiler now ignores
# apostrophe-suffixed archival .ASM files rather than chasing art that belongs
# only to the stale snapshot.
grep -q 'SUBR[[:space:]]*lex_stand4_anim' "$ORIG/LEXSEQ1.ASM" || {
    echo "ERROR: production LEXSEQ1.ASM lex_stand4_anim not found" >&2
    exit 2
}
grep -q 'L4ST4C+FR4' "$ORIG/LEXSEQ1.ASM" || {
    echo "ERROR: production LEXSEQ1.ASM does not contain expected L4ST4C stand sequence" >&2
    exit 2
}
printf '%s\n' "Fix23: production LEXSEQ1.ASM selected; archival LEXSEQ1'.ASM ignored"

python3 "$ROOT/tools/select_bundle.py" \
    --img-dir "$ORIG/IMG" \
    --imgpal "$ORIG/IMGPAL.ASM" \
    --progress "$ORIG/PROGRESS.ASM" \
    --out "$ROOT/src/generated/select_sprites.c"

python3 "$ROOT/tools/progress_wrestler_bundle.py" \
    --source-dir "$ORIG" \
    --img-dir "$ORIG/IMG" \
    --imgpal "$ORIG/IMGPAL.ASM" \
    --progress "$ORIG/PROGRESS.ASM" \
    --out "$ROOT/src/generated/progress_wrestlers.c"

python3 "$ROOT/tools/select_background_bundle.py" \
    --romzip "$WWFMANIA_ZIP" \
    --bgndtbl "$ORIG/BGNDTBL.ASM" \
    --bgndpal "$ORIG/BGNDPAL.ASM" \
    --imgpal "$ORIG/IMGPAL.ASM" \
    --out-main "$ROOT/src/generated/select_background_main.c" \
    --out-choice "$ROOT/src/generated/select_background_choice.c" \
    --out-progress "$ROOT/src/generated/progress_background.c"

python3 "$ROOT/tools/bmod_source.py" \
    --source "$ORIG/BGNDTBL.ASM" \
    --module NTITLESCBMOD \
    --module SPORTBKBMOD \
    --module wwfselbkBMOD \
    --module choiceBMOD \
    --module LADDERBMOD \
    --out "$ROOT/src/generated/bmod_tables.c"
