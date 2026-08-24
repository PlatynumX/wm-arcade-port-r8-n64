#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1]); p=repo/'src/platform/n64/main.c'; t=p.read_text()
anchor='#include "wm/ring_arena_assets.h"\n'
inc='#include "wm/crowd_assets.h"\n'
strinc='#include <string.h>\n'
if strinc not in t: t=strinc+t
if inc not in t:
    if anchor not in t: raise SystemExit('crowd patch: arena include anchor missing')
    t=t.replace(anchor,anchor+inc,1)
# Insert crowd runtime immediately before arena block renderer.
mark='static void fix39_draw_arena_block(const wm_ring_arena_block *b){'
if mark not in t: raise SystemExit('crowd patch: arena draw anchor missing')
code=r'''/* Combat2BC: CROWD.ASM crowd_anim translation.  BAKLST selects a crowd
   animator by (OZPOS >> 1), and only indices 0..29 are crowd-controlled.  The
   ring BMOD carries those literal Z values, so no new placement table is
   invented here. */
typedef struct { uint16_t script,pc,time,frame,repeat_pc,repeat_n; bool ready; } fix39_crowd_state;
static fix39_crowd_state fix39_crowd[30];
static bool fix39_crowd_ready;
static uint8_t *fix39_crowd_pixels[160];
static void fix39_crowd_init(void){
    if(fix39_crowd_ready)return;
    for(unsigned i=0;i<30;i++){
        const wm_crowd_person *p=wm_crowd_person_at(i); if(!p)continue;
        fix39_crowd[i].script=p->normal_script; fix39_crowd[i].ready=true;
    }
    fix39_crowd_ready=true;
}
static void fix39_crowd_tick_one(fix39_crowd_state *s){
    if(!s||!s->ready)return;
    if(s->time){--s->time; if(s->time)return;}
    for(unsigned guard=0;guard<32;guard++){
        const wm_crowd_script *sc=wm_crowd_script_at(s->script); if(!sc||!sc->count){s->ready=false;return;}
        if(s->pc>=sc->count)s->pc=0;
        const wm_crowd_cmd *c=&sc->cmd[s->pc++];
        switch((wm_crowd_op)c->op){
        case WM_CROWD_FRAME:s->frame=c->arg;s->time=c->value;return;
        case WM_CROWD_GOTO:s->script=c->arg;s->pc=0;continue;
        case WM_CROWD_RNDWAIT:s->time=(uint16_t)wm_fix39_rndrng0(c->value);return;
        case WM_CROWD_REPEAT:s->repeat_pc=s->pc;s->repeat_n=(uint16_t)wm_fix39_rndrng0(c->value);continue;
        case WM_CROWD_SHOULD_REPEAT:
            if(s->repeat_n>1){--s->repeat_n;s->pc=s->repeat_pc;} else s->repeat_n=0;
            continue;
        default:s->ready=false;return;
        }
    }
}
static void fix39_crowd_tick(void){fix39_crowd_init();for(unsigned i=0;i<30;i++)fix39_crowd_tick_one(&fix39_crowd[i]);}
/* CROWD.ASM DO_CROWD_CHEER: C_OVERRIDE|C_LONG, no random filter. */
static void fix39_crowd_do_crowd_cheer(void){
    fix39_crowd_init();
    for(unsigned i=0;i<30;i++){
        if(i==20u||i==21u||i==23u)continue;
        const wm_crowd_person *p=wm_crowd_person_at(i); if(!p)continue;
        fix39_crowd[i].script=p->cheer2_script; fix39_crowd[i].pc=0; fix39_crowd[i].time=1; fix39_crowd[i].ready=true;
        if(i==19u){
            const unsigned side[]={20u,21u,23u};
            for(unsigned k=0;k<3;k++){ unsigned j=side[k]; const wm_crowd_person *q=wm_crowd_person_at(j); if(q){ fix39_crowd[j].script=q->cheer1_script; fix39_crowd[j].pc=0; fix39_crowd[j].time=1; fix39_crowd[j].ready=true; } }
        }
    }
}
static bool fix39_crowd_cheer_frame(const char *f){
    static const char *const frames[]={
        "L3PN5B+FR8","L4SW5A+FR2","L4FX5B+FR1","S3PN5C+FR7","S1TT5Z+FR2","S4SW4A+FR1",
        "H3PN5A+FR8","H1TL5A+FR3","H4SL4C+FR1","B2PN5A+FR6","B1TT5Z+FR2","B4SW4B+FR3",
        "U5RV5A+FR4","U4PS3A+FR5","U5RV5A+FR6","U3PN5A+FR9","U1TT5A+FR2","U5RV5A+FR1",
        "D4PN5A+FR5","D1TT5Z+FR2","D5WN5B+FR2","Y3PF3C+FR12","Y1TT5Z+FR2","Y5RV5A+FR1",
        "R3PN5A+FR6","R1TT5Z+FR2","R4SW4D+FR3"
    };
    if(!f)return false;
    for(unsigned i=0;i<sizeof(frames)/sizeof(frames[0]);i++) if(strcmp(f,frames[i])==0) return true;
    return false;
}
static void fix39_crowd_source_frame_event(unsigned slot,const char *frame){
    static char last[2][32]; if(slot>=2)return;
    if(!frame)return;
    if(strncmp(last[slot],frame,sizeof(last[slot])-1)!=0){ strncpy(last[slot],frame,sizeof(last[slot])-1); last[slot][sizeof(last[slot])-1]=0; if(fix39_crowd_cheer_frame(frame))fix39_crowd_do_crowd_cheer(); }
}
static const uint8_t *fix39_crowd_load(uint16_t fi,uint16_t **pal){
    const wm_crowd_asset *a=wm_crowd_asset_at(fi); if(!a||!pal||fi>=160)return NULL;
    if(!fix39_crowd_pixels[fi]){
        const size_t bytes=(size_t)a->palette_offset+(size_t)a->palette_colors*2u; uint8_t *m=malloc(bytes); if(!m)return NULL;
        FILE *fp=fopen(a->path,"rb"); if(!fp){free(m);return NULL;} if(fread(m,1,bytes,fp)!=bytes){fclose(fp);free(m);return NULL;} fclose(fp);
        data_cache_hit_writeback(m,bytes); fix39_crowd_pixels[fi]=m;
    }
    *pal=(uint16_t*)(void*)(fix39_crowd_pixels[fi]+a->palette_offset); return fix39_crowd_pixels[fi];
}
static bool fix39_draw_crowd_block(const wm_ring_arena_block *b){
    if(!b || (b->z&1u) || b->z>58u)return false;
    const unsigned ci=(unsigned)b->z>>1; if(ci>=30)return false; fix39_crowd_init();
    const fix39_crowd_state *s=&fix39_crowd[ci];
    /* CROWD.ASM mutates an existing BAKLST object in place. If the translated
       dynamic frame is not available, leave that object on its original BMOD
       image instead of deleting it from the scene. */
    const wm_crowd_asset *a=wm_crowd_asset_at(s->frame); if(!a)return false;
    const wm_crowd_person *person=wm_crowd_person_at(ci); const wm_crowd_script *norm=person?wm_crowd_script_at(person->normal_script):NULL;
    int ix=0,iy=0;
    if(norm){for(unsigned j=0;j<norm->count;j++)if(norm->cmd[j].op==WM_CROWD_FRAME){const wm_crowd_asset *ia=wm_crowd_asset_at(norm->cmd[j].arg);if(ia){ix=ia->xani;iy=ia->yani;}break;}}
    /* CROWD.ASM anibobj preserves the animation point by moving OXPOS/OYPOS
       by old IANI - new IANI. BMOD x/y are the source object's initial pos. */
    int x=(int)b->x+WM_RING_MODULE_X-WM_RING_WORLD_TL_X + ix-(int)a->xani;
    int y=(int)b->y+WM_RING_MODULE_Y-WM_RING_WORLD_TL_Y + iy-(int)a->yani;
    uint16_t *pal=NULL; const uint8_t *px=fix39_crowd_load(s->frame,&pal);
    if(!px||!pal)return false;
    surface_t tex=surface_make_linear((void*)px,FMT_CI8,a->width,a->height);
    rdpq_set_mode_standard();rdpq_mode_tlut(TLUT_RGBA16);rdpq_mode_filter(FILTER_POINT);rdpq_mode_alphacompare(1);rdpq_tex_upload_tlut(pal,0,a->palette_colors);
    int pitch=(a->width+7)&~7,sh=pitch?2048/pitch:1;if(sh<1)sh=1;if(sh>2)sh&=~1;
    const bool fx=(b->flags&WM_BMOD_HFLIP)!=0;
    for(int top=0;top<(int)a->height;top+=sh){int h=(int)a->height-top;if(h>sh)h=sh;rdpq_tex_blit(&tex,(float)x*WM_FRONTEND_SCALE_X,(float)(y+top)*WM_FRONTEND_SCALE_Y,&(rdpq_blitparms_t){.t0=top,.height=h,.flip_x=fx,.scale_x=WM_FRONTEND_SCALE_X,.scale_y=WM_FRONTEND_SCALE_Y,.filtering=false});}
    return true;
}
'''
t=t.replace(mark,code+'\n'+mark,1)
# CROWD.ASM animates the existing BAKLST object in place by replacing its
# image while retaining the object's BAKLST ordering.  Render the translated
# frame at that exact object slot; if its source frame cannot be loaded, fall
# through to the original ringBMOD image as the hardware-safe fallback.
body='static void fix39_draw_arena_block(const wm_ring_arena_block *b){\n    if (!b) return;'
repl='static void fix39_draw_arena_block(const wm_ring_arena_block *b){\n    if (!b) return;\n    if (fix39_draw_crowd_block(b)) return;'
if body not in t: raise SystemExit('crowd patch: arena block body anchor missing')
t=t.replace(body,repl,1)
# Combat2BG safety correction: CROWD.ASM mutates existing BAKLST objects; it does not
# remove their initial BMOD image first. The previous replacement hook could suppress
# every source crowd block if the translated dynamic draw was not hardware-correct.
# Keep the source ringBMOD crowd object visible until the in-place object mutation is
# translated exactly. Dynamic CROWD.ASM state/data remain generated and ticking.

