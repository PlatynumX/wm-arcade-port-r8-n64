#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import pathlib
from dataclasses import dataclass, asdict

ROOT = pathlib.Path.cwd()

@dataclass
class Finding:
    subsystem: str
    status: str
    evidence: list[str]
    gaps: list[str]
    ignored: list[str]

def read(path: str) -> str:
    p = ROOT / path
    return p.read_text(errors="replace") if p.exists() else ""

def count_wav64() -> int:
    p = ROOT / "filesystem/dcs"
    if not p.exists():
        return 0
    return len(list(p.glob("stream_*.wav64")))

def load_json(path: str) -> dict:
    p = ROOT / path
    if not p.exists():
        return {}
    return json.loads(p.read_text())

def has_any(text: str, needles: list[str]) -> bool:
    return any(n in text for n in needles)

def boot_attract() -> Finding:
    app = read("src/core/app.c")
    hdr = read("include/wm/app.h")
    ev, gaps = [], []
    for label, needle in [
        ("wm_app_init starts ATTRACT mode", "app->mode = WM_APP_MODE_ATTRACT"),
        ("8-tick ATTRACT boot boundary", "WM_ATTRACT_BOOT_DELAY_TICKS"),
        ("startup SNDSND command 0", "wm_audio_send_command(&app->audio, 0)"),
        ("DCS logo entry command 1005", "wm_audio_send_command(&app->audio, 1005)"),
        ("source attract cycle begin", "begin_base_loop(app)"),
        ("gameplay demo enters match path", "wm_fix39_match_begin"),
        ("gameplay demo uses live combat tick", "wm_fix39_match_tick"),
    ]:
        if needle in app or needle in hdr:
            ev.append(label)
        else:
            gaps.append(label)
    status = "GREEN_WIRED" if not gaps else "YELLOW_GAPS"
    return Finding("boot_attract_entry", status, ev, gaps, [
        "intentional Midway Sports / Be a Man / PlatynumX replacement assets are ignored; timing/control flow is still audited"
    ])

def audio_runtime() -> Finding:
    bind = load_json("assets/dcs/catalog/DCS_PORT_BINDINGS.json")
    audio_h = read("include/wm/audio.h")
    audio_c = read("src/core/audio.c")
    dcs_h = read("include/wm/dcs_port_bindings.h")
    dcs_c = read("src/generated/dcs_r2b_port_bindings.c")
    wav64 = count_wav64()
    ev, gaps = [], []
    if bind.get("decoded_stream_count") == 647:
        ev.append("decoded_stream_count=647")
    else:
        gaps.append(f"decoded_stream_count={bind.get('decoded_stream_count')!r}")
    if bind.get("decoded_command_stream_bindings", 0) >= 704:
        ev.append("decoded command stream bindings >=704")
    else:
        gaps.append("decoded command stream bindings below expected R2B count")
    if wav64 == 647:
        ev.append("filesystem/dcs has 647 WAV64 assets")
    else:
        gaps.append(f"filesystem/dcs wav64 count={wav64}")
    if "wm_dcs_find_first_binding_for_command" in dcs_h and "rom:/dcs/" in dcs_c:
        ev.append("generated C command->DragonFS WAV64 binding table present")
    else:
        gaps.append("generated C DCS runtime binding table missing")
    if "last_command_has_decoded_binding" in audio_h and "wm_dcs_find_first_binding_for_command" in audio_c:
        ev.append("wm_audio_send_command path records decoded DCS binding state")
    else:
        gaps.append("wm_audio_send_command does not record decoded DCS binding")
    if "command == 0u" in audio_c and "stop_events" in audio_c:
        ev.append("command 0 kept as stop/reset control")
    else:
        gaps.append("command 0 stop/reset binding not explicit")
    status = "GREEN_WIRED" if not gaps else "RED_MISSING"
    return Finding("audio_dcs_runtime", status, ev, gaps, [])

