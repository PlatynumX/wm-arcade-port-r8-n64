#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1])
p=repo/'src/fix39/wm_fix39_runtime.c'
t=p.read_text()
anchor="""    if (g.status.drone_runtime_ready) {
        wm_arcade_drone_world_t world;"""
insert="""    /* WRESTLE.ASM::wrestler_main calls update_newfacing before drone_main.
       Keep the translated actor facing/world direction current before DRONE
       chooses seek/action input and before character move dispatch consumes it. */
    live_source_face_opponents();
    refresh_distances();

"""
if insert not in t:
    if anchor not in t: raise SystemExit('Combat2BG source-order anchor missing')
    t=t.replace(anchor,insert+anchor,1)
old="""    live_source_face_opponents();
    refresh_distances();

    /* SPECIAL.ASM process state"""
if old in t:
    t=t.replace(old,"""    refresh_distances();

    /* SPECIAL.ASM process state""",1)
p.write_text(t)
print('Combat2BG WRESTLE.ASM facing/drone source order patch applied')
