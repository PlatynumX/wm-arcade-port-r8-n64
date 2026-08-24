from pathlib import Path
p=Path(__file__).resolve().parents[1]/'tools'/'fix39_combat_completion_patch.py'
t=p.read_text()
assert 'missing_gameplay_helper = sp is None' in t
assert "marker='static bool tick_title(wm_app *app, const wm_input_state *input) {'" in t
assert "proto='static bool fix39_tick_gameplay_demo(wm_app *app, const wm_input_state *input);\\n'" in t
assert "nt=nt.rstrip()+'\\n\\n'+newfn+'\\n'" in t
assert 'N64 gameplay helper insertion point missing' in t
assert 'N64 gameplay helper and tick_title insertion anchor missing' not in t
seg=t[t.index("newfn='''"):t.index('# renderer helper')]
assert 'wm_fix39_match_tick(0,0,false,false,false,false,false,false);' in seg
print('Combat2BS helper insertion without tick_title anchor: PASS')