def frontend_select() -> Finding:
    app = read("src/core/app.c")
    cmake = read("CMakeLists.txt")
    ev, gaps = [], []
    for label, needle in [
        ("select core compiled", "src/core/select_screen.c"),
        ("continue select compiled", "src/core/select_continue.c"),
        ("P2 start bridge present", "SOURCE_SELECT_P2_START_BRIDGE"),
        ("title start bridge present", "SOURCE_SELECT_TITLE_START_BRIDGE"),
        ("Howard/select bonus state carried", "wm_select_screen_set_howard_done"),
    ]:
        if needle in app or needle in cmake:
            ev.append(label)
        else:
            gaps.append(label)
    status = "GREEN_WIRED" if not gaps else "YELLOW_GAPS"
    return Finding("frontend_select_continue", status, ev, gaps, [
        "cabinet coin/PSTATUS accounting remains an explicit N64 bridge where comments say so"
    ])

def pregame_progression() -> Finding:
    app = read("src/core/app.c")
    cmake = read("CMakeLists.txt")
    ev, gaps = [], []
    for label, needle in [
        ("pregame core compiled", "src/core/pregame.c"),
        ("match lifecycle compiled", "src/fix39/wm_arcade_match_lifecycle.c"),
        ("matchflow compiled", "src/fix39/wm_arcade_matchflow.c"),
        ("story compiled", "src/fix39/wm_arcade_story.c"),
        ("select->pregame handoff present", "wm_pregame_init"),
    ]:
        if needle in app or needle in cmake:
            ev.append(label)
        else:
            gaps.append(label)
    if "WM_APP_MODE_MATCH_INIT" in app and "Explicit boundary: start_match is the next source subsystem" in app:
        gaps.append("MATCH_INIT still stops at explicit start_match boundary")
    status = "YELLOW_BOUNDARY" if gaps else "GREEN_WIRED"
    return Finding("pregame_progression_match_start", status, ev, gaps, [])

def hiscore_persistence() -> Finding:
    cmake = read("CMakeLists.txt")
    ev, gaps = [], []
    for label, needle in [
        ("hiscore adapter compiled", "wmania_hiscore_adapter.c"),
        ("hiscore core compiled", "wmania_hiscore_core.c"),
        ("hiscore persistence compiled", "wmania_hiscore_persist.c"),
        ("hiscore presentation compiled", "wmania_hiscore_present.c"),
    ]:
        if needle in cmake:
            ev.append(label)
        else:
            gaps.append(label)
    persist = read("src/core/arcade/wmania_hiscore_persist.c")
    if has_any(persist.lower(), ["sd", "dragonfs", "filesystem", "file", "fopen"]):
        ev.append("persistence file/SD-style API surface present")
    else:
        gaps.append("SD-card filesystem persistence path not proven by audit")
    status = "YELLOW_BOUNDARY" if gaps else "GREEN_WIRED"
    return Finding("hiscore_persistence", status, ev, gaps, [
        "must not use SRAM/EEPROM/FlashRAM/Controller Pak for final persistent data"
    ])

def rendering_presentation() -> Finding:
    cmake = read("CMakeLists.txt")
    ev, gaps = [], []
    for label, needle in [
        ("visual core compiled", "src/core/visual.c"),
        ("composite compiled", "src/core/composite.c"),
        ("WIMP/source frame binding compiled", "src/fix39/wm_arcade_wimp_frame.c"),
        ("ring geometry compiled", "src/fix39/wmania_ring_geometry.c"),
        ("ring onscreen compiled", "src/fix39/wmania_ring_onscreen.c"),
        ("ring/crowd generated assets compiled", "src/generated/ring_arena_assets.c"),
    ]:
        if needle in cmake:
            ev.append(label)
        else:
            gaps.append(label)
    gaps.append("platform renderer equivalence for palettes/z-order/transparency is not proven in this pass")
    return Finding("rendering_presentation_adapter", "YELLOW_BOUNDARY", ev, gaps, [])

