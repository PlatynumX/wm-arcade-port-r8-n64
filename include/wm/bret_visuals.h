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
 * HRTSEQ4.ASM:104 hrt_4_block_anim. The 2-facing twin (hrt_2_block_anim,
 * HRTSEQ4.ASM:68) is entirely commented out in the source, so BLOCK4 is
 * the only real block animation Bret has -- matching his own do_block,
 * which only ever selects WM_BRET_ANIM_BLOCK4.
 *
 * This one carries three real ANIM.ASM commands the backend executes at
 * their traced positions rather than as flat frames: ANI_SETPLYRMODE,
 * MODE_BLOCK before frame 0, ANI_WAITRELEASE,PLAYER_BLOCK_BIT parked on
 * frame 1, and ANI_SETPLYRMODE,MODE_NORMAL after frame 2 -- see
 * wm_bret_backend_change_anim/tick.
 */
extern const wm_visual_sequence wm_bret_block4_anim;

/*
 * Batch 1 of Bret's remaining strike set (HRTSEQ2.ASM), each one an id his
 * own mode_normal already selected but which resolved to nothing: the
 * close-range headbutt do_punch falls back to, the close-range knee
 * do_kick falls back to, and the crouching uppercut do_super_punch selects
 * on a down input. Attack windows traced with tools/wlattack.py.
 */
extern const wm_visual_sequence wm_bret_butt2_anim;
extern const wm_visual_sequence wm_bret_butt4_anim;
extern const wm_visual_sequence wm_bret_knee2_anim;
extern const wm_visual_sequence wm_bret_knee4_anim;
extern const wm_visual_sequence wm_bret_uppercut4_anim;

/*
 * Batch 2 (HRTSEQ2.ASM): the two ids do_kick selects on a grounded
 * opponent inside near(160,140), and the two do_punch selects on the same
 * test. These are the first wired animations that carry more than one real
 * ANI_ATTACK_ON pulse each -- stomp two (an AMODE_HITCHECK probe then the
 * AMODE_STOMP2 hit), ground punch three (a probe then two separate
 * AMODE_LBOWDROP2 elbow drops) -- which is why src/core/bret_backend.c's
 * attack window table is keyed on (id, frame) rather than id alone. Their
 * headers also add a real MODE_OVERLAP the other wired attacks don't have.
 */
extern const wm_visual_sequence wm_bret_stomp2_anim;
extern const wm_visual_sequence wm_bret_stomp4_anim;
extern const wm_visual_sequence wm_bret_ground_punch2_anim;
extern const wm_visual_sequence wm_bret_ground_punch4_anim;

/*
 * Batch 3 (HRTSEQ2.ASM / HRTSEQ3.ASM). Picked by tools/wlattack.py --audit
 * rather than by eye: of every remaining animation Bret's dispatcher can
 * select, these are the ones whose control flow a flat wlanim.py --slice
 * genuinely represents. mode_block's shove-off-a-blocker and its
 * down-input branch select push4; wm_arcade_bret_fire_secret selects
 * jump_kick4; do_kick's own "stick matches facing" branch selects
 * knee_fall4; do_kick selects kick_tb against an INAIR2 (turnbuckle-leaping)
 * opponent; and mode_headhold selects head_held_stand3 once the grapple
 * has no attached process left.
 */
extern const wm_visual_sequence wm_bret_push4_anim;
extern const wm_visual_sequence wm_bret_jump_kick4_anim;
extern const wm_visual_sequence wm_bret_knee_fall4_anim;
extern const wm_visual_sequence wm_bret_kick_tb_anim;
extern const wm_visual_sequence wm_bret_head_held_stand3_anim;

/*
 * HRTSEQ2.ASM:618/677 hrt_2/4_super_punch2_anim -- BRET.ASM #spunch_slap's
 * own `FACE24 hrt,super_punch2_anim` pair, i.e. what do_super_punch
 * actually selects in ordinary play. Distinct animations from
 * wm_bret_power_punch_anim (hrt_4_super_punch_anim, HRTSEQ2.ASM:223),
 * which is #scrt_cut's supercut target: different frames, and a different
 * real attack box (AMODE_URN,19,75,35,24 at frame 3 versus
 * AMODE_UPRCUT,-6,40,64,90 at frame 5). The port previously mapped both to
 * the latter.
 */
extern const wm_visual_sequence wm_bret_super_punch2_2_anim;
extern const wm_visual_sequence wm_bret_super_punch2_4_anim;

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
