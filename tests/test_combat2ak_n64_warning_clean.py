from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = (root / 'src/fix39/wm_arcade_source_attack_frames.c').read_text()

assert 'return false;for' not in src
assert 'if (!source_frame || !uses_z || !zargs || !args) {' in src
assert '\n    for (unsigned i = 0; i < n; i++) {' in src
assert 'if (strcmp(source_frame, r->frame)) {' in src
assert 'continue;' in src
print('Combat2AK N64 -Wmisleading-indentation regression: PASS')