def operator_service() -> Finding:
    cmake = read("CMakeLists.txt")
    app = read("src/core/app.c")
    ev, gaps = [], []
    for label, needle in [
        ("operator attract screen compiled", "wmania_attract_operator.c"),
        ("time/date attract screen compiled", "wmania_attract_time.c"),
        ("copyright/AAMA mapped in attract switch", "WM_FIX39_ATTRACT_AAMA"),
    ]:
        if needle in cmake or needle in app:
            ev.append(label)
        else:
            gaps.append(label)
    gaps.append("arcade bookkeeping/coin/PSTATUS remains N64-boundaried, not a native cabinet port")
    return Finding("operator_service_cabinet_leftovers", "YELLOW_BOUNDARY", ev, gaps, [])

def fake_complete_scan() -> list[str]:
    bad = []
    for path in [
        "src/core/app.c",
        "src/core/audio.c",
        "src/fix39/wm_fix39_runtime.c",
        "include/wm/audio.h",
    ]:
        text = read(path).lower()
        for needle in ["fake complete", "pretend complete", "stub complete", "todo fake"]:
            if needle in text:
                bad.append(f"{path}: {needle}")
    return bad

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--fail-on-fake-complete", action="store_true")
    args = ap.parse_args()

    findings = [
        audio_runtime(),
        boot_attract(),
        frontend_select(),
        pregame_progression(),
        hiscore_persistence(),
        rendering_presentation(),
        operator_service(),
    ]
    fake = fake_complete_scan()

    report_dir = ROOT / "reports"
    report_dir.mkdir(parents=True, exist_ok=True)
    summary = {
        "status": "PASS_WITH_REMAINING_GAPS_REPORTED",
        "scope": "all remaining non-combat systems manifest plus obvious completed wiring",
        "base_expected": "fix39-v13e-dcs-r2b-decoded-port-assets",
        "ignore_custom_branding_assets": True,
        "fake_complete_markers": fake,
        "green": sum(1 for f in findings if f.status.startswith("GREEN")),
        "yellow": sum(1 for f in findings if f.status.startswith("YELLOW")),
        "red": sum(1 for f in findings if f.status.startswith("RED")),
        "findings": [asdict(f) for f in findings],
    }
    (report_dir / "remaining_noncombat_systems_manifest.json").write_text(json.dumps(summary, indent=2) + "\n")

    lines = [
        "# R35 remaining non-combat systems manifest",
        "",
        f"- status: {summary['status']}",
        "- scope: all remaining non-combat systems manifest + install/wire obvious completed pieces + fail/report remaining gaps",
        "- base expected: fix39-v13e-dcs-r2b-decoded-port-assets",
        "- ignored deltas: intentional Midway Sports / Be a Man / PlatynumX branding assets",
        f"- green: {summary['green']}",
        f"- yellow: {summary['yellow']}",
        f"- red: {summary['red']}",
        "",
    ]
    for f in findings:
        lines.extend([f"## {f.subsystem}", "", f"- status: {f.status}", ""])
        if f.evidence:
            lines.append("Evidence:")
            for e in f.evidence:
                lines.append(f"- {e}")
            lines.append("")
        if f.gaps:
            lines.append("Remaining gaps / boundaries:")
            for g in f.gaps:
                lines.append(f"- {g}")
            lines.append("")
        if f.ignored:
            lines.append("Ignored / intentional:")
            for i in f.ignored:
                lines.append(f"- {i}")
            lines.append("")
    if fake:
        lines.append("## Fake-complete markers")
        lines.extend([f"- {x}" for x in fake])
        lines.append("")
    (report_dir / "remaining_noncombat_systems_manifest.md").write_text("\n".join(lines) + "\n")

    print("R35 remaining non-combat systems audit: PASS_WITH_REMAINING_GAPS_REPORTED")
    print(f"green={summary['green']} yellow={summary['yellow']} red={summary['red']}")
    print("report=reports/remaining_noncombat_systems_manifest.md")
    if fake and args.fail_on_fake_complete:
        print("fake complete markers found")
        return 2
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
