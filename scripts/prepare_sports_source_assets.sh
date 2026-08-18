#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
SPORTS_BG_SOURCE="$ORIG/IMG/SPORTBK.IMG"
SPORTS_FONT_SOURCE="$ORIG/IMG/SGMD8.IMG"
SPORTS_BG_OUT="$ROOT/src/generated/sports_background.c"
SPORTS_MOTTO_OUT="$ROOT/src/generated/sports_motto.c"

# This is intentionally separate from prepare_frontend_assets.sh. The source
# port should not depend on that script's exact formatting or gate expression.
if [ ! -f "$SPORTS_BG_SOURCE" ] || [ ! -f "$SPORTS_FONT_SOURCE" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi

test -f "$SPORTS_BG_SOURCE"
test -f "$SPORTS_FONT_SOURCE"

python3 "$ROOT/tools/sports_background_bundle.py" \
    --source "$SPORTS_BG_SOURCE" \
    --out "$SPORTS_BG_OUT"

python3 "$ROOT/tools/sports_motto_bundle.py" \
    --source "$SPORTS_FONT_SOURCE" \
    --out "$SPORTS_MOTTO_OUT"

test -s "$SPORTS_BG_OUT"
test -s "$SPORTS_MOTTO_OUT"
