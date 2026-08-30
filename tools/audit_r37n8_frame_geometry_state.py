#!/usr/bin/env python3
from __future__ import annotations
import pathlib, sys

ROOT = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else pathlib.Path.cwd()

def read(rel):
    p=ROOT/rel
    if not p.exists(): raise SystemExit(f"missing {p}")
    return p.read_text(encoding="utf-8")

def need(text, token, label):
    if token not in text: raise SystemExit(f"FAIL {label}: missing {token!r}")

def forbid(text, token, label):
    if token in text: raise SystemExit(f"FAIL {label}: stale {token!r}")

h=read("src/fix39/wm_arcade_source_animation_runtime.h")
c=read("src/fix39/wm_arcade_source_animation_runtime.c")
f=read("src/fix39/wm_fix39_runtime.c")
cm=read("CMakeLists.txt")

for token in (
    "wm_source_anim_frame_geometry_t",
    "bool (*frame_geometry)",
    "wm_source_anim_frame_geometry_t frame_geometry;",
    "wm_source_anim_runtime_force_frame_actor",
    "wm_source_anim_runtime_copy_geometry_frame",
    "wm_source_anim_runtime_geometry",
): need(h,token,"header contract")

need(c,"static int source_anim_copy_geometry", "independent geometry copy")
need(c,"static int source_anim_apply_frame", "atomic frame apply")
need(c,"wm_source_anim_runtime_copy_geometry_frame", "public distinct geometry-owner API")
need(c,"source_anim_apply_frame(s,a,i->name)", "ordinary frame copy")
need(c,"source_anim_apply_frame(s,a,afr)", "opcode 79 attacker copy")
need(c,"memset(&s->frame_geometry,0,sizeof(s->frame_geometry))", "stale geometry invalidation")
need(c,"geom.valid=1u", "geometry validity")

for token in (
    "out->width=sp->width;",
    "out->height=sp->height;",
    "out->xani=sp->xani;",
    "out->yani=sp->yani;",
    "out->iani3x=sp->wimp_tail[WM_WIMP_IANI3_X_SLOT];",
    "out->iani3id=sp->wimp_tail[WM_WIMP_IANI3_ID_SLOT];",
    "wm_source_anim_runtime_force_frame_actor(&g.source_anim[i],o,frame)",
    "live_source_runtime_frame_box",
    "wm_source_anim_runtime_geometry(runtime)",
): need(f,token,"live geometry ownership")

forbid(f,"live_source_cur_frame_box", "late display-token collision lookup")
need(cm,"wm_r37n8_frame_geometry_state_audit", "CMake audit")
need(cm,"wm_r37n8_frame_geometry_state_model", "CMake model")
print("R37N8 frame geometry state structural audit: PASS")
