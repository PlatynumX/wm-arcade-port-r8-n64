#ifndef WM_BRET_VISUALS_H
#define WM_BRET_VISUALS_H

#include "wm/visual.h"

/* Movement / stance streams from HRTSEQ1.ASM. */
extern const wm_visual_sequence wm_bret_stand2_anim;
extern const wm_visual_sequence wm_bret_stand4_anim;
extern const wm_visual_sequence wm_bret_torso2_anim;
extern const wm_visual_sequence wm_bret_torso4_anim;
/* HRTSEQ1.ASM:104,116: hrt_torso8_anim/hrt_torso6_anim are literal SUBR
   aliases for hrt_torso2_anim/hrt_torso4_anim (same address, no code in
   between) -- these extract byte-identical frame data, not new artwork. */
extern const wm_visual_sequence wm_bret_torso6_anim;
extern const wm_visual_sequence wm_bret_torso8_anim;
/* All 12 hrt_walkM_fF_anim leg-cycle sequences BRET.ASM:2897
   hrt_leg_anims_table can select (M=1/2/4/5/6/8 move compass,
   F=2/4 facing bank). */
extern const wm_visual_sequence wm_bret_walk1_f2_anim;
extern const wm_visual_sequence wm_bret_walk1_f4_anim;
extern const wm_visual_sequence wm_bret_walk2_f2_anim;
extern const wm_visual_sequence wm_bret_walk2_f4_anim;
extern const wm_visual_sequence wm_bret_walk4_f2_anim;
extern const wm_visual_sequence wm_bret_walk4_f4_anim;
extern const wm_visual_sequence wm_bret_walk5_f2_anim;
extern const wm_visual_sequence wm_bret_walk5_f4_anim;
extern const wm_visual_sequence wm_bret_walk6_f2_anim;
extern const wm_visual_sequence wm_bret_walk6_f4_anim;
extern const wm_visual_sequence wm_bret_walk8_f2_anim;
extern const wm_visual_sequence wm_bret_walk8_f4_anim;
extern const wm_visual_sequence wm_bret_run_anim;

/*
 * BRET.ASM:2871 hrt_rotate_anims_table's 12 off-diagonal turn-transition
 * entries (the "TURNS (STANDS)" block, HRTSEQ1.ASM:390-454) -- played by
 * set_rotate_anim/change_anim1 for the idle (#zip/stance) facing-change
 * case, indexed by (old FACING_DIR diag, new NEW_FACING_DIR diag). Each of
 * the table's 12 off-diagonal cells is a literal SUBR alias of one of these
 * 6 (HRTSEQ1.ASM pairs each with its reverse-rotation twin at the same
 * address, e.g. hrt_8_to_6_turn_anim is hrt_2_to_4_turn_anim): only the
 * "old diag < new diag, going through the lower index" direction of each
 * pair is extracted here -- see wm_bret_rotate_anim's own comment in
 * src/core/bret_backend.c for which symbol backs which cell. */
extern const wm_visual_sequence wm_bret_2_to_4_turn_anim;
extern const wm_visual_sequence wm_bret_4_to_2_turn_anim;
extern const wm_visual_sequence wm_bret_4_to_6_turn_anim;
extern const wm_visual_sequence wm_bret_2_to_8_turn_anim;
extern const wm_visual_sequence wm_bret_4_to_8_turn_anim;
extern const wm_visual_sequence wm_bret_2_to_6_turn_anim;

/*
 * BRET.ASM:2981 hrt_torso_anims_table's 12 off-diagonal turn-transition
 * entries (the "TURNS (TORSOS)" block, HRTSEQ1.ASM:456-524) -- played by
 * change_walk_anim's torso half while actually walking with FACING_DIR
 * lagging NEW_FACING_DIR. Each carries one or two real ANI_SETFACING
 * commands (HRTSEQ1.ASM's own "<- not primary anim" comment: this is the
 * torso track's equivalent of the leg track's ANI_XFLIP at the same
 * structural point) -- see wm_bret_torso_turn_setfacing_frames in
 * src/core/bret_backend.c for the hand-traced 0-based frame indices those
 * fire at. Same reverse-rotation SUBR aliasing as the stand-turn table
 * above (e.g. hrt_8_to_6_turn2_anim is hrt_2_to_4_turn2_anim). */
extern const wm_visual_sequence wm_bret_2_to_4_turn2_anim;
extern const wm_visual_sequence wm_bret_4_to_2_turn2_anim;
extern const wm_visual_sequence wm_bret_4_to_6_turn2_anim;
extern const wm_visual_sequence wm_bret_2_to_8_turn2_anim;
extern const wm_visual_sequence wm_bret_4_to_8_turn2_anim;
extern const wm_visual_sequence wm_bret_2_to_6_turn2_anim;

/* Four distinct base attack buttons from HRTSEQ2.ASM. */
extern const wm_visual_sequence wm_bret_light_punch2_anim;
extern const wm_visual_sequence wm_bret_light_punch4_anim;
extern const wm_visual_sequence wm_bret_power_punch_anim;
extern const wm_visual_sequence wm_bret_light_kick2_anim;
extern const wm_visual_sequence wm_bret_light_kick4_anim;
extern const wm_visual_sequence wm_bret_power_kick_anim;

#endif
