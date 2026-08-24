#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]

b  = (root / "termux_fix39_build.sh").read_text()
p  = (root / "tools/fix39_playable_lane_patch.py").read_text()
sp = (root / "tools/fix39_source_proof_gate.py").read_text()

# Frozen hardware-working Combat2AM visual lane remains intentional.
assert "combat2eg" in b.lower()
assert "fix39_character_assets_development_frozen.py" in b
assert """python tools/fix39_character_assets.py \
  --root original/wwf-wrestlemania""" not in b
assert "FIX39_WIMP_RESEARCH" in b

# Exact frozen-generator provenance.
assert "14a5f7a739e69de39d3912e8b70c8f33dd9ccc8b" in p
assert "75a371c2e92685982666a080adcedccb3aa5a52c" in p
assert "1280555b4d041dd025198c8e85ed14b4c1c91cfb" in p

assert 'STRICT_TOOL = "tools/fix39_character_assets.py"' in p
assert 'FROZEN_TOOL = "tools/fix39_character_assets_development_frozen.py"' in p
assert "ca=text('tools/fix39_character_assets.py')" in sp

# Revision-independent logging / ROM naming.
assert "wm_arcade_fix39_v13e_combat2eg_" in b
assert 'FAIL_LOG="$DOWNLOAD_DIR/fix39-v13e-' in b
assert '-${RUN_ID}-failed.log"' in b

# Combat2EG no longer accepts the old DRONE playable-gap contract.
assert "canonical DRONE coverage: 15/15" in b
assert 'fix39_strict_runtime_parity_audit.py" "$WORK"' in b
assert '--playable-lane "$WORK"' not in b

print("Combat2EG frozen-visual + strict DRONE 15/15 contract: PASS")
