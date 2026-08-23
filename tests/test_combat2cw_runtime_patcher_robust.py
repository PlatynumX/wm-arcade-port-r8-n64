from pathlib import Path
import subprocess, tempfile, textwrap, shutil, sys
ROOT = Path(__file__).resolve().parents[1]
PATCHER = ROOT / 'tools/fix39_combat_runtime_parity_patch.py'
BASE_RUNTIME = ROOT / 'src/fix39/wm_fix39_runtime.c'
BASE_HEADER = ROOT / 'src/fix39/wm_fix39_runtime.h'

def run_case(torso_decl: str):
    with tempfile.TemporaryDirectory() as td:
        repo = Path(td)
        (repo/'src/fix39').mkdir(parents=True)
        (repo/'src/platform/n64').mkdir(parents=True)
        shutil.copy2(BASE_RUNTIME, repo/'src/fix39/wm_fix39_runtime.c')
        shutil.copy2(BASE_HEADER, repo/'src/fix39/wm_fix39_runtime.h')
        n64 = textwrap.dedent(f'''\
            #include <stdbool.h>
            #include <stddef.h>
            #include <stdint.h>
            static int fix39_runtime_draw_index = -1;
            typedef struct wm_demo_fighter {{ int action; uint8_t roster_id; int torso_visual; }} wm_demo_fighter;
            typedef struct wm_visual_frame {{ const char *source_frame; }} wm_visual_frame;
            typedef struct wm_source_sprite {{ int x; }} wm_source_sprite;
            typedef struct wm_arcade_actor {{ int wrestler_num; }} wm_arcade_actor_t;
            enum {{ WM_DEMO_IDLE, WM_DEMO_WALK, WM_DEMO_BLOCK }};
            static bool fighter_uses_torso_layer(const wm_demo_fighter *f) {{
                return f->action == WM_DEMO_IDLE || f->action == WM_DEMO_WALK ||
                       f->action == WM_DEMO_BLOCK;
            }}
            static void draw(const wm_demo_fighter *f) {{
                const wm_visual_frame *torso_frame = wm_visual_current(&f->torso_visual);
            {torso_decl}
                (void)torso_frame; (void)torso;
            }}
        ''')
        (repo/'src/platform/n64/main.c').write_text(n64)
        subprocess.run([sys.executable, str(PATCHER), str(repo)], check=True, capture_output=True, text=True)
        out=(repo/'src/platform/n64/main.c').read_text()
        assert 'wm_fix39_actor_source_torso_frame((size_t)fix39_runtime_draw_index)' in out
        assert 'if(a&&tf) torso=wm_character_sprite_find((uint8_t)a->wrestler_num,tf);' in out
        assert 'torso=wm_character_sprite_find(f->roster_id,torso_frame->source_frame);' in out
        subprocess.run([sys.executable, str(PATCHER), str(repo)], check=True, capture_output=True, text=True)

run_case('''    const wm_source_sprite *torso = torso_frame\n        ? wm_character_sprite_find(f->roster_id, torso_frame->source_frame) : NULL;''')
run_case('''    const wm_source_sprite *torso=torso_frame ? wm_bret_sprite_find(torso_frame->source_frame):NULL;''')
print('Combat2CW robust runtime patcher regression: PASS')
