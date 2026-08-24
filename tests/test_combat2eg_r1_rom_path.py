#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
b=(root/'termux_fix39_build.sh').read_text()
chunk=(root/'tests/test_v13e_chunk4c.py').read_text()
smoke=(root/'tests/run_v13e_chunk3_script_smoke.sh').read_text()
assert "if (!ran) return WM_DRONE_STEP_SCRIPT;" in chunk
assert "wm_arcade_drone_source_bodies.c" in smoke
assert 'cp "$ROOT/tests/test_v13e_chunk4c.py" tests/test_v13e_chunk4c.py' in b
assert 'cp "$ROOT/tests/run_v13e_chunk3_script_smoke.sh" tests/run_v13e_chunk3_script_smoke.sh' in b
assert 'tests/test_v13e_chunk4c.py tests/run_v13e_chunk3_script_smoke.sh' in b
assert "Combat2EG-R1 canonical DRONE coverage: 15/15" in b
post=b[b.find('Translating conservative state-only DRONE'):b.find('PREFLIGHT="$WORK')]
assert 'python "$ROOT/tools/fix39_strict_runtime_parity_audit.py" "$WORK"' in post
assert '--playable-lane "$WORK"' not in post
assert 'gh run download "$RUN_ID" --name wm-arcade-r9-build' in b
assert "wm_arcade_r9.z64" in b
assert "wm_arcade_fix39_v13e_combat2eg_r1_" in b
print("Combat2EG-R1 ROM-path/stale-regression contract: PASS")
