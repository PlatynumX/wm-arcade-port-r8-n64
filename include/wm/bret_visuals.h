#ifndef WM_BRET_VISUALS_H
#define WM_BRET_VISUALS_H

#include "wm/visual.h"

/* Movement / stance streams from HRTSEQ1.ASM. */
extern const wm_visual_sequence wm_bret_stand2_anim;
extern const wm_visual_sequence wm_bret_stand4_anim;
extern const wm_visual_sequence wm_bret_torso2_anim;
extern const wm_visual_sequence wm_bret_torso4_anim;
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

/* Four distinct base attack buttons from HRTSEQ2.ASM. */
extern const wm_visual_sequence wm_bret_light_punch2_anim;
extern const wm_visual_sequence wm_bret_light_punch4_anim;
extern const wm_visual_sequence wm_bret_power_punch_anim;
extern const wm_visual_sequence wm_bret_light_kick2_anim;
extern const wm_visual_sequence wm_bret_light_kick4_anim;
extern const wm_visual_sequence wm_bret_power_kick_anim;

#endif
