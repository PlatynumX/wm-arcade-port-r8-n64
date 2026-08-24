from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
app=(ROOT/'tools/apply_fix39.py').read_text()
live=(ROOT/'src/fix39/wmania_attract_live.c').read_text()
runtime=(ROOT/'src/fix39/wm_fix39_runtime.c').read_text()
hdr=(ROOT/'src/fix39/wm_fix39_runtime.h').read_text()
assert 'fix39_tick_gameplay_demo' in app
assert 'case WM_ATTRACT_SHOW_GAMEPLAY: done = fix39_tick_gameplay_demo' in app
assert 'could not locate Sports->gameplay attract test region' in app
assert 'flow_re = re.compile' in app
assert 'for (unsigned i = 0; i < 60u; ++i)' in app
assert 'second source gameplay slot after Title' in app
assert 'could not locate Title->second-gameplay attract test region' in app
assert 'live attract gameplay mutates demo combat counters' in app
assert 'total_hits\\s*==\\s*0' in app
assert 'render_match(app)' in app
assert 'WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1' in live
assert 'wm_fix39_match_set_cpu_vs_cpu' in hdr
assert 'g.match_cpu_vs_cpu' in runtime
assert '&g.actors[0], &g.drone_state[0]' in runtime
print('Fix39 attract demo activation regression: PASS')
