#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="${TMPDIR:-/tmp}/wm-v13e-c2-range-$$"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP"

cat > "$TMP/DRONE.ASM" <<'SRC'
PAIR .macro me,him,lst
.byte :me:,:him:
.long :lst:
.endm
ROOT9 .macro a,b,c,d,e,f,g,h,i
.long :a:,:b:,:c:,:d:,:e:,:f:,:g:,:h:,:i:
.endm
#wnshort_t
ROOT9 s0,s1,s2,s3,s4,s5,s6,s6,s8
#wnmed_t
ROOT9 m0,m1,m2,m3,m4,m5,m6,m6,m8
#wnlong_t
ROOT9 l0,l1,l2,l3,l4,l5,l6,l6,l8
SRC
for p in s m l; do
  for n in 0 1 2 3 4 5 6 8; do
    cat >> "$TMP/DRONE.ASM" <<SRC
#$p$n
PAIR MODE_NORMAL,MODE_BLOCK,list_specific
WL -1,list_default
SRC
  done
done
cat >> "$TMP/DRONE.ASM" <<'SRC'
#list_specific
.word (list_specific_n-$)/32-1
.long attack_a,attack_b
#list_specific_n
#list_default
.word 0fffeh
.long fallback_a,fallback_b,fallback_c
SRC

python "$ROOT/tools/fix39_drone_ranges.py" \
  --source "$TMP/DRONE.ASM" \
  --out "$TMP/wm_arcade_drone_source_ranges_generated.h"
cp "$ROOT/src/fix39/wm_arcade_drone_source_ranges.c" "$TMP/"
cp "$ROOT/src/fix39/wm_arcade_drone_source_ranges.h" "$TMP/"
cat > "$TMP/test.c" <<'C'
#include <assert.h>
#include <string.h>
#include "wm_arcade_drone_source_ranges.h"

int main(void)
{
    wm_arcade_actor_t a = {0};
    const wm_arcade_drone_script_list_t *list;
    a.wrestler_num = 0;
    assert(wm_arcade_drone_source_ranges_ready());
    list = wm_arcade_drone_source_range_script_list(
        &a, 0, 0, WM_PMODE_NORMAL, WM_PMODE_BLOCK, 0);
    assert(list && list->source_max_index == 1 && list->script_count == 2u);
    assert(strcmp(list->scripts[0], "attack_a") == 0);
    list = wm_arcade_drone_source_range_script_list(
        &a, 0, 2, WM_PMODE_RUNNING, WM_PMODE_DEAD, 0);
    assert(list && list->source_max_index == -2 && list->script_count == 3u);
    assert(strcmp(list->scripts[2], "fallback_c") == 0);
    /* Source internal slot 7 is the unused Adam Bomb slot and aliases the
     * Doink DRONE table; Lex is source wrestler index 8. */
    a.wrestler_num = 7;
    list = wm_arcade_drone_source_range_script_list(
        &a, 0, 0, WM_PMODE_NORMAL, WM_PMODE_BLOCK, 0);
    assert(list && strcmp(list->scripts[0], "attack_a") == 0);
    a.wrestler_num = 8;
    list = wm_arcade_drone_source_range_script_list(
        &a, 0, 0, WM_PMODE_NORMAL, WM_PMODE_BLOCK, 0);
    assert(list && strcmp(list->scripts[1], "attack_b") == 0);
    a.wrestler_num = 9;
    assert(wm_arcade_drone_source_range_script_list(&a, 0, 0, 0, 0, 0) == 0);
    return 0;
}
C
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$TMP" -I"$ROOT/src/fix39" \
  "$TMP/wm_arcade_drone_source_ranges.c" "$TMP/test.c" -o "$TMP/test"
"$TMP/test"
echo "Fix39 V13e chunk-2 source range resolver smoke: PASS"
