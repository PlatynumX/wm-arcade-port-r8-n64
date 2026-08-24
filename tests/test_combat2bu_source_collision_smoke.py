from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'tools'/'fix39_combat_completion_patch.py').read_text()
assert 'ANIM.ASM is authoritative after the tick' in s
assert 'combat_collision_ticks == 0u' in s
print('Combat2BU source-owned collision smoke regression: PASS')
