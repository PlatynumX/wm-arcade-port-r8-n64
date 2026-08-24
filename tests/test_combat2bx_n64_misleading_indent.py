from pathlib import Path
s=Path('tools/fix39_combat_completion_patch.py').read_text()
assert 'if(index>=WM_FIX39_ACTOR_COUNT)return;\n    a=&g.actors[index];' in s
assert 'if(index>=WM_FIX39_ACTOR_COUNT)return; a=&g.actors[index];' not in s
print('Combat2BX N64 -Wmisleading-indentation regression: PASS')