# Source ANI_CODE,DO_CROWD_CHEER sites are bound to the next source frame.
rm='static __attribute__((unused)) void render_match(const wm_app *app) {'
ri=t.find(rm)
if ri<0: raise SystemExit('crowd patch: render_match missing')
rb=t.find('{',ri); dep=0; re_=None
for ii in range(rb,len(t)):
    if t[ii]=='{': dep+=1
    elif t[ii]=='}':
        dep-=1
        if dep==0: re_=ii+1; break
if re_ is None: raise SystemExit('crowd patch: render_match close missing')
seg=t[ri:re_]
d='const wm_demo *demo = &app->demo;'
if d not in seg: raise SystemExit('crowd patch: demo declaration missing')
if 'fix39_crowd_source_frame_event(0' not in seg:
    h='\n    { const wm_visual_frame *c0=wm_visual_current(&demo->p1.visual); const wm_visual_frame *c1=wm_visual_current(&demo->p2.visual); fix39_crowd_source_frame_event(0,c0?c0->source_frame:NULL); fix39_crowd_source_frame_event(1,c1?c1->source_frame:NULL); }'
    seg=seg.replace(d,d+h,1)
    t=t[:ri]+seg+t[re_:]

# Tick once per rendered match frame, before the background pass traverses BAKLST-equivalent blocks.
old='''static void draw_ring_back(void) {\n    draw_background();'''
new='''static void draw_ring_back(void) {\n    fix39_crowd_tick();\n    draw_background();'''
if old not in t: raise SystemExit('crowd patch: draw_ring_back anchor changed')
t=t.replace(old,new,1)
p.write_text(t)
for fn in ['CMakeLists.txt','Makefile']:
    q=repo/fn;s=q.read_text()
    if 'src/generated/crowd_assets.c' not in s:
        if fn=='CMakeLists.txt':
            a='    src/generated/ring_arena_assets.c\n';
            if a not in s: raise SystemExit('crowd patch: CMake arena anchor missing')
            s=s.replace(a,a+'    src/generated/crowd_assets.c\n',1)
        else:
            a='src/generated/ring_arena_assets.c'
            if a not in s: raise SystemExit('crowd patch: Make arena anchor missing')
            s=s.replace(a,a+' src/generated/crowd_assets.c',1)
    q.write_text(s)
print('Combat2BL CROWD.ASM in-place BAKLST image mutation enabled with ringBMOD fallback')
