#!/usr/bin/env python3
from pathlib import Path
import sys

root=Path(__file__).resolve().parents[1]
anim=(root/'src/fix39/wm_arcade_source_animation_runtime.c').read_text(errors='replace')
hdr=(root/'src/fix39/wm_arcade_source_animation_runtime.h').read_text(errors='replace')
fix=(root/'src/fix39/wm_fix39_runtime.c').read_text(errors='replace')
gen=(root/'tools/fix39_anim_vm_program.py').read_text(errors='replace')
checks={
 'geometry service declared':'multipart_geometry' in hdr,
 'geometry service bound':'multipart_geometry=source_anim_multipart_geometry' in fix,
 'logical WIMP xani used':'*xani=sp->xani' in fix,
 'logical WIMP yani used':'*yani=sp->yani' in fix,
 'logical WIMP width used':'*xsize=sp->width' in fix,
 'old raw x shortcut removed':'a->attach_xoff=st->entries[off+1].value' not in anim,
 'old raw y shortcut removed':'a->attach_yoff=st->entries[off+2].value' not in anim,
 'attacker flip-vs-facing test':'superslave2_flip_matches_facing' in anim,
 'Y formula':'-(int32_t)dy16+(int32_t)ay16' in anim,
 'defender width correction':'dx=(int32_t)dw-dx' in anim,
 'mismatch mirrors X':'if(!match)x=-x' in anim,
 'defender facing seeds FLIPH':'if(a->facing_dir&WM_MOVE_RIGHT)o->obj_control|=WM_OBJ_FLIPH' in anim,
 'table flip XOR':'if(flip)o->obj_control^=WM_OBJ_FLIPH' in anim,
 'missing geometry faults instead of approximation':'s->fault=79;a->source_vm_fault=79' in anim,
 'frame token canonicalizer present':'def canonical_frame_token(text):' in gen,
 'command frame operands canonicalized':'text=canonical_frame_token(text)' in gen,
 'table frame operands canonicalized':'t=canonical_frame_token(t)' in gen,
}
bad=[k for k,v in checks.items() if not v]
for k,v in checks.items(): print(('PASS' if v else 'FAIL')+': '+k)
if bad:
    print('R37N7 audit FAILED: '+', '.join(bad),file=sys.stderr); raise SystemExit(1)
print('R37N7 ANI_SUPERSLAVE2 structural parity audit: PASS')
