#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import sys

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
combat = (root / "src/fix39/wm_arcade_combat.c").read_text(encoding="utf-8")
attach = (root / "src/fix39/wm_arcade_attach_anim.c").read_text(encoding="utf-8")
anim = (root / "src/fix39/wm_arcade_source_animation_runtime.c").read_text(encoding="utf-8")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")

def case_body(text: str, opcode: int) -> str:
    start = f"case {opcode}:"
    end = f"case {opcode + 1}:"
    if start not in text:
        return ""
    body = text.split(start, 1)[1]
    if end in body:
        body = body.split(end, 1)[0]
    return body

case21 = case_body(anim, 21)
case82 = case_body(anim, 82)
case125 = case_body(anim, 125)

checks = {
    "collision emulates OBJ_XPOSINT word writes": "source_write_xposint(mover" in combat,
    "collision emulates OBJ_ZPOSINT word writes": "source_write_zposint(mover" in combat,
    "collision WORD write preserves fixed fractional low half": "fixed & 0xffffu" in combat,
    "master attachment re-syncs fixed/int aliases": "source_sync_position_aliases(opp);" in attach,
    "slave attachment re-syncs fixed/int aliases": "source_sync_position_aliases(a);" in attach,
    "animation VM has source WORD alias helpers": "source_posword_into_fixed" in anim and "source_write_yposint" in anim,
    "opcode 21 _ani_offset preserves fractions": "case 21:" in anim and "source_write_xposint(a,a->x_int+v)" in anim and "source_write_zposint(a,a->z_int+av(i,2))" in anim,
    "opcode 82 _ani_oppoffset preserves fractions": "case 82:" in anim and "source_write_xposint(o,o->x_int+v)" in anim and "source_write_yposint(o,o->y_int+t->entries[idx+1].value)" in anim,
    "opcode 125 _ani_ground preserves Y fraction": "case 125: source_write_yposint(a,a->ground_y);NEXT();" in anim,
    "old opcode 21 zero-fraction rewrite removed": "a->x_fixed=a->x_int<<16" not in case21,
    "old opcode 82 zero-fraction rewrite removed": "o->x_fixed=o->x_int<<16" not in case82,
    "old opcode 125 zero-fraction rewrite removed": "a->y_fixed=a->y_int<<16" not in case125,
    "regression executable is registered": "wm_r37n9_position_alias_regression_tests" in cmake,
    "structural audit is registered": "wm_r37n9_position_alias_audit" in cmake,
    "position alias model is registered": "wm_r37n9_position_alias_model" in cmake,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)
if failed:
    raise SystemExit(1)
print("R37N9 position-alias structural audit: PASS")
