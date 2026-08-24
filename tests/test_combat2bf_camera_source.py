#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
c=(R/'src/fix39/wm_fix39_runtime.c').read_text()
h=(R/'src/fix39/wm_fix39_runtime.h').read_text()
a=(R/'tools/apply_fix39.py').read_text()
p=(R/'tools/fix39_camera_renderer_patch.py').read_text()
need=[
 ('init x','g.camera.worldtlx_fp16 = (WM_RING_X_CENTER - 200) << 16' in c),
 ('init y','g.camera.worldtly_fp16 = -(27 << 16)' in c),
 ('buffer','delta += (20 << 16)' in c and 'delta -= (20 << 16)' in c),
 ('x eighth','delta >> 3' in c),
 ('x limits','0x12f << 16' in c and '0x648 << 16' in c),
 ('yscale','0x3566' in c),
 ('ymid','0x0d8 << 16' in c),
 ('y quarter','delta >> 2' in c),
 ('front fence','0x97 << 16' in c),
 ('camera api','wm_fix39_camera_worldtlx_int' in h and 'wm_fix39_camera_worldtly_int' in h),
 ('no presenter pose injection','wm_fix39_match_sync_presenter_pose(0, app->demo.p1.screen_x' not in a),
 ('arena live camera','WM_RING_WORLD_TL_X ((int)wm_fix39_camera_worldtlx_int())' in p),
 ('rope live camera','wm_fix39_camera_worldtlx_int()' in p and 'wm_fix39_camera_worldtly_int()' in p),
 ('fighter source projection','((int64_t)a->z_int*0x3566)>>16' in p),
]
bad=[n for n,ok in need if not ok]
if bad: raise SystemExit('Combat2BG camera parity missing: '+', '.join(bad))
print('Combat2BG camera source parity: PASS')
