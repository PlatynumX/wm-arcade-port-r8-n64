#!/usr/bin/env python3
"""
Combat2ES behavioral parity audit.

The previous Combat2ES passes closed the string/manifest checklist for all active
SMOVE monitors. This audit is deliberately stricter in a different direction: it
reports implementation areas that can pass the label checklist while still not
being exact Midway behavior.

Default exit code is 0 so this can live in CTest as a reporting test. Use
--fail-on-critical to make the currently-known parity debt block CI.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / "reports"
BUILD = ROOT / "build"

SMOVE_C = ROOT / "src" / "fix39" / "wm_arcade_smove_runtime.c"
SMOVE_H = ROOT / "src" / "fix39" / "wm_arcade_smove_runtime.h"
RUNTIME_C = ROOT / "src" / "fix39" / "wm_fix39_runtime.c"
MANIFEST_AUDIT = ROOT / "tools" / "audit_combat_source_manifest.py"
PROOF_TEST = ROOT / "tests" / "test_combat2es_behavioral_proofs.py"


@dataclass
class Finding:
    severity: str
    key: str
    file: str
    evidence: str
    why_it_matters: str
    next_action: str


def _read(path: Path) -> str:
    if not path.exists():
        raise FileNotFoundError(str(path))
    return path.read_text(encoding="utf-8", errors="replace")


def _proof_has(marker: str) -> bool:
    if not PROOF_TEST.exists():
        return False
    return marker in PROOF_TEST.read_text(encoding="utf-8", errors="replace")


def _line_no(text: str, needle: str) -> int:
    idx = text.find(needle)
    if idx < 0:
        return 0
    return text[:idx].count("\n") + 1


def _snippet(text: str, needle: str, radius: int = 160) -> str:
    idx = text.find(needle)
    if idx < 0:
        return ""
    start = max(0, idx - radius)
    end = min(len(text), idx + len(needle) + radius)
    return re.sub(r"\s+", " ", text[start:end]).strip()


def _add_if_present(findings: list[Finding], text: str, path: Path, needle: str,
                    severity: str, key: str, why: str, next_action: str) -> None:
    if needle not in text:
        return
    ln = _line_no(text, needle)
    evidence = f"{path.relative_to(ROOT)}:{ln}: {_snippet(text, needle)}"
    findings.append(Finding(severity, key, str(path.relative_to(ROOT)), evidence, why, next_action))


def _extract_manifest_entries(smove_c: str) -> list[dict[str, str | int]]:
    entries: list[dict[str, str | int]] = []
    # Example:
    # { WM_ROSTER_BRET, "hrt_charge_face_rake", "hrt_rake_face_anim", 0, 0, G_BRET_CHARGE_FACE_RAKE, 1, 1 },
    pat = re.compile(
        r"\{\s*(WM_ROSTER_[A-Z]+)\s*,\s*\"([^\"]+)\"\s*,\s*\"([^\"]+)\"\s*,"
        r"\s*([^,]+)\s*,\s*(\d+)\s*,\s*([A-Z0-9_]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}"
    )
    for m in pat.finditer(smove_c):
        entries.append({
            "wrestler": m.group(1),
            "process_label": m.group(2),
            "result_label": m.group(3),
            "steps": m.group(4).strip(),
            "step_count": int(m.group(5)),
            "gate_kind": m.group(6),
            "post_fire_sleep": int(m.group(7)),
            "source_exact_body": int(m.group(8)),
        })
    return entries


def _run_strict_manifest() -> dict[str, object]:
    if not MANIFEST_AUDIT.exists():
        return {"ran": False, "rc": 127, "stdout": "", "stderr": "missing audit_combat_source_manifest.py"}
    proc = subprocess.run(
        [sys.executable, str(MANIFEST_AUDIT), "--strict-complete"],
        cwd=str(ROOT), text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    return {"ran": True, "rc": proc.returncode, "stdout": proc.stdout, "stderr": proc.stderr}


def _service_body(text: str, function_name: str) -> str:
    marker = f"static void {function_name}"
    start = text.find(marker)
    if start < 0:
        return ""
    brace = text.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    for pos in range(brace, len(text)):
        ch = text[pos]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[start:pos + 1]
    return text[start:]


def _service_findings(runtime_c: str, findings: list[Finding]) -> None:
    fk_body = _service_body(runtime_c, "source_find_and_kill_endless_port")
    if "source_find_and_kill_endless_port" in runtime_c and "wm_sound_find_and_kill_endless" not in fk_body:
        _add_if_present(
            findings,
            runtime_c,
            RUNTIME_C,
            "source_find_and_kill_endless_port",
            "critical",
            "find_and_kill_endless_service_stub",
            "FIND_AND_KILL_ENDLESS is part of many SMOVE bodies. If this service does not call the translated DCSSOUND helper, active SMOVE bodies can leave endless DCS channels alive differently from source.",
            "Bind the service to wm_sound_find_and_kill_endless and keep a focused sound-channel regression test.",
        )

    service_checks = [
        (
            "no separate N64 process class yet",
            "critical",
            "explicit_no_process_class_debt",
            "The code states the source process class is not represented yet. That is parity debt, not finished behavior.",
            "Identify the corresponding source process class and add N64-side lifecycle state instead of treating it as trace-only.",
        ),
        (
            'common_sound_label(a, "DO_REVERSAL_MESS"',
            "major",
            "reversal_message_routed_as_sound_label",
            "DO_REVERSAL_MESS is a source message effect. Routing it through the sound-label path hides the message object boundary.",
            "Route the source hook through explicit message state, then connect the real message renderer path and assert timing/object spawn semantics.",
        ),
        (
            "BONUS_MESS_%03d",
            "major",
            "bonus_message_synthesized_label",
            "BONUS_MESS was converted into a synthesized label/accounting hook. That may not match source scoring, text lifetime, message PID, or display timing.",
            "Translate BONUS_MESS as a real source message event/process and compare bonus values/timing per wrestler.",
        ),
        (
            "a->damage_given += bonus",
            "major",
            "bonus_message_changes_damage_counter",
            "Damage accounting and bonus-message scoring are not necessarily the same source side effect.",
            "Verify the original accounting path before modifying damage_given as a stand-in.",
        ),
        (
            "source_ck_ignore_a8_port",
            "minor",
            "ck_ignore_a8_direction_table_needs_source_proof",
            "ck_ignore/ck_ignore_a8 is direction-table logic in WRESTLE.ASM. The local translation needs exact mv_tbl coverage evidence.",
            "Add a small table-driven test against the WRESTLE.ASM mv_tbl mapping for all facing/move_dir cases.",
        ),
    ]
    for needle, sev, key, why, action in service_checks:
        if key == "ck_ignore_a8_direction_table_needs_source_proof" and _proof_has("PROOF_CK_IGNORE_A8_MVTBL"):
            continue
        _add_if_present(findings, runtime_c, RUNTIME_C, needle, sev, key, why, action)


def _runtime_findings(smove_c: str, findings: list[Finding]) -> None:
    dangerous_terms = [
        ("generic", "major", "generic_helper_term", "Generic helper language is a red flag after label closure; make sure it is shared source code, not approximation."),
        ("shortcut", "major", "shortcut_term", "Shortcut language suggests source behavior may have been compressed."),
        ("approx", "major", "approx_term", "Approximation language is incompatible with the strict port goal."),
        ("TODO", "major", "todo_term", "A TODO in active combat code is unresolved parity work."),
        ("stub", "major", "stub_term", "A stub in active combat code can pass the label checklist while missing behavior."),
        ("source_exact_body", "info", "source_exact_body_flags_present", "The boolean exact-body flag is only a checklist marker; this audit tracks where deeper proof is still needed."),
    ]
    for needle, sev, key, why in dangerous_terms:
        if needle in smove_c:
            if key == "source_exact_body_flags_present" and _proof_has("PROOF_SOURCE_EXACT_FLAGS_ARE_MANIFEST_ONLY"):
                continue
            findings.append(Finding(
                sev, key, str(SMOVE_C.relative_to(ROOT)),
                f"{SMOVE_C.relative_to(ROOT)}:{_line_no(smove_c, needle)}: {_snippet(smove_c, needle)}",
                why,
                "Review each occurrence and either back it with exact source citations/tests or remove the approximation."
            ))

    # Zero-step charge bodies rely on bespoke C branches rather than WAITSWITCH. That is valid for hold/release bodies,
    # but every one needs source-body proof because the string audit cannot validate timing itself.
    entries = _extract_manifest_entries(smove_c)
    zero_step = [e for e in entries if e["step_count"] == 0 and e["source_exact_body"] == 1]
    if zero_step and not _proof_has("PROOF_ZERO_STEP_CHARGE_BODIES"):
        labels = ", ".join(str(e["process_label"]) for e in zero_step)
        findings.append(Finding(
            "minor",
            "zero_step_charge_body_review",
            str(SMOVE_C.relative_to(ROOT)),
            f"zero-step source_exact bodies: {labels}",
            "Hold/release monitors do not go through WAITSWITCH. They need separate timing review for charge thresholds and reset behavior.",
            "Add focused tests for every zero-step body: hold duration below threshold, threshold hit, release reset, mode rejection, and sound/result side effects."
        ))

    # Detect all direct result-label assignment strings that bypass queue_result; this may be required for FACE24 branches,
    # but it should be reviewed because it can skip callbacks/label resolution if not careful.
    direct_assigns = re.findall(r"special_move_addr\s*=\s*\(uintptr_t\)\s*\(?(?:\(?[^;\n]+)", smove_c)
    if direct_assigns and not _proof_has("PROOF_DIRECT_FACE24_RESOLVER_ASSIGNMENTS"):
        findings.append(Finding(
            "minor",
            "direct_special_move_addr_assignments",
            str(SMOVE_C.relative_to(ROOT)),
            f"direct assignments found: {len(direct_assigns)}",
            "Some source bodies pick FACE24/directional result labels manually. Direct writes must still use the label resolver and match source facing choices.",
            "For each direct assignment, add tests for both left/right or up/down facing choices and verify resolve_label_token is called."
        ))


def _write_reports(findings: list[Finding], strict: dict[str, object], entries: list[dict[str, str | int]]) -> None:
    REPORTS.mkdir(parents=True, exist_ok=True)
    counts: dict[str, int] = {}
    for f in findings:
        counts[f.severity] = counts.get(f.severity, 0) + 1

    data = {
        "audit": "combat2es_behavioral_parity",
        "strict_manifest": strict,
        "manifest_entry_count": len(entries),
        "source_exact_entry_count": sum(1 for e in entries if e.get("source_exact_body") == 1),
        "counts": counts,
        "findings": [asdict(f) for f in findings],
    }
    (REPORTS / "combat2es_behavioral_parity_audit.json").write_text(
        json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    lines = []
    lines.append("# Combat2ES behavioral parity audit")
    lines.append("")
    lines.append("This audit starts after active SMOVE label closure. It is not a gameplay change.")
    lines.append("")
    lines.append("## Strict manifest status")
    lines.append("")
    lines.append(f"- strict_manifest_ran: {strict.get('ran')}")
    lines.append(f"- strict_manifest_rc: {strict.get('rc')}")
    lines.append(f"- manifest_entries: {len(entries)}")
    lines.append(f"- source_exact_entries: {sum(1 for e in entries if e.get('source_exact_body') == 1)}")
    lines.append("")
    lines.append("## Finding counts")
    lines.append("")
    if counts:
        for sev in ["critical", "major", "minor", "info"]:
            if sev in counts:
                lines.append(f"- {sev}: {counts[sev]}")
    else:
        lines.append("- none")
    lines.append("")
    lines.append("## Findings")
    lines.append("")
    if not findings:
        lines.append("- none")
    else:
        for f in findings:
            lines.append(f"### {f.severity.upper()}: {f.key}")
            lines.append("")
            lines.append(f"- file: `{f.file}`")
            lines.append(f"- evidence: {f.evidence}")
            lines.append(f"- why: {f.why_it_matters}")
            lines.append(f"- next: {f.next_action}")
            lines.append("")
    (REPORTS / "combat2es_behavioral_parity_audit.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv: Iterable[str] | None = None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--fail-on-critical", action="store_true",
                    help="return nonzero if critical findings are present")
    args = ap.parse_args(list(argv) if argv is not None else None)

    smove_c = _read(SMOVE_C)
    _ = _read(SMOVE_H)
    runtime_c = _read(RUNTIME_C)

    strict = _run_strict_manifest()
    entries = _extract_manifest_entries(smove_c)
    findings: list[Finding] = []

    if strict.get("rc") != 0:
        findings.append(Finding(
            "critical", "strict_manifest_not_green", "tools/audit_combat_source_manifest.py",
            f"strict rc={strict.get('rc')}",
            "Behavioral parity work must not regress the active SMOVE closure checklist.",
            "Fix strict manifest failures before using this behavioral audit."
        ))

    if not entries:
        findings.append(Finding(
            "critical", "manifest_not_parseable", str(SMOVE_C.relative_to(ROOT)),
            "No wm_arcade_smove_entry_t manifest entries parsed.",
            "The audit cannot reason about active SMOVE bodies without parsing the manifest.",
            "Update the parser or inspect manifest formatting changes."
        ))

    _service_findings(runtime_c, findings)
    _runtime_findings(smove_c, findings)

    _write_reports(findings, strict, entries)

    crit = sum(1 for f in findings if f.severity == "critical")
    maj = sum(1 for f in findings if f.severity == "major")
    minor = sum(1 for f in findings if f.severity == "minor")
    print(f"Combat2ES behavioral parity audit: critical={crit} major={maj} minor={minor} total={len(findings)}")
    print("Report: reports/combat2es_behavioral_parity_audit.md")
    if args.fail_on_critical and crit:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
