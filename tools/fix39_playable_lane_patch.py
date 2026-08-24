#!/usr/bin/env python3
from __future__ import annotations
import pathlib, subprocess, sys

COMBAT2AM_COMMIT = "14a5f7a739e69de39d3912e8b70c8f33dd9ccc8b"
COMBAT2AM_GENERATOR_BLOB = "75a371c2e92685982666a080adcedccb3aa5a52c"
MIDWAY_SOURCE_COMMIT = "1280555b4d041dd025198c8e85ed14b4c1c91cfb"
STRICT_TOOL = "tools/fix39_character_assets.py"
FROZEN_TOOL = "tools/fix39_character_assets_development_frozen.py"


def die(msg: str) -> None:
    raise SystemExit("Combat2EF playable-lane patch: " + msg)


def run(repo: pathlib.Path, *args: str) -> bytes:
    try:
        return subprocess.check_output(args, cwd=repo)
    except subprocess.CalledProcessError as exc:
        die(f"command failed ({exc.returncode}): {' '.join(args)}")


def main() -> None:
    repo = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    if not (repo / ".git").exists():
        die(f"not a git checkout: {repo}")
    strict = repo / STRICT_TOOL
    if not strict.is_file():
        die(f"strict EC converter missing: {STRICT_TOOL}")

    # Recover the exact generator from the last hardware-working Combat2AM lineage.
    # Do not copy its semantics into the strict EC research converter.
    data = run(repo, "git", "show", f"{COMBAT2AM_COMMIT}:{STRICT_TOOL}")
    frozen = repo / FROZEN_TOOL
    frozen.write_bytes(data)
    blob = run(repo, "git", "hash-object", FROZEN_TOOL).decode().strip()
    if blob != COMBAT2AM_GENERATOR_BLOB:
        die(f"Combat2AM frozen generator blob mismatch: {blob} != {COMBAT2AM_GENERATOR_BLOB}")

    wf = repo / ".github/workflows/build.yml"
    if not wf.is_file():
        die("build workflow missing")
    text = wf.read_text()
    old = "python3 tools/fix39_character_assets.py --root original/wwf-wrestlemania --out-c src/generated/character_assets.c --out-h include/wm/character_assets.h --out-fs filesystem/fix39_chars"
    new = "python3 tools/fix39_character_assets_development_frozen.py --root original/wwf-wrestlemania --out-c src/generated/character_assets.c --out-h include/wm/character_assets.h --out-fs filesystem/fix39_chars"
    count = text.count(old)
    if count < 2:
        die(f"expected at least host + N64 character-generation commands, found {count}")
    text = text.replace(old, new)
    text = text.replace(
        "      - name: Generate all wrestler source art and attack frames\n",
        "      - name: Generate development-frozen Combat2AM wrestler art and current attack frames\n",
    )
    marker = "# FIX39 COMBAT2EF PLAYABLE LANE: Combat2AM visual generator is development-only; strict EC converter remains final authority."
    if marker not in text:
        first = text.find(new)
        line_start = text.rfind("\n", 0, first) + 1 if first >= 0 else -1
        if line_start >= 0:
            indent = text[line_start:first]
            text = text[:line_start] + indent + marker + "\n" + text[line_start:]
    wf.write_text(text)

    # Fail if a ROM-producing workflow invocation still points at the unresolved strict converter.
    check = wf.read_text()
    if old in check:
        die("strict WIMP converter still owns a ROM-producing workflow generation command")
    if check.count(new) < 2:
        die("frozen playable generator is not wired into both host and N64 jobs")

    docs = repo / "docs/COMBAT2EF_PLAYABLE_LANE.md"
    docs.parent.mkdir(parents=True, exist_ok=True)
    docs.write_text(
        f"# Combat2EF playable lane\n\n"
        f"This revision deliberately separates ROM production from unresolved LOADW/WIMP research.\n\n"
        f"- Frozen development visual generator source commit: `{COMBAT2AM_COMMIT}`\n"
        f"- Frozen generator Git blob: `{COMBAT2AM_GENERATOR_BLOB}`\n"
        f"- Historical Midway source commit: `{MIDWAY_SOURCE_COMMIT}`\n"
        f"- Final strict converter remains: `{STRICT_TOOL}`\n"
        f"- ROM-producing development converter: `{FROZEN_TOOL}`\n\n"
        "The frozen converter reproduces the last hardware-working Combat2AM character-art pipeline. "
        "It is a development fixture, not a claim that LOADW palette/index semantics are solved. "
        "Final art conversion remains gated by the strict EC source-proof path.\n"
    )

    print("Combat2EF playable-lane patch: PASS")
    print(f"Combat2AM generator blob: {blob}")
    print(f"Workflow frozen invocations: {check.count(new)}")


if __name__ == "__main__":
    main()
