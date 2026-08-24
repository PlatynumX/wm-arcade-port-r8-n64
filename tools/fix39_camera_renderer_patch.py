#!/usr/bin/env python3
from pathlib import Path
import sys

repo=Path(sys.argv[1])
p=repo/'src/platform/n64/main.c'
t=p.read_text()

t=t.replace('#define WM_RING_WORLD_TL_X ((0x400 + 50) - 200)',
            '#define WM_RING_WORLD_TL_X ((int)wm_fix39_camera_worldtlx_int())')
t=t.replace('#define WM_RING_WORLD_TL_Y (-27)',
            '#define WM_RING_WORLD_TL_Y ((int)wm_fix39_camera_worldtly_int())')

t=t.replace('const int obj_x=((int)(wm_rope_spawn_x_fp16(s)>>16))-(WM_RING_X_CENTER-200);',
            'const int obj_x=((int)(wm_rope_spawn_x_fp16(s)>>16))-wm_fix39_camera_worldtlx_int();')
t=t.replace('const int obj_y=((int)(wm_rope_spawn_y_fp16(s)>>16))-(-27);',
            'const int obj_y=((int)(wm_rope_spawn_y_fp16(s)>>16))-wm_fix39_camera_worldtly_int();')

anchor='static __attribute__((unused)) void render_match(const wm_app *app) {'
if anchor not in t:
    raise SystemExit('camera renderer: render_match anchor missing')
helper='''static void fix39_project_actor(size_t index, int *sx, int *sy, bool *flip_x) {
    const wm_arcade_actor_t *a=wm_fix39_actor(index);
    if (!a) { if(sx)*sx=0; if(sy)*sy=0; if(flip_x)*flip_x=false; return; }
    if (sx) *sx=(int)a->x_int-(int)wm_fix39_camera_worldtlx_int();
    if (sy) *sy=(int)(((int64_t)a->z_int*0x3566)>>16)-(int)a->y_int-(int)wm_fix39_camera_worldtly_int();
    if (flip_x) *flip_x=(a->obj_control&WM_OBJ_FLIPH)!=0;
}
static void fix39_draw_fighter_from_source(const wm_demo_fighter *src,size_t index){
    wm_demo_fighter f=*src;
    fix39_project_actor(index,&f.screen_x,&f.screen_y,&f.flip_x);
    draw_fighter(&f);
}
'''
if 'static void fix39_project_actor(' not in t:
    t=t.replace(anchor,helper+anchor,1)

fn=t.index(anchor)
brace=t.index('{',fn)
depth=0
fn_end=None
for i in range(brace,len(t)):
    if t[i]=='{': depth+=1
    elif t[i]=='}':
        depth-=1
        if depth==0:
            fn_end=i+1
            break
if fn_end is None:
    raise SystemExit('camera renderer: render_match closing brace missing')
body=t[fn:fn_end]

demo_decl='const wm_demo *demo = &app->demo;'
if demo_decl not in body:
    raise SystemExit('camera renderer: demo declaration missing')
if 'fix39_project_actor(0,&p1x,&p1y,&p1f);' not in body:
    body=body.replace(demo_decl,demo_decl+'''\n    int p1x,p1y,p2x,p2y; bool p1f,p2f;\n    fix39_project_actor(0,&p1x,&p1y,&p1f);\n    fix39_project_actor(1,&p2x,&p2y,&p2f);\n    (void)p1x; (void)p2x; (void)p1f; (void)p2f;''',1)

old_cond='demo->p1.screen_y <= demo->p2.screen_y'
if old_cond in body:
    body=body.replace(old_cond,'p1y <= p2y',1)
elif 'p1y <= p2y' not in body:
    raise SystemExit('camera renderer: fighter depth condition missing')

for old,new in [
    ('draw_fighter(&demo->p1);','fix39_draw_fighter_from_source(&demo->p1,0);'),
    ('draw_fighter(&demo->p2);','fix39_draw_fighter_from_source(&demo->p2,1);')]:
    if old in body:
        body=body.replace(old,new)
    if new not in body:
        raise SystemExit('camera renderer: fighter draw seam missing: '+old)

if old_cond in body or 'draw_fighter(&demo->p1);' in body or 'draw_fighter(&demo->p2);' in body:
    raise SystemExit('camera renderer: presenter-owned render seam survived')

t=t[:fn]+body+t[fn_end:]
p.write_text(t)
print('Combat2BG WRESTLE2 scroll_world camera renderer patch applied structurally')
