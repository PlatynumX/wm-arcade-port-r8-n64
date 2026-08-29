#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST="$ROOT/original/wwf-wrestlemania"
URL="https://github.com/historicalsource/wwf-wrestlemania.git"
PIN="1280555b4d041dd025198c8e85ed14b4c1c91cfb"

if [ ! -d "$DEST/.git" ]; then
    rm -rf "$DEST"
    git clone "$URL" "$DEST"
fi

git -C "$DEST" fetch origin "$PIN" --depth=1 2>/dev/null || git -C "$DEST" fetch origin --depth=1

git -C "$DEST" checkout --detach "$PIN"

ACTUAL=$(git -C "$DEST" rev-parse HEAD)
test "$ACTUAL" = "$PIN" || {
    echo "ERROR: historical source pin mismatch: $ACTUAL" >&2
    exit 1
}

printf 'Original source pinned at %s (%s)\n' "$DEST" "$PIN"
