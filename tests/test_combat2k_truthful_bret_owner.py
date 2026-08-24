from pathlib import Path
p=Path('tools/apply_fix39.py').read_text()
assert 'ATTR.ASM SHOW_GAMEPLAY enters the real match path' in p
assert 'wm_demo_tick(&app->demo' not in p
assert 'wm_demo_reset_match(&app->demo)' not in p
assert 'wm_fix39_match_begin((unsigned)app->p1_choice, (unsigned)app->p2_choice);' in p
print('Combat2CO ATTR gameplay ownership regression: PASS')
