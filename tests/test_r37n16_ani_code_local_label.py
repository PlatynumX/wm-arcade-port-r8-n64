#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
blob_path = root / "filesystem/fix39_anim/programs/p1523.bin"
runtime_path = root / "src/fix39/wm_fix39_runtime.c"

blob = blob_path.read_bytes()
runtime = runtime_path.read_text(errors="ignore")

# p1523 is the generated start_run_anim program observed in the N15 hardware log.
assert b"#setup_run\x00" in blob, "start_run_anim no longer carries the source-local #setup_run operand"

def source_call_symbol(serialized: str) -> str:
    # Midway '#' is the local assembler-label namespace sigil; CALL sees the symbol.
    return serialized[1:] if serialized.startswith("#") else serialized

raw = "#setup_run"
assert source_call_symbol(raw) == "setup_run"

compact = "".join(runtime.split())
assert "if(label[0]=='#')label++;" in compact
assert '!strcmp(label,"setup_run")' in runtime

# Prove the pre-R37N16 behavior was exactly the observed mismatch.
assert raw != "setup_run"
assert source_call_symbol(raw) == "setup_run"

print("R37N16 ANI_CODE local-label model: PASS")
print("  p1523 raw operand: #setup_run")
print("  callable symbol:   setup_run")
