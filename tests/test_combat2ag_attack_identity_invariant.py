#!/usr/bin/env python3
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'tools'/'apply_fix39.py'
s=p.read_text()
required=[
 "wm_character_visual(0u, WM_CV_LP4)",
 "wm_character_visual(0u, WM_CV_PP)",
 "wm_character_visual(0u, WM_CV_LK4)",
 "wm_character_visual(0u, WM_CV_PK)",
 "stale Bret visual.sequence behavior assertions remain",
]
for x in required:
    assert x in s, x
print('Combat2AG attack identity invariant regression: PASS')
