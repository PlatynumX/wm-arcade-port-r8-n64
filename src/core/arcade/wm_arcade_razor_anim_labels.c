#include "wm/wrestler_backend.h"

/*
 * Razor's animation ids, as the source's own routine names.
 *
 * He is the one wrestler whose dispatcher selects animations by a typed id
 * rather than by label (wm_arcade_razor_callbacks_t against
 * wm_arcade_roster_callbacks_t), so where the other six needed nothing but
 * the program registry, he needs this map. Every row was checked against
 * RZRSEQ1-4.ASM: the routine exists, and it emits as a program.
 *
 * Three could not be derived from the id name and were read out of the
 * source instead:
 *
 *   UPPERCUT4  RAZOR.ASM:1565 #ck_up loads rzr_4_uprcut_anim -- "uprcut",
 *              not "uppercut", and nothing else in RZRSEQ matches
 *   KICK_TB    rzr_kick_TB_anim keeps the source's capital TB, exactly as
 *              Bret's hrt_kick_TB_anim does
 *   START_RUN  start_run_anim, the shared WRESTLE2.ASM routine every
 *              wrestler runs -- it has no frames of its own, so the
 *              backend runs its state setup rather than a program
 *
 * WM_RZR_ANIM_FINISH1 and FINISH2 are deliberately absent. Their routines
 * exist -- FINISEQ.ASM:259/277 rzr_finish1_move and rzr_finish2_move -- but
 * both are eight lines of commands ending in ANI_END behind a
 * `.if NUM_RAZOR_FINISHES` guard, with no WL frames at all. There is no
 * animation there to map, in the original either.
 */
