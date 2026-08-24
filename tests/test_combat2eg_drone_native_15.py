#!/usr/bin/env python3
from pathlib import Path
import re, subprocess, sys, tempfile
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
tr=(root/'tools/fix39_drone_translate.py').read_text()
body=(root/'src/fix39/wm_arcade_drone_source_bodies.c').read_text()
hdr=(root/'src/fix39/wm_arcade_drone_source_bodies_generated.h').read_text()
bld=(root/'termux_fix39_build.sh').read_text()
labels=re.findall(r'\("([^"]+)",\s*"(wm_fix39_native_[^"]+)"\)',tr.split('NATIVE_15 =',1)[1].split(']',1)[0])
assert len(labels)==15, len(labels)
assert '#define WM_FIX39_DRONE_TRANSLATED_BODY_COUNT 15' in hdr
for label,fn in labels:
    assert f'{{"{label}", {fn}}}' in hdr, label
    assert re.search(rf'\b{re.escape(fn)}\s*\(',body), fn
assert 'git add "${STAGE_PATHS[@]}"' in bld
assert 'tests/test_combat2ee_rndper_keep_onscreen_source_parity.py' in bld
assert 'tests/test_combat2eg_drone_native_15.py' in bld
with tempfile.TemporaryDirectory() as td:
    inc=['-I'+str(root/'src/fix39'),'-I'+str(root/'include')]
    for rel in ['src/fix39/wm_arcade_drone_source_bodies.c','src/fix39/wm_arcade_drone.c']:
        out=Path(td)/(Path(rel).stem+'.o')
        subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror',*inc,'-c',str(root/rel),'-o',str(out)],check=True)
    harness=Path(td)/'h.c'
    harness.write_text(r'''#include <assert.h>
#include <string.h>
#include "wm_arcade_drone_source_bodies.h"
#include "wm_arcade_drone_source_services.h"
#include "wmania_rng.h"
#include "wmania_ring_geometry.h"
int main(void){
 WmRng rng; wm_arcade_actor_t s,o; wm_arcade_drone_state_t d;
 memset(&s,0,sizeof(s)); memset(&o,0,sizeof(o)); memset(&d,0,sizeof(d));
 wm_rng_init(&rng,0x12345678u,0,0,0); wm_rng_set_latched_inputs(&rng,0x1357u,0x2468u);
 wm_arcade_drone_source_service_reset_handlers();
 assert(wm_arcade_drone_source_generated_body_count()==15);
 assert(wm_arcade_drone_source_install_generated_bodies()==15);
 s.player_mode=WM_PMODE_NORMAL; s.wrestler_num=0; s.x_int=WM_RING_X_CENTER; s.z_int=WM_RING_TOP;
 o.player_mode=WM_PMODE_NORMAL; o.x_int=WM_RING_X_CENTER+50; o.z_int=WM_RING_TOP+150;
 assert(wm_arcade_drone_source_service_dispatch(&s,&o,&d,"drn_taunt@EXGPC_0000",&rng)==1);
 assert(d.anim_request && strcmp(d.anim_request,"hrt_4_taunt_anim")==0);
 memset(&d,0,sizeof(d)); s.in_ring=1; o.in_ring=0; s.x_int=WM_RING_X_CENTER; s.z_int=WM_RING_TOP-10;
 assert(wm_arcade_drone_source_service_dispatch(&s,&o,&d,"drn_enterring@EXGPC_0000",&rng)==1); assert(d.but!=0);
 memset(&d,0,sizeof(d)); s.wrestler_num=0;
 assert(wm_arcade_drone_source_service_dispatch(&s,&o,&d,"drone_chrg",&rng)==1); assert(d.but_charge!=0 && d.but_charge_delay==120);
 return 0;
}
''')
    exe=Path(td)/'h'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror',*inc,str(harness),
                    str(root/'src/fix39/wm_arcade_drone_source_bodies.c'),
                    str(root/'src/fix39/wm_arcade_drone_source_services.c'),
                    str(root/'src/fix39/wm_arcade_drone_source_scripts.c'),
                    str(root/'src/fix39/wmania_rng.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Combat2EG native DRONE 15/15 + staging regression: PASS')
