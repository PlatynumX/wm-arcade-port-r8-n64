#!/usr/bin/env python3
from pathlib import Path
import tempfile, subprocess
R=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
    repo=Path(td)
    p=repo/'src/platform/n64/main.c'; p.parent.mkdir(parents=True)
    p.write_text('''#include <stdbool.h>\n#define WM_RING_X_CENTER 1074\n#define WM_RING_WORLD_TL_X ((0x400 + 50) - 200)\n#define WM_RING_WORLD_TL_Y (-27)\nstatic void draw_ring_back(void){}\nstatic void draw_ring_front(void){}\nstatic void draw_fighter(const wm_demo_fighter *f){}\nstatic __attribute__((unused)) void render_match(const wm_app *app) {\n    const wm_demo *demo = &app->demo;\n    draw_ring_back();\n    if (demo->p1.screen_y <= demo->p2.screen_y) {\n        draw_fighter(&demo->p1);\n        draw_fighter(&demo->p2);\n    } else {\n        draw_fighter(&demo->p2);\n        draw_fighter(&demo->p1);\n    }\n    draw_ring_front();\n}\n''')
    subprocess.run(['python3',str(R/'tools/fix39_camera_renderer_patch.py'),str(repo)],check=True,capture_output=True,text=True)
    s=p.read_text()
    assert 'p1y <= p2y' in s
    assert 'fix39_draw_fighter_from_source(&demo->p1,0);' in s
    assert 'fix39_draw_fighter_from_source(&demo->p2,1);' in s
    assert 'demo->p1.screen_y <= demo->p2.screen_y' not in s
print('Combat2BG structural camera patcher: PASS')