const char *wm_arcade_razor_anim_label(wm_arcade_razor_anim_id_t id) {
    switch (id) {
        case WM_RZR_ANIM_BIGBOOT4: return "rzr_4_bigboot_anim";
        case WM_RZR_ANIM_BLOCK2: return "rzr_2_block_anim";
        case WM_RZR_ANIM_BLOCK4: return "rzr_4_block_anim";
        case WM_RZR_ANIM_BUTT2: return "rzr_2_butt_anim";
        case WM_RZR_ANIM_BUTT4: return "rzr_4_butt_anim";
        case WM_RZR_ANIM_CLIMB_DOWN: return "rzr_climb_down_anim";
        case WM_RZR_ANIM_COMBO_KICK: return "rzr_combo_kick_anim";
        case WM_RZR_ANIM_COMBO_PUNCH: return "rzr_combo_punch_anim";
        case WM_RZR_ANIM_DSLASH3: return "rzr_3_dslash_anim";
        case WM_RZR_ANIM_DSLASHES_TO_HEAD: return "rzr_dslashes_to_head_anim";
        case WM_RZR_ANIM_FAKE_HOLD3: return "rzr_3_fake_hold_anim";
        case WM_RZR_ANIM_FALL_BACK: return "rzr_fall_back_anim";
        case WM_RZR_ANIM_FLYING_ELBOW: return "rzr_flying_elbow_anim";
        case WM_RZR_ANIM_FLYING_KICK: return "rzr_flying_kick_anim";
        case WM_RZR_ANIM_GRABFLING2: return "rzr_2_grabfling_anim";
        case WM_RZR_ANIM_GRABFLING4: return "rzr_4_grabfling_anim";
        case WM_RZR_ANIM_GROUND_PUNCH2: return "rzr_2_ground_punch_anim";
        case WM_RZR_ANIM_GROUND_PUNCH4: return "rzr_4_ground_punch_anim";
        case WM_RZR_ANIM_HAIR_PICKUP2: return "rzr_2_hair_pickup_anim";
        case WM_RZR_ANIM_HAIR_PICKUP4: return "rzr_4_hair_pickup_anim";
        case WM_RZR_ANIM_HEAD_HELD_STAND3: return "rzr_3_head_held_stand_anim";
        case WM_RZR_ANIM_HEAD_HOLD2_3: return "rzr_3_head_hold2_anim";
        case WM_RZR_ANIM_HEAD_HOLD3: return "rzr_3_head_hold_anim";
        case WM_RZR_ANIM_HIPTOSS2: return "rzr_2_hiptoss_anim";
        case WM_RZR_ANIM_HIPTOSS2_2: return "rzr_2_hiptoss2_anim";
        case WM_RZR_ANIM_HIPTOSS2_4: return "rzr_4_hiptoss2_anim";
        case WM_RZR_ANIM_HIPTOSS4: return "rzr_4_hiptoss_anim";
        case WM_RZR_ANIM_KICK2: return "rzr_2_kick_anim";
        case WM_RZR_ANIM_KICK2_4: return "rzr_4_kick2_anim";
        case WM_RZR_ANIM_KICK4: return "rzr_4_kick_anim";
        case WM_RZR_ANIM_KICK_TB: return "rzr_kick_TB_anim";
        case WM_RZR_ANIM_KNEE2: return "rzr_2_knee_anim";
        case WM_RZR_ANIM_KNEE4: return "rzr_4_knee_anim";
        case WM_RZR_ANIM_KNEE_FALL4: return "rzr_4_knee_fall_anim";
        case WM_RZR_ANIM_PILE_DRIVER3: return "rzr_3_pile_driver_anim";
        case WM_RZR_ANIM_PIN2: return "rzr_2_pin_anim";
        case WM_RZR_ANIM_PIN4: return "rzr_4_pin_anim";
        case WM_RZR_ANIM_PUMMEL2: return "rzr_2_pummel_anim";
        case WM_RZR_ANIM_PUMMEL4: return "rzr_4_pummel_anim";
        case WM_RZR_ANIM_PUNCH2: return "rzr_2_punch_anim";
        case WM_RZR_ANIM_PUNCH4: return "rzr_4_punch_anim";
        case WM_RZR_ANIM_PUSH4: return "rzr_4_push_anim";
        case WM_RZR_ANIM_RAISE_ARM2: return "rzr_2_raise_arm_anim";
        case WM_RZR_ANIM_RAISE_ARM4: return "rzr_4_raise_arm_anim";
        case WM_RZR_ANIM_RAZORS_EDGE: return "rzr_razors_edge_anim";
        case WM_RZR_ANIM_REPEAT_SLASH: return "rzr_repeat_slash_anim";
        case WM_RZR_ANIM_RUGSHAKE: return "rzr_rugshake_anim";
        case WM_RZR_ANIM_RUGSHAKE2: return "rzr_rugshake2_anim";
        case WM_RZR_ANIM_RUN2: return "rzr_run2_anim";
        case WM_RZR_ANIM_SLIDING_RUG: return "rzr_sliding_rug_anim";
        case WM_RZR_ANIM_STAND2: return "rzr_stand2_anim";
        case WM_RZR_ANIM_STAND4: return "rzr_stand4_anim";
        case WM_RZR_ANIM_START_RUN: return "start_run_anim";
        case WM_RZR_ANIM_STOMP2: return "rzr_2_stomp_anim";
        case WM_RZR_ANIM_STOMP4: return "rzr_4_stomp_anim";
        case WM_RZR_ANIM_SUPER_KICK2: return "rzr_2_super_kick_anim";
        case WM_RZR_ANIM_SUPER_KICK4: return "rzr_4_super_kick_anim";
        case WM_RZR_ANIM_TBUKL_ELBOW: return "rzr_tbukl_elbow_anim";
        case WM_RZR_ANIM_TORSO2: return "rzr_torso2_anim";
        case WM_RZR_ANIM_TORSO4: return "rzr_torso4_anim";
        case WM_RZR_ANIM_UPPERCUT4: return "rzr_4_uprcut_anim";
        case WM_RZR_ANIM_USLASH3: return "rzr_3_uslash_anim";
        case WM_RZR_ANIM_USLASHES_TO_HEAD: return "rzr_uslashes_to_head_anim";
        default: return 0;
    }
}
