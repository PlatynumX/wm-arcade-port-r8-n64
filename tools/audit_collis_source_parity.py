#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import sys


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"FAIL: {label}: missing {needle!r}")
    print(f"PASS: {label}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"FAIL: {label}: forbidden {needle!r} still present")
    print(f"PASS: {label}")


def main() -> int:
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    runtime_p = root / "src/fix39/wm_fix39_runtime.c"
    header_p = root / "src/fix39/wm_fix39_runtime.h"
    cmake_p = root / "CMakeLists.txt"
    for p in (runtime_p, header_p, cmake_p):
        if not p.is_file():
            raise SystemExit(f"FAIL: missing {p}")

    runtime = runtime_p.read_text(encoding="utf-8")
    header = header_p.read_text(encoding="utf-8")
    cmake = cmake_p.read_text(encoding="utf-8")

    require(runtime, "static bool live_collision_boxes_ready_for_active(void)",
            "all-active CUR_FRAME readiness helper")
    require(runtime, "i < g.active_actor_count", "active process bounds")
    require(runtime, "j < g.active_actor_count", "all-active overlap victim scan")
    require(runtime, "if ((size_t)i == j) continue;", "self overlap exclusion")
    special_needles = [
        "&g.special_lists, g.actor_ptrs, g.active_actor_count",
        "&g.special_lists, valid_defenders, g.active_actor_count",
    ]
    special_pos = min((runtime.find(n) for n in special_needles if runtime.find(n) >= 0), default=-1)
    if special_pos < 0:
        raise SystemExit("FAIL: SPECIAL collision uses active process count")
    print("PASS: SPECIAL collision uses active process count")

    wrestler_needles = [
        "wm_arcade_check_wrestler_collisions(",
        "wm_arcade_try_attack_hit(attacker, victim,",
    ]
    wrestler_pos = min((runtime.find(n) for n in wrestler_needles if runtime.find(n) >= 0), default=-1)
    if wrestler_pos < 0:
        raise SystemExit("FAIL: wrestler collision scan missing")
    print("PASS: wrestler collision uses active process count")

    if special_pos > wrestler_pos:
        raise SystemExit("FAIL: COLLIS source order must run SPECIAL/object collisions before wrestler scan")
    print("PASS: COLLIS source master order SPECIAL-before-wrestler")

    forbid(runtime, "size_t vi = i ^ 1u;", "fixed XOR victim shortcut removed")
    forbid(runtime, "g.frame_box_valid[0] && g.frame_box_valid[1]",
           "two-slot readiness shortcut removed")

    require(header, "every active wrestler has a valid\n * current frame box",
            "public all-active collision contract")
    require(cmake, "tests/r37n4_collis_source_parity_regression.c",
            "R37N4 C regression registered")
    require(cmake, "tools/audit_collis_source_parity.py",
            "R37N4 structural audit registered")

    print("R37N4 COLLIS source-parity structural audit: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
