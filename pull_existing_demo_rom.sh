#!/data/data/com.termux/files/usr/bin/bash
set -Eeuo pipefail
RUN_ID="${1:-32531153785}"
DOWNLOAD_DIR="/sdcard/Download"
ART="${TMPDIR:-$PREFIX/tmp}/wm-fix39-artifact-$RUN_ID"
command -v gh >/dev/null 2>&1 || { echo "gh is required" >&2; exit 1; }
gh auth status >/dev/null 2>&1 || { echo "GitHub CLI is not authenticated. Run: gh auth login" >&2; exit 1; }
rm -rf "$ART"
mkdir -p "$ART" "$DOWNLOAD_DIR"
echo "Pulling wm-arcade-r9-build from GitHub run $RUN_ID..."
gh run download "$RUN_ID" -R PlatynumX/wm-arcade-port-r8-n64 --name wm-arcade-r9-build --dir "$ART"
ROM="$(find "$ART" -type f -name 'wm_arcade_r9.z64' -print -quit)"
[ -n "$ROM" ] && [ -s "$ROM" ] || { echo "Artifact downloaded but wm_arcade_r9.z64 was not found" >&2; exit 1; }
OUT="$DOWNLOAD_DIR/wm_arcade_fix39_attract_demo_run_${RUN_ID}.z64"
cp "$ROM" "$OUT"
echo "ROM: $OUT"
