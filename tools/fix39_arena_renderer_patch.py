#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1])
p=repo/'src/platform/n64/main.c'; t=p.read_text()
inc='#include "wm/ring_arena_assets.h"\n'
anchor='#include "wm/ring_rope_assets.h"\n'
if inc not in t:
    if anchor not in t: raise SystemExit('arena patch: rope include anchor missing')
    t=t.replace(anchor,anchor+inc,1)
start=t.index('static void draw_ring_back(void) {')
front=t.index('static void draw_ring_front(void)',start)
brace=t.index('{',front)
depth=0; front_end=None
for i in range(brace,len(t)):
    if t[i]=='{': depth+=1
    elif t[i]=='}':
        depth-=1
        if depth==0:
            front_end=i+1
            break
if front_end is None: raise SystemExit('arena patch: draw_ring_front closing brace missing')
code=r'''/* Combat2AX: exact ringBMOD renderer translated from the Midway background
   object path.  Source authority is BGNDTBL.ASM ringBMOD/ringBLKS plus the
   selected NEWRINGB.BDD image database.  WRESTLE.ASM ring_mod starts the
   module at (105,-450); init_scroller starts WORLDTL at
   (RING_X_CENTER-200,-27) for normal 1v1. */
#define WM_RING_MODULE_X 105
#define WM_RING_MODULE_Y (-450)
#define WM_RING_WORLD_TL_X ((0x400 + 50) - 200)
#define WM_RING_WORLD_TL_Y (-27)
#define WM_RING_SOURCE_VIEW_W 400
#define WM_RING_SOURCE_VIEW_H 256
#define WM_RING_ARENA_IMAGE_COUNT 95
static uint8_t *fix39_arena_pixels[WM_RING_ARENA_IMAGE_COUNT];
static size_t fix39_arena_pixel_bytes[WM_RING_ARENA_IMAGE_COUNT];
static uint16_t fix39_arena_order[211];
static bool fix39_arena_order_ready;
static const uint8_t *fix39_arena_image_load(uint16_t header_index) {
    if (header_index >= WM_RING_ARENA_IMAGE_COUNT) return NULL;
    if (fix39_arena_pixels[header_index]) return fix39_arena_pixels[header_index];
    const wm_ring_arena_image *a = wm_ring_arena_image_at(header_index);
    if (!a) return NULL;
    const size_t bytes=(size_t)a->width*(size_t)a->height;
    uint8_t *mem = malloc(bytes);
    if (!mem) return NULL;
    FILE *fp = fopen(a->path, "rb");
    if (!fp) { free(mem); return NULL; }
    if (fread(mem, 1, bytes, fp) != bytes) {
        fclose(fp);
        free(mem);
        return NULL;
    }
    fclose(fp);
    data_cache_hit_writeback(mem,bytes);
    fix39_arena_pixels[header_index] = mem;
    fix39_arena_pixel_bytes[header_index] = bytes;
    return mem;
}
static bool fix39_arena_draw_before(const wm_ring_arena_block *a,const wm_ring_arena_block *b){
    wm_bmod_block aa={a->palette,a->flags,a->z,a->x,a->y,a->header_index};
    wm_bmod_block bb={b->palette,b->flags,b->z,b->x,b->y,b->header_index};
    return wm_bmod_draw_before(&aa,&bb);
}
static void fix39_arena_order_init(void){
    if (fix39_arena_order_ready) return;
    const size_t n = wm_ring_arena_block_count();
    if (n != 211) return;
    for(size_t i=0;i<n;i++)fix39_arena_order[i]=(uint16_t)i;
    for(size_t i=1;i<n;i++){
        uint16_t key = fix39_arena_order[i];
        size_t j = i;
        const wm_ring_arena_block *kb=wm_ring_arena_block_at(key);
        while (j > 0) {
            const wm_ring_arena_block *pb = wm_ring_arena_block_at(fix39_arena_order[j - 1]);
            if (!fix39_arena_draw_before(kb, pb)) break;
            fix39_arena_order[j] = fix39_arena_order[j - 1];
            --j;
        }
        fix39_arena_order[j]=key;
    }
    fix39_arena_order_ready=true;
}
static void fix39_draw_arena_block(const wm_ring_arena_block *b){
    if (!b) return;
    const wm_ring_arena_image *img = wm_ring_arena_image_at(b->header_index);
    const wm_ring_arena_palette *pal = wm_ring_arena_palette_at(b->palette);
    if (!img || !pal || !pal->color_count) return;
    const int sx=(int)b->x+WM_RING_MODULE_X-WM_RING_WORLD_TL_X;
    const int sy=(int)b->y+WM_RING_MODULE_Y-WM_RING_WORLD_TL_Y;
    if (sx + (int)img->width < 0 || sx >= WM_RING_SOURCE_VIEW_W ||
        sy + (int)img->height < 0 || sy >= WM_RING_SOURCE_VIEW_H) return;
    const uint8_t *px = fix39_arena_image_load(b->header_index);
    if (!px) return;
    const bool transparent=(b->flags&WM_BMOD_TRANSPARENT)!=0;
    uint16_t *tlut=transparent?pal->rgba5551_keyed:pal->rgba5551_opaque;
    surface_t tex=surface_make_linear((void*)px,FMT_CI8,img->width,img->height);
    rdpq_set_mode_standard(); rdpq_mode_tlut(TLUT_RGBA16); rdpq_mode_filter(FILTER_POINT); rdpq_mode_alphacompare(1); rdpq_tex_upload_tlut(tlut,0,pal->color_count);
    int pitch = (img->width + 7) & ~7;
    int sh = pitch ? 2048 / pitch : 1;
    if (sh < 1) sh = 1;
    if (sh > 2) sh &= ~1;
    const bool fx=(b->flags&WM_BMOD_HFLIP)!=0, fy=(b->flags&WM_BMOD_VFLIP)!=0;
    for (int top = 0; top < (int)img->height; top += sh) {
        int h = (int)img->height - top;
        if (h > sh) h = sh;
        int dr = fy ? (int)img->height - top - h : top;
        rdpq_tex_blit(&tex,(float)sx*WM_FRONTEND_SCALE_X,(float)(sy+dr)*WM_FRONTEND_SCALE_Y,&(rdpq_blitparms_t){.t0=top,.height=h,.flip_x=fx,.flip_y=fy,.scale_x=WM_FRONTEND_SCALE_X,.scale_y=WM_FRONTEND_SCALE_Y,.filtering=false});}
}
static void fix39_draw_arena_pass(bool front){
    fix39_arena_order_init();
    if (!fix39_arena_order_ready) return;
    for(size_t oi=0;oi<211;oi++){
        const wm_ring_arena_block *b = wm_ring_arena_block_at(fix39_arena_order[oi]);
        if (!b) continue;
        /* BAKGND.ASM #ztbl is the source split around wrestlers:
           100 mat, 101 back posts, 102 back buckles,
           103 front buckles, 104 front posts, 105 front mat, 106 front gate. */
        if (front ? (b->z < 103) : (b->z >= 103)) continue;
        fix39_draw_arena_block(b);
    }
}
static void draw_ring_back(void) {
    draw_background();
    fix39_draw_arena_pass(false);
    fix39_draw_rope_bank(WM_FIX39_ROPE_BACK);
    fix39_draw_rope_bank(WM_FIX39_ROPE_LEFT);
    fix39_draw_rope_bank(WM_FIX39_ROPE_RIGHT);
}
static void draw_ring_front(void) {
    fix39_draw_rope_bank(WM_FIX39_ROPE_FRONT);
    fix39_draw_arena_pass(true);
}'''
t=t[:start]+code+t[front_end:]
p.write_text(t)
for fn in ['CMakeLists.txt','Makefile']:
    q=repo/fn; s=q.read_text()
    if 'src/generated/ring_arena_assets.c' not in s:
        if fn=='CMakeLists.txt':
            a='    src/generated/ring_rope_assets.c\n';
            if a not in s: raise SystemExit('arena patch: CMake rope source anchor missing')
            s=s.replace(a,a+'    src/generated/ring_arena_assets.c\n',1)
        else:
            a='src/generated/ring_rope_assets.c'
            if a not in s: raise SystemExit('arena patch: Makefile rope source anchor missing')
            s=s.replace(a,a+' src/generated/ring_arena_assets.c',1)
    q.write_text(s)
print('Combat2AX exact ringBMOD renderer patch applied')
