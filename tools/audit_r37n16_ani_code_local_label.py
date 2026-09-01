#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
runtime = (root / "src/fix39/wm_fix39_runtime.c").read_text(errors="ignore")
program = (root / "filesystem/fix39_anim/programs/p1523.bin").read_bytes()

required = [
    "source_anim_code",
    '!strcmp(label,"setup_run")',
    "source_native_setup_run",
    "a->player_mode=WM_PMODE_RUNNING",
]
for item in required:
    assert item in runtime, f"missing runtime source-parity marker: {item}"
    print("PASS:", item)

assert b"#setup_run\x00" in program
print("PASS: generated start_run_anim retains source operand #setup_run")

compact = "".join(runtime.split())
assert "if(label[0]=='#')label++;" in compact
print("PASS: ANI_CODE strips only the Midway local-label sigil before native dispatch")

# Guard against the tempting but wrong fixes: do not rewrite the serialized
# program and do not special-case start_run_anim itself.
assert 'strcmp(label,"#setup_run")' not in runtime
print("PASS: dispatcher remains symbol-based, not a #setup_run one-off")

print("R37N16 ANI_CODE local-label structural audit: PASS")
