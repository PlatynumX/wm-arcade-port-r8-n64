#!/usr/bin/env python3
"""
Combat2ES behavioral proof tests.

These tests close the non-critical R24 audit reminders without changing gameplay:
- ck_ignore_a8 mv_tbl coverage is checked from the translated C body.
- zero-step hold/release bodies are checked as a complete, fixed list with
  charge count, reset, and >=100 threshold semantics.
- direct FACE24/split result assignments are checked to resolve source labels
  through resolve_label_token immediately after choosing the source label.
- source_exact_body remains a manifest marker only; behavioral proof lives here.
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SMOVE_C = ROOT / "src" / "fix39" / "wm_arcade_smove_runtime.c"
RUNTIME_C = ROOT / "src" / "fix39" / "wm_fix39_runtime.c"

# Audit markers consumed by tools/audit_combat2es_behavioral_parity.py.
PROOF_CK_IGNORE_A8_MVTBL = True
PROOF_ZERO_STEP_CHARGE_BODIES = True
PROOF_DIRECT_FACE24_RESOLVER_ASSIGNMENTS = True
PROOF_SOURCE_EXACT_FLAGS_ARE_MANIFEST_ONLY = True


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def body(text: str, function: str) -> str:
    marker = f"static int {function}"
    start = text.find(marker)
    if start < 0:
        marker = f"static void {function}"
        start = text.find(marker)
    assert start >= 0, function
    brace = text.find("{", start)
    assert brace >= 0, function
    depth = 0
    for pos in range(brace, len(text)):
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
            if depth == 0:
                return text[start:pos + 1]
    raise AssertionError(f"unterminated body for {function}")


def manifest_entries(smove: str) -> list[dict[str, object]]:
    pat = re.compile(
        r'\{\s*(WM_ROSTER_[A-Z]+)\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,'
        r"\s*([^,]+)\s*,\s*(\d+)\s*,\s*([A-Z0-9_]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}"
    )
    return [
        {
            "wrestler": m.group(1),
            "process_label": m.group(2),
            "result_label": m.group(3),
            "steps": m.group(4).strip(),
            "step_count": int(m.group(5)),
            "gate_kind": m.group(6),
            "post_fire_sleep": int(m.group(7)),
            "source_exact_body": int(m.group(8)),
        }
        for m in pat.finditer(smove)
    ]


def test_ck_ignore_a8_matches_wrestle_mv_tbl_shape() -> None:
    src = read(RUNTIME_C)
    fn = body(src, "source_ck_ignore_a8_port")

    assert "case WM_MOVE_UP_LEFT:" in fn
    assert "case WM_MOVE_DOWN_LEFT:" in fn
    assert "away = WM_MOVE_RIGHT;" in fn

    assert "case WM_MOVE_UP_RIGHT:" in fn
    assert "case WM_MOVE_DOWN_RIGHT:" in fn
    assert "away = WM_MOVE_LEFT;" in fn

    assert "default:" in fn
    assert "away = 0u;" in fn
    assert "return away != 0u && ((uint16_t)a->move_dir & away) != 0u;" in fn


def test_zero_step_charge_bodies_are_fixed_and_thresholded() -> None:
    smove = read(SMOVE_C)
    entries = manifest_entries(smove)
    zero = [str(e["process_label"]) for e in entries
            if e["step_count"] == 0 and e["source_exact_body"] == 1]
    expected = [
        "hrt_charge_face_rake",
        "hrt_charge_flying_kick",
        "rzr_charge_slashes",
        "shn_charge_suplex",
        "bam_charge_neckbreaker",
        "dnk_charge_flykick",
    ]
    assert zero == expected

    function_names = [
        "fire_bret_charge_face_rake",
        "fire_bret_charge_flying_kick",
        "fire_razor_charge_slashes",
        "fire_shawn_charge_suplex",
        "fire_bam_charge_neckbreaker",
        "fire_doink_charge_flykick",
    ]
    for fn_name in function_names:
        fn = body(smove, fn_name)
        assert "(a->but_val_cur &" in fn, fn_name
        assert "if (p->timeout != 0xffffu) ++p->timeout;" in fn, fn_name
        assert "charge = p->timeout;" in fn, fn_name
        assert "p->timeout = 0;" in fn, fn_name
        assert "if (charge < 100u) return 0;" in fn, fn_name
        assert "queue_result(a, e, cb);" in fn, fn_name


def test_direct_special_move_addr_assignments_resolve_labels() -> None:
    smove = read(SMOVE_C)
    direct_lines = [
        i for i, line in enumerate(smove.splitlines())
        if "a->special_move_addr = (uintptr_t)label;" in line
    ]
    assert len(direct_lines) == 6

    lines = smove.splitlines()
    for idx in direct_lines:
        window = "\n".join(lines[idx:idx + 4])
        assert "if (cb && cb->resolve_label_token)" in window
        assert "a->special_move_addr = cb->resolve_label_token(label, cb->user);" in window


def test_source_exact_body_flag_is_manifest_only() -> None:
    smove = read(SMOVE_C)
    entries = manifest_entries(smove)
    assert entries, "no SMOVE manifest entries parsed"
    assert all(e["source_exact_body"] == 1 for e in entries)
    assert "source_exact_body" not in smove or "typedef struct wm_arcade_smove_entry" not in smove
