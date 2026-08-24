#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1])
p=repo/'src/platform/n64/main.c'; t=p.read_text()
inc='#include "wm/ring_rope_assets.h"\n#include "wmania_rope_spawn.h"\n#include "wmania_ring_geometry.h"\n#include <stdlib.h>\n'
anchor='#include "wm/bret_sprites.h"\n'
if '#include "wm/ring_rope_assets.h"' not in t:
    if anchor not in t: raise SystemExit('ring patch: include anchor missing')
    t=t.replace(anchor,anchor+inc,1)
start=t.index('static void draw_ring_back(void) {')
end=t.index('static const wm_source_sprite *bret_object_palette',start)
code=r'''/* Combat2AU: ROPES.ASM source-object renderer.  Object positions are the
   literal LWWWWW seeds translated through BEGINOBJ, then DISPLAY.ASM world-TL
   subtraction.  WRESTLE.ASM init_scroller sets WORLDTLX=RING_X_CENTER-200 and
   WORLDTLY=-27 for the normal 1v1 attract match. */
typedef struct { const wm_ring_rope_asset *a; uint8_t *mem; size_t bytes; uint32_t stamp; } fix39_rope_cache;
static fix39_rope_cache fix39_rope_cache_slots[12];
static uint32_t fix39_rope_stamp;
static const uint8_t *fix39_rope_load(const wm_ring_rope_asset *a, uint16_t **pal) {
    if (!a || !pal) return NULL;
    ++fix39_rope_stamp;
    for (unsigned i=0;i<12;i++) if (fix39_rope_cache_slots[i].a==a) {
        fix39_rope_cache_slots[i].stamp=fix39_rope_stamp;
        *pal=(uint16_t*)(void*)(fix39_rope_cache_slots[i].mem+a->palette_offset);
        return fix39_rope_cache_slots[i].mem;
    }
    unsigned pick=0; for(unsigned i=0;i<12;i++){ if(!fix39_rope_cache_slots[i].a){pick=i;break;} if(fix39_rope_cache_slots[i].stamp<fix39_rope_cache_slots[pick].stamp)pick=i; }
    fix39_rope_cache *c=&fix39_rope_cache_slots[pick]; free(c->mem); memset(c,0,sizeof(*c));
    c->bytes=(size_t)a->palette_offset+(size_t)a->palette_colors*2u; c->mem=malloc(c->bytes); if(!c->mem)return NULL;
    FILE *fp=fopen(a->path,"rb"); if(!fp){free(c->mem);memset(c,0,sizeof(*c));return NULL;}
    if(fread(c->mem,1,c->bytes,fp)!=c->bytes){fclose(fp);free(c->mem);memset(c,0,sizeof(*c));return NULL;} fclose(fp);
    data_cache_hit_writeback(c->mem,c->bytes); c->a=a; c->stamp=fix39_rope_stamp;
    *pal=(uint16_t*)(void*)(c->mem+a->palette_offset); return c->mem;
}
static void fix39_draw_rope_object(WmRopeBank bank,WmRopeChannel ch,WmRopeHalf half) {
    const WmRopeObjectSeed *s=wm_rope_object_seed(bank,ch,half); if(!s||!s->exists)return;
    const char *sym=wm_fix39_rope_image_symbol(bank,ch,half); const wm_ring_rope_asset *a=wm_ring_rope_asset_find(sym); if(!a)return;
    uint16_t *pal=NULL; const uint8_t *px=fix39_rope_load(a,&pal); if(!px||!pal)return;
    /* ROPES.ASM calls BEGINOBJ (not BEGINOBJW) after applying the literal
       +104/-258 object offsets. DISPLAY.ASM later subtracts WORLDTL and then
       subtracts the image ODOFF animation point. For M_FLIPH it first changes
       the X offset to (width - 1 - offset). Reproduce that sequence directly
       instead of relying on rdpq_tex_blit's centre/flip semantics. */
    const int obj_x=((int)(wm_rope_spawn_x_fp16(s)>>16))-(WM_RING_X_CENTER-200);
    const int obj_y=((int)(wm_rope_spawn_y_fp16(s)>>16))-(-27);
    const int xoff=s->flip_horizontal ? ((int)a->width-1-(int)a->xani) : (int)a->xani;
    const int yoff=(int)a->yani;
    const float sx=(float)(obj_x-xoff)*WM_FRONTEND_SCALE_X;
    const float sy=(float)(obj_y-yoff)*WM_FRONTEND_SCALE_Y;
    surface_t tex=surface_make_linear((void*)px,FMT_CI8,a->width,a->height);
    rdpq_set_mode_standard(); rdpq_mode_tlut(TLUT_RGBA16); rdpq_mode_filter(FILTER_POINT); rdpq_mode_alphacompare(1);
    rdpq_tex_upload_tlut(pal,0,a->palette_colors);
    int pitch=(a->width+7)&~7, sh=pitch?2048/pitch:1; if(sh<1)sh=1; if(sh>2)sh&=~1;
    for(int top=0;top<a->height;top+=sh){int h=a->height-top;if(h>sh)h=sh;
        rdpq_tex_blit(&tex,sx,sy+(float)top*WM_FRONTEND_SCALE_Y,&(rdpq_blitparms_t){.t0=top,.height=h,.flip_x=s->flip_horizontal,.scale_x=WM_FRONTEND_SCALE_X,.scale_y=WM_FRONTEND_SCALE_Y,.filtering=false});}
}
static void fix39_draw_rope_bank(WmRopeBank bank){
    for(unsigned ch=0;ch<WM_FIX39_ROPE_CHANNEL_COUNT;ch++) for(unsigned h=0;h<2;h++) fix39_draw_rope_object(bank,(WmRopeChannel)ch,(WmRopeHalf)h);
}
static void draw_ring_back(void) {
    draw_background();
    /* Mat/apron remain on the pre-existing path until the source BMOD arena
       database is translated; rope pixels themselves are no longer fabricated. */
    fill_rect(0,82,320,240,RGBA32(20,22,28,255));
    fill_rect(26,96,294,214,RGBA32(92,92,102,255));
    fill_rect(33,102,287,205,RGBA32(194,194,198,255));
    fill_rect(39,108,281,199,RGBA32(216,216,216,255));
    fix39_draw_rope_bank(WM_FIX39_ROPE_BACK);
    fix39_draw_rope_bank(WM_FIX39_ROPE_LEFT);
    fix39_draw_rope_bank(WM_FIX39_ROPE_RIGHT);
}
static void draw_ring_front(void) { fix39_draw_rope_bank(WM_FIX39_ROPE_FRONT); }
'''
t=t[:start]+code+'\n'+t[end:]
# Combat2AW: the all-roster renderer no longer calls the old Bret-only palette helper.
# Remove the now-dead static function so the N64 -Werror build remains warning-clean.
needle='static const wm_source_sprite *bret_object_palette'
if needle in t:
    hs=t.index(needle)
    he=t.index('\n}\n',hs)+3
    t=t[:hs]+t[he:]

p.write_text(t)
# generated metadata must be linked
for fn in ['CMakeLists.txt','Makefile']:
    q=repo/fn; s=q.read_text()
    if 'src/generated/ring_rope_assets.c' not in s:
        if fn=='CMakeLists.txt':
            a='    src/generated/character_assets.c\n'
            if a not in s: raise SystemExit('ring patch: CMake anchor missing')
            s=s.replace(a,a+'    src/generated/ring_rope_assets.c\n',1)
        else:
            a='src/generated/character_assets.c'
            if a not in s: raise SystemExit('ring patch: Makefile anchor missing')
            s=s.replace(a,a+' src/generated/ring_rope_assets.c',1)
    q.write_text(s)
print('Combat2AZ source-accurate rope ODOFF/flip projection patch applied')
