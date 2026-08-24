#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/src/fix39"
CC_BIN="${CC:-cc}"
TMP="${TMPDIR:-/tmp}"
OUT="$TMP/wm_fix39_smoke"
NS="$TMP/wm_fix39_namespace_probe.c"
NSO="$TMP/wm_fix39_namespace_probe.o"

"$CC_BIN" -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$SRC" "$SRC"/*.c "$ROOT/tests/fix39_smoke.c" -o "$OUT"
"$OUT"

# Regression for the exact v4 failure: app.c includes the frontend enums first,
# then wm_fix39_runtime.h.  The direct-port constants must occupy a separate C
# enumerator namespace even though their numeric values remain source-exact.
cat > "$NS" <<'PROBE'
#include <stdint.h>
enum {
    WM_MODE_END=0x0001, WM_MODE_INTURN=0x0002, WM_MODE_UNINT=0x0004,
    WM_MODE_NOAUTOFLIP=0x0008, WM_MODE_CHECKHIT=0x0010,
    WM_MODE_NOGRAVITY=0x0020, WM_MODE_FRICTION=0x0040,
    WM_MODE_NOCONFINE=0x0080, WM_MODE_NOCOLLIS=0x0100,
    WM_MODE_STATUS=0x0200, WM_MODE_OVERLAP=0x0400, WM_MODE_GHOST=0x0800,
    WM_MODE_NOSHADOW=0x1000, WM_MODE_KEEPATTACHED=0x2000,
    WM_MODE_WAITHITOPP=0x4000, WM_MODE_INVISIBLE=0x8000
};
enum { WM_ATTRACT_SHOW_HSTD=0, WM_ATTRACT_DCS_LOGO=1 };
/* Regression for the V8 N64 failure: include/wm/ropes.h already owns these
 * global C enumerator names.  The direct-port rope API must be namespaced. */
enum {
    WM_ROPE_FRONT=0, WM_ROPE_BACK=1, WM_ROPE_LEFT=2, WM_ROPE_RIGHT=3,
    WM_ROPE_TOP=0, WM_ROPE_MIDDLE=1, WM_ROPE_BOTTOM=2,
    WM_ROPE_Z_HIGH=0, WM_ROPE_Z_NORM=1,
    WM_ROPE_BOUNCE_UD=0, WM_ROPE_BOUNCE_IO=1, WM_ROPE_SIDE_SPRING=2,
    WM_ROPE_DOWN_SPRING=3, WM_ROPE_SIDE_SPRING_RELEASE=4,
    WM_ROPE_DOWN_SPRING_RELEASE=5, WM_ROPE_COMMAND_COUNT=6,
    WM_ROPE_CHANNEL_RED=0, WM_ROPE_CHANNEL_WHITE=1,
    WM_ROPE_CHANNEL_BLUE=2, WM_ROPE_CHANNEL_SHADOW=3,
    WM_ROPE_CHANNEL_COUNT=4, WM_ROPE_HALF_FIRST=0, WM_ROPE_HALF_SECOND=1
};
#include "wm_fix39_runtime.h"
#include "wmania_rope_runtime.h"
_Static_assert(WM_ARCADE_MODE_INVISIBLE == 0x8000, "ANIM.EQU value drift");
_Static_assert(WM_FIX39_ATTRACT_DCS_LOGO == 1, "attract sequence value drift");
_Static_assert(WM_FIX39_ROPE_COMMAND_COUNT == 6, "ROPES.ASM command value drift");
_Static_assert(WM_FIX39_ROPE_CHANNEL_COUNT == 4, "ROPES.ASM channel value drift");
int main(void) { return 0; }
PROBE
"$CC_BIN" -std=c11 -Wall -Wextra -Wpedantic -Werror -I"$SRC" -c "$NS" -o "$NSO"
echo "Fix39 namespace collision regression: PASS"
