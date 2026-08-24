#!/usr/bin/env python3
from pathlib import Path
import sys
repo=Path(sys.argv[1])
p=repo/'src/platform/n64/main.c'
t=p.read_text()
old=r'''static void fix39_draw_arena_pass(bool front){
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
new=r'''/* Combat2BA: preserve the original shared OZPOS order from BAKGND.ASM
   #ztbl and ROPES.ASM instead of grouping every rope bank around a coarse
   back/front split.  The exact source Z values are:
     ring 100=13c7 mat, 101=13c8 back posts, 102=13c9 back buckles,
     side shadows=13c8, back/side-blue=13ca, side-white=13cb,
     side-red=13cc,
     ring 103=1500 front buckles, 104=1501 front posts,
     105=1502 front mat, front ropes=15aa, ring 106=1769 front gate.
   WRESTLE.ASM creates the ring module before the rope processes, so equal-Z
   rope objects are emitted after their ring-module peers. */
static void fix39_draw_arena_z(uint8_t source_z) {
    fix39_arena_order_init();
    if (!fix39_arena_order_ready) return;
    for (size_t oi=0; oi<211; ++oi) {
        const wm_ring_arena_block *b=wm_ring_arena_block_at(fix39_arena_order[oi]);
        if (b && b->z==source_z) fix39_draw_arena_block(b);
    }
}
static void fix39_draw_rope_channel(WmRopeBank bank,WmRopeChannel channel) {
    fix39_draw_rope_object(bank,channel,WM_FIX39_ROPE_HALF_FIRST);
    fix39_draw_rope_object(bank,channel,WM_FIX39_ROPE_HALF_SECOND);
}
static void fix39_draw_side_channel(WmRopeChannel channel) {
    fix39_draw_rope_channel(WM_FIX39_ROPE_LEFT,channel);
    fix39_draw_rope_channel(WM_FIX39_ROPE_RIGHT,channel);
}
static void fix39_draw_arena_through_back_posts(void) {
    /* Combat2BH: restore the exact source-ordered BAKLST traversal proven on
       hardware in AX/AY.  CROWD.ASM-controlled/background objects (raw OZPOS
       0..70) and ring #ztbl families 100/101 are one ordered list in Midway's
       renderer; do not split the crowd into an isolated pass.  Splitting that
       list in BA/BG left the hardware scene without the crowd/rail layer. */
    fix39_arena_order_init();
    if (!fix39_arena_order_ready) return;
    for (size_t oi=0; oi<211; ++oi) {
        const wm_ring_arena_block *b=wm_ring_arena_block_at(fix39_arena_order[oi]);
        if (b && b->z <= 101) fix39_draw_arena_block(b);
    }
}
static void draw_ring_back(void) {
    draw_background();
    /* BAKGND.ASM BAKLST is a single ordered list: crowd/background, mat and
       back posts.  Keep that traversal intact so the source crowd cannot be
       dropped between independent renderer passes. */
    fix39_draw_arena_through_back_posts(); /* Z 0..101 */
    fix39_draw_side_channel(WM_FIX39_ROPE_CHANNEL_SHADOW); /* 13c8 */
    fix39_draw_arena_z(102); /* 13c9 back buckles */
    fix39_draw_rope_channel(WM_FIX39_ROPE_BACK,WM_FIX39_ROPE_CHANNEL_RED);   /* 13ca */
    fix39_draw_rope_channel(WM_FIX39_ROPE_BACK,WM_FIX39_ROPE_CHANNEL_WHITE); /* 13ca */
    fix39_draw_rope_channel(WM_FIX39_ROPE_BACK,WM_FIX39_ROPE_CHANNEL_BLUE);  /* 13ca */
    fix39_draw_side_channel(WM_FIX39_ROPE_CHANNEL_BLUE);  /* 13ca */
    fix39_draw_side_channel(WM_FIX39_ROPE_CHANNEL_WHITE); /* 13cb */
    fix39_draw_side_channel(WM_FIX39_ROPE_CHANNEL_RED);   /* 13cc */
}
static void draw_ring_front(void) {
    /* BAKGND.ASM puts these three ring families before the 15aa front ropes,
       while the 1769 front gate is after them. */
    fix39_draw_arena_z(103); /* 1500 front buckles */
    fix39_draw_arena_z(104); /* 1501 front posts */
    fix39_draw_arena_z(105); /* 1502 front mat */
    fix39_draw_rope_channel(WM_FIX39_ROPE_FRONT,WM_FIX39_ROPE_CHANNEL_RED);
    fix39_draw_rope_channel(WM_FIX39_ROPE_FRONT,WM_FIX39_ROPE_CHANNEL_WHITE);
    fix39_draw_rope_channel(WM_FIX39_ROPE_FRONT,WM_FIX39_ROPE_CHANNEL_BLUE);
    fix39_draw_arena_z(106); /* 1769 front gate */
}'''
if old not in t:
    raise SystemExit('Combat2BA depth patch: Combat2AX/AZ arena split anchor missing')
t=t.replace(old,new,1)
p.write_text(t)
print('Combat2BH source-ordered BAKLST crowd + ring/rope OZPOS ordering patch applied')
