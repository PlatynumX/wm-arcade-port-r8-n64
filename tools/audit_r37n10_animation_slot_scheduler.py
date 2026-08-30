#!/usr/bin/env python3
"""Structural audit for R37N10 ANIM primary/secondary slot scheduler parity."""
from __future__ import annotations
import pathlib
import sys


def need(text: str, token: str, label: str) -> None:
    if token not in text:
        raise AssertionError(f"missing {label}: {token}")


def forbid(text: str, token: str, label: str) -> None:
    if token in text:
        raise AssertionError(f"legacy {label} still present: {token}")


def main() -> int:
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    c = (root / "src/fix39/wm_arcade_source_animation_runtime.c").read_text(encoding="utf-8")
    h = (root / "src/fix39/wm_arcade_source_animation_runtime.h").read_text(encoding="utf-8")
    live = (root / "src/fix39/wm_fix39_runtime.c").read_text(encoding="utf-8")
    cm = (root / "CMakeLists.txt").read_text(encoding="utf-8")

    for token, label in (
        ("static uint16_t source_slot_mode", "slot mode helper"),
        ("static int32_t source_slot_count", "slot count helper"),
        ("source_slot_set_mode(s,a,(uint16_t)av(i,0))", "ANI_SETMODE current-slot write"),
        ("source_slot_set_count(s,a,(int32_t)av(i,0))", "ANI_PAUSE current-slot count"),
        ("source_slot_mode(s,a)|WM_ARCADE_MODE_END", "ANI_END current-slot write"),
        ("source_slot_set_count(s,a,(int32_t)(((int64_t)av(i,0)", "ANI_SUPERSLAVE2 slot count"),
        ("source_slot_set_count(s,a,v);return v!=0", "ANI_HMBWAIT slot count"),
        ("source_slot_set_count(s,a,(int32_t)ticks)", "exact frame count write"),
        ("cnt=source_slot_count(s,a)-1", "unconditional source slot countdown"),
        ("wm_source_anim_runtime_tick_impl(s,a);", "no-swap tick"),
        ("source_anim_restart", "restart implementation"),
        ("CUR_FRAME is", "CUR_FRAME preservation note"),
    ):
        need(c, token, label)

    for token, label in (
        ("primary_mode=a->anim_mode", "secondary actor-mode swap"),
        ("primary_count=a->ani_count", "secondary actor-count swap"),
        ("a->anim_mode=s->mode_shadow", "secondary mode shadow copy into primary"),
        ("a->ani_count=s->count_shadow", "secondary count shadow copy into primary"),
        ("(ticks?ticks:1u)", "zero-tick coercion"),
        ("case 73: a->anim_mode|=WM_ARCADE_MODE_END", "primary-only ANI_END"),
        ("#define HOLD() do{a->ani_count=1", "primary-only HOLD"),
    ):
        forbid(c, token, label)

    for token, label in (
        ("wm_source_anim_runtime_change_and_prime", "conditional change/prime API"),
        ("wm_source_anim_runtime_restart_and_prime", "force restart/prime API"),
        ("wm_source_anim_runtime_slot_mode", "slot mode accessor"),
        ("wm_source_anim_runtime_slot_count", "slot count accessor"),
    ):
        need(h, token, label)

    need(live,
         "return wm_source_anim_runtime_restart_and_prime(&g.source_anim[i],o,(uint8_t)o->wrestler_num,label);",
         "ANI_SLAVEANIM change_anim1a behavior")
    need(live,
         "return wm_source_anim_runtime_change_and_prime(rt,a,(uint8_t)a->wrestler_num,label);",
         "live change_anim1/2 conditional prime")
    need(live,
         "wm_source_anim_runtime_change_and_prime(&g.source_anim[i],a,(uint8_t)a->wrestler_num,label)",
         "native primary conditional prime")
    forbid(live,
           "if(wm_source_anim_runtime_change(&g.source_anim[i],a,(uint8_t)a->wrestler_num,label))wm_source_anim_runtime_tick",
           "same-label native extra-tick pair")

    for token, label in (
        ("wm_r37n10_animation_slot_scheduler_regression_tests", "C regression"),
        ("wm_r37n10_animation_slot_scheduler_audit", "structural audit"),
        ("wm_r37n10_animation_slot_scheduler_model", "model test"),
    ):
        need(cm, token, label)

    print("R37N10 animation-slot scheduler structural audit: PASS")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
