from pathlib import Path
root=Path(__file__).resolve().parents[1]
r=(root/'src/fix39/wm_fix39_runtime.c').read_text()
h=(root/'src/fix39/wm_fix39_runtime.h').read_text()
p=(root/'tools/apply_fix39.py').read_text()
required=[
    'live_keep_onscreen_from_camera();',
    'live_ringout_tick();',
    'wm_ring_are_we_in_ring_tick(',
    'wm_ring_kill_when_hit_ground_apply(',
    'wm_fix39_match_spawn_special(',
    'wm_arcade_spawn_doink_pie',
    'wm_arcade_spawn_bam_fireball',
    'wm_arcade_spawn_taker_spirit',
    'wm_arcade_spawn_taker_reaper',
    'wm_arcade_spawn_yoko_salt',
    'g.status.special_spawn_command_service_ready = true;',
    'g.status.ringout_operator_state_ready = true;',
    'g.status.secret_input_scheduler_ready = true;',
    'wm_fix39_hiscore_bind_persistence(',
    'wm_hs_save_read(',
    'wm_hs_save_write(',
]
for tok in required:
    assert tok in r or tok in h, tok
# Source inclusion is dynamic in Combat2DI+: apply_fix39 discovers every
# src/fix39/*.c module and the build-graph convergence layer makes overlapping
# names authoritative.  Validate the service modules themselves plus that
# general source-discovery contract instead of demanding filenames be hardcoded
# in the patcher implementation.
assert 'sources = sorted(p.name for p in dest.glob("*.c"))' in p
for src in ['wmania_ring_out.c','wmania_hiscore_persist.c','wmania_attract_secret.c','wm_arcade_special.c']:
    assert (root/'src/fix39'/src).is_file(), src
assert 'ring-out damage still waits for live RING_OUT_ON/operator state' not in r
assert 'projectile process/collision ticks are live, while spawn commands still' not in r
assert 'live_scroll_world_attract();' in r
print('Fix39 ported-service live wiring regression with camera-owned onscreen state: PASS')
