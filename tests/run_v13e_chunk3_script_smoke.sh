#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="${TMPDIR:-/tmp}/wm-v13e-c3-script-$$"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP"

cat > "$TMP/DRONE.ASM" <<'SRC'
.file "drone.asm"
U_M .equ 20h
P_M .equ 1
#wnshort_t
.long m0,m1,m2,m3,m4,m5,m6,m7,m8
#wnmed_t
.long m0,m1,m2,m3,m4,m5,m6,m7,m8
#wnlong_t
.long m0,m1,m2,m3,m4,m5,m6,m7,m8
#m0
WL -1,list0
#m1
WL -1,list0
#m2
WL -1,list0
#m3
WL -1,list0
#m4
WL -1,list0
#m5
WL -1,list0
#m6
WL -1,list0
#m7
WL -1,list0
#m8
WL -1,list0
#list0
.word 0
.long ucu
#M_shrtblkr
.word 0
.long hgrab
#M_shrtblkrdl
.word 0
.long hgrabdl
#slhtoss
#drn_enterring
#drn_opinair
#drn_oprun
#drn_roll
#drn_inair
#drn_ontb
#drn_run
#drn_combo
#drn_seekclose
#drn_oppdead
#hgrab
#hgrabdl
#UCU
.word P_M+U_M,2
.word 0c000h
.word 08002h
.long sktab
.word 0,-1
#sktab
.word 0,1,2,3,4,5,6,7,8,9
.word 10,11,12,13,14,15,16,17,18,19
.word 20,21,22,23,24,25,26,27,28,29
SRC

python "$ROOT/tools/fix39_drone_scripts.py" \
  --source "$TMP/DRONE.ASM" \
  --out "$TMP/wm_arcade_drone_source_scripts_generated.h"
cp "$ROOT/src/fix39/wm_arcade_drone_source_scripts.c" "$TMP/"
cp "$ROOT/src/fix39/wm_arcade_drone_source_scripts.h" "$TMP/"
cat > "$TMP/resolver_test.c" <<'C'
#include <assert.h>
#include <string.h>
#include "wm_arcade_drone_source_scripts.h"
int main(void)
{
    const wm_arcade_drone_script_t *s;
    assert(wm_arcade_drone_source_scripts_ready());
    s = wm_arcade_drone_source_resolve_script("ucu", 0);
    assert(s && s->op_count >= 3u);
    assert(s->ops[0].opcode == WM_DRONE_SC_INPUT);
    assert(s->ops[0].input_word == 0x21u);
    assert(s->ops[0].delay == 2);
    assert(s->ops[1].opcode == WM_DRONE_SC_DONE);
    assert(s->ops[2].opcode == WM_DRONE_SC_SKILL_ABORT);
    assert(strcmp(s->ops[2].source_label, "sktab") == 0);
    assert(wm_arcade_drone_source_script_skill_pct("sktab", 0, 0) == 0);
    assert(wm_arcade_drone_source_script_skill_pct("sktab", 29, 0) == 29);
    return 0;
}
C
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$TMP" -I"$ROOT/src/fix39" \
  "$TMP/wm_arcade_drone_source_scripts.c" "$TMP/resolver_test.c" -o "$TMP/resolver_test"
"$TMP/resolver_test"

cat > "$TMP/vm_test.c" <<'C'
#include <assert.h>
#include <string.h>
#include "wm_arcade_drone.h"
static int calls;
static int call_cb(wm_arcade_actor_t *a, wm_arcade_actor_t *o, wm_arcade_drone_state_t *d, const char *label, void *u)
{ (void)a; (void)o; (void)d; (void)label; (void)u; ++calls; return 1; }
int main(void)
{
    wm_arcade_actor_t a, o;
    wm_arcade_drone_state_t d;
    wm_arcade_drone_callbacks_t cb;
    static const wm_arcade_drone_script_op_t ops[] = {
        { WM_DRONE_SC_DONE, 0, 0, 0, 0, 0 },
        { WM_DRONE_SC_CALL_CODE, 0, 0, 0, 0, "code0" },
        { WM_DRONE_SC_INPUT, 1, 1, 0, 0, 0 }
    };
    static const wm_arcade_drone_script_t sc = { "t", ops, 3u };
    memset(&a,0,sizeof(a)); memset(&o,0,sizeof(o)); memset(&cb,0,sizeof(cb));
    wm_arcade_drone_init(&d, 10); d.script="t"; d.script_mode=WM_PMODE_NORMAL;
    assert(wm_arcade_drone_script_step(&a,&o,&d,&sc,&cb) == WM_DRONE_STEP_SCRIPT);
    assert(d.script && d.script_pc == 1u); /* #dsdone yields, does not abort */
    assert(wm_arcade_drone_script_step(&a,&o,&d,&sc,&cb) == WM_DRONE_STEP_SCRIPT);
    assert(d.script_pc == 1u && calls == 0); /* missing C4 service fails closed */
    cb.script_call=call_cb;
    assert(wm_arcade_drone_script_step(&a,&o,&d,&sc,&cb) == WM_DRONE_STEP_INPUT);
    assert(calls == 1 && d.script_pc == 3u);
    return 0;
}
C
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$ROOT/src/fix39" \
  "$ROOT/src/fix39/wm_arcade_drone.c" \
  "$ROOT/src/fix39/wm_arcade_drone_source_tables.c" \
  "$ROOT/src/fix39/wm_arcade_drone_source_bodies.c" \
  "$ROOT/src/fix39/wm_arcade_drone_source_services.c" \
  "$ROOT/src/fix39/wm_arcade_drone_source_scripts.c" \
  "$ROOT/src/fix39/wmania_rng.c" \
  "$TMP/vm_test.c" -o "$TMP/vm_test"
"$TMP/vm_test"
echo "Fix39 V13e chunk-3 source scripts + VM semantics smoke: PASS"
