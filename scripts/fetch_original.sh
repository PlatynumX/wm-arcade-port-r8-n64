#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST="$ROOT/original/wwf-wrestlemania"
if [ -d "$DEST/.git" ]; then
  git -C "$DEST" pull --ff-only
else
  rm -rf "$DEST"
  git clone --depth 1 https://github.com/historicalsource/wwf-wrestlemania.git "$DEST"
fi
printf 'Original source available at %s\n' "$DEST"
