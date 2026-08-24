#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
rt=(R/'src/fix39/wm_arcade_source_animation_runtime.c').read_text(errors='replace')
legacy=(R/'tests/test_combat2bp_animation_runtime.py').read_text(errors='replace')
assert 'case 22:' in rt and 'a->obj_friction=av(i,0)' in rt
assert "WM_SRC_ANIM_INIT_FRICTION' in h" in legacy
assert "WM_SRC_ANIM_INIT_FRICTION','obj_friction'" not in legacy
print('Combat2CF full-VM stale-regression correction: PASS')
