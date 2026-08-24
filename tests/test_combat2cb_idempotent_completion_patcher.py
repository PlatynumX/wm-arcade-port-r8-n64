from pathlib import Path
s=(Path(__file__).parents[1]/'tools/fix39_combat_completion_patch.py').read_text()
assert 'already_source_owned = source_owned_marker in t' in s
assert 'if start < 0 and not already_source_owned:' in s
assert 'if start >= 0:' in s
assert 'presenter attack block start missing' not in s
print('Combat2CB idempotent completion patcher: PASS')
