from pathlib import Path
R=Path(__file__).resolve().parents[1]
r=(R/'src/fix39/wm_fix39_runtime.c').read_text()
h=(R/'src/fix39/wm_fix39_runtime.h').read_text()
rng=(R/'src/fix39/wmania_rng.c').read_text()
assert 'wm_rng_rnd_mask' in rng
assert 'g.drone_callbacks.rnd_upto = drone_rnd_upto;' in r
assert 'g.drone_callbacks.script_seek = drone_script_seek;' in r
assert 'wm_arcade_drone_main(' in r
assert 'g.actors[1].move_dir = g.actors[1].stick_val_cur;' in r
assert 'drone_ticks' in h and 'drone_input_ticks' in h
assert 'wm_fix39_drone_state(size_t index)' in r
print('Fix39 Chunk 6 live P2 DRONE activation guards: PASS')

# Combat2DI+ build ownership: if the baseline carries wmania_rng.c, the
# authoritative N64 source must be src/fix39/wmania_rng.c, never the stale
# src/core/arcade copy.  test_v13_completion exercises patch_makefile itself;
# this guard prevents the obsolete BASELINE_OVERRIDES mechanism returning.
apply=(R/'tools/apply_fix39.py').read_text()
assert 'overlaps = sorted(set(sources) & arcade_names)' in apply
assert 'arcade_names.difference_update(BASELINE_OVERRIDES)' not in apply
