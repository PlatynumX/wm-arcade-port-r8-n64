#ifndef WM_ARCADE_COMBAT_H
#define WM_ARCADE_COMBAT_H

#include <stddef.h>
#include <stdint.h>
#include "wm/arcade/wm_arcade_combat_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wm_arcade_box3 {
    int32_t x1, x2;
    int32_t y1, y2;
    int32_t z1, z2;
} wm_arcade_box3_t;

typedef struct wm_arcade_frame_box {
    int32_t iani3x;
    int32_t iani3y;
    int32_t iani3z;
    int32_t iani3id;
} wm_arcade_frame_box_t;

typedef struct wm_arcade_actor wm_arcade_actor_t;

/*
 * Merge adapter only.  It names original arcade process fields in portable C;
 * it is not intended to replace the N64 port's wrestler structure.
 */
struct wm_arcade_actor {
    int active;

    int32_t x_int;
    int32_t y_int;
    int32_t z_int;
    /* Full 16.16 positions used by attachment animation commands. */
    int32_t x_fixed;
    int32_t y_fixed;
    int32_t z_fixed;

    uint16_t obj_control;
    uint16_t player_mode;
    uint16_t attack_mode;
    uint16_t anim_mode;
    uint32_t status_flags;

    int32_t move_dir;
    /* Stage 7 / REACT5: directional and input state used by running/hiptoss reactions. */
    int32_t facing_dir;
    int32_t new_facing_dir;
    uint16_t stick_val_cur;
    int32_t immobilize_time;
    int32_t combo_count;
    /* Stage 4: REACT2 combo-uppercut reads WHOHITME->RPT_COUNT. */
    int32_t rpt_count;
    /*
     * PLYR.EQU:103 INRING, WITH THE POLARITY INVERTED. The source's own
     * comment there reads "0 = in ring, 1 = outside"; this port stores the
     * ordinary boolean instead -- non-zero means IN the ring -- which is
     * what wm_match_start's own `a->in_ring = 1` and every reader here
     * already assume. Translating a source routine that tests INRING means
     * flipping the test, not copying it.
     */
    int32_t in_ring;
    /* WRESTLE.ASM PLYR.EQU CAN_MOVE_DIR: WM_MOVE_* bits the wrestler is
       currently confined against (real, see wm/arcade/wm_arcade_confine.h). */
    int32_t can_move_dir;

    wm_arcade_actor_t *attach_proc;
    wm_arcade_actor_t *smart_target;
    wm_arcade_actor_t *who_i_hit;
    wm_arcade_actor_t *who_hit_me;

    wm_arcade_box3_t hurt_box;

    int32_t attack_xoff;
    int32_t attack_yoff;
    int32_t attack_zoff;
    int32_t attack_width;
    int32_t attack_height;
    int32_t attack_depth;
    wm_arcade_box3_t attack_box;

    int32_t hit_blocker;
    int32_t hit_side;
    int32_t ani_count;
    int32_t ani_count2;

    int32_t getup_time;
    int32_t delay_meter;
    int32_t safe_time;
    int32_t dizzy;
    uint32_t head_grab_time;
    void *meter_proc;
    int32_t wrestler_num;
    int32_t player_num;

    /* Stage 2: REACT1.ASM / ANIM.ASM combat state. */
    uint32_t last_hit_time;
    uint16_t last_damage;          /* source stores a WORD PCNT stamp */
    int16_t next_damage;
    uint32_t special_damage_time;
    uint16_t risk;

    int32_t ptime;
    int32_t stars_flag;
    int32_t debris_x;
    int32_t run_time;
    void *shadtrail_proc;
    void *attimg_cur_frame;
    int32_t my_pal;
    int32_t obj_pal;
    /* PLYR.EQU SKELETON_PAL: the palette Doink's buzzer swaps in, put back
       from MY_PAL when it ends (DNKSEQ3.ASM set_skeleton_pal/set_my_pal). */
    int32_t skeleton_pal;
    /* PLYR.EQU OBJ_CONST: the constant colour the DMA writes in place of
       non-zero pixels when M_CONNON is on -- DNKSEQ3.ASM's make_white and
       make_black are exactly this field plus that control bit. */
    uint16_t obj_const;

    /* Stage 3: concrete REACT1.ASM reaction state. Velocities are source 16.16. */
    int32_t x_vel;
    int32_t y_vel;
    int32_t z_vel;
    int32_t ground_y;
    /*
     * PLYR.EQU:64 OBJ_GRAVITY, the per-tick pull WRESTLE2.ASM:2282
     * wrestler_veladd subtracts from OBJ_YVEL. It is not a constant: every
     * animation change resets it to GAME.EQU:436's GRAVITY (0x8000) --
     * ANIM.ASM:4520 change_anim_anim and :4553 change_anim1 both do it --
     * and an animation can then override it for itself with
     * ANI_SETLONG,OBJ_GRAVITY (115 uses, e.g. BAMSEQ2.ASM:3746's 0E000h
     * for a heavier fall).
     */
    int32_t gravity;
    /* PLYR.EQU:33 OBJ_PRIORITY, written by WRESTLE2.ASM:2385 calc_ground_y
       from where the wrestler is standing: 112 in the ring, 103 or 117
       outside it depending on Z. It is the sprite's draw order. */
    int32_t obj_priority;
    /*
     * PLYR.EQU:149 CLIMBING_THRU. calc_ground_y reads it to decide that a
     * wrestler between the mat edges is climbing IN rather than standing
     * outside, and puts him on MAT_Y. The climb subsystem this port has
     * (wm/arcade/wmania_ring_climb.h) keeps its own player struct and is
     * not joined to wm_arcade_actor yet, so nothing sets this field and
     * that branch does not fire -- which is the same path a wrestler who
     * is not climbing takes anyway.
     */
    int32_t climbing_thru;
    /* PLYR.EQU OBJ_FRICTION, set by ANIM.ASM's ANI_FRICTION (:22) together
       with MODE_FRICTION. */
    int32_t friction;
    /*
     * PLYR.EQU:152-156's five button-mash counters, in the source's own
     * order -- it comments them "keep ordered", because two pieces of code
     * depend on the layout:
     *
     *   WRESTLE.ASM:4681 count_button_presses walks them with `addi 16,a2`,
     *   one 16-bit WORD per button, testing BUT_VAL_DOWN bit 0 upward:
     *   punch, block, super punch, kick, super kick.
     *
     *   ANIM.ASM:3512 _ani_clr_butcount clears all five with three writes,
     *   two of them 32-bit: `move a14,*a13(PUNCHB_COUNT),L` covers punch
     *   AND block, `*a13(SPUNCHB_COUNT),L` covers super punch AND kick, and
     *   the plain 16-bit write covers super kick. The five commented-out
     *   single-WORD lines above them in the source are the unoptimised
     *   version of the same thing -- reading the comments rather than the
     *   widths is what made an earlier pass here believe block and kick
     *   were dropped from the reset. They are not.
     */
    /* PLYR.EQU HITBLOCKER: the wrestler who blocked this attack, which
       ANIM.ASM:83 ANI_IFBLOCKED branches on. Nothing sets it yet -- the
       blocked-reaction dispatch that would is still unwired -- so the
       branch is present and always falls through, which is the same path
       the flat model always took. */
    int32_t hitblocker;
    int32_t punchb_count;
    int32_t blockb_count;
    int32_t spunchb_count;
    int32_t kickb_count;
    int32_t skickb_count;
    /*
     * ANIM.ASM:2681 _ani_superslave2 writes the wrestler it is holding
     * directly: `move a0,*a11(CUR_FRAME)` plus ATTACH_XOFF/ATTACH_YOFF.
     * While a grapple is running, the victim's frame is not chosen by his
     * own animation at all -- the attacker's is choosing it for him, which
     * is why one throw shows a different victim pose per wrestler.
     *
     * puppet_frame NULL means nobody is driving him and his own animation
     * decides, as usual.
     */
    const char *puppet_frame;
    int32_t puppet_flip;
    int32_t roll_pos;
    int32_t usr_var1;
    int32_t usr_var2;              /* PLYR.EQU USR_VAR2; Yoko salt failure flag. */
    int32_t player_side;           /* PLYR.EQU PLYR_SIDE: 0, 1, or -1. */
    int32_t consecutive_hits;
    int32_t life;

    int32_t attach_xoff;
    int32_t attach_yoff;
    int32_t attach_zoff;
    uint16_t attack_time;          /* round_tickcount WORD */

    /* Final combat-core integration: animation/move-dispatch fields. */
    uint16_t ani_speed;
    uintptr_t special_move_addr;   /* source SPECIAL_MOVE_ADDR pointer/token */

    /* Stage 14 / BRET.ASM character-control adapter fields. */
    uint16_t but_val_cur;
    uint16_t but_val_down;
    uint16_t but_val_up;
    uint16_t stick_val_down;
    uint16_t stick_val_up;
    /* WRESTLE.ASM's punch_dtime1/powerp_dtime1/powerk_dtime1 (BSSX,
       WRESTLE.ASM:3954-3958): consecutive ticks that button has been held,
       reset to 0 the instant it's released -- see
       wm_arcade_update_joy_dtime. block_dtime1/kick_dtime1 aren't tracked
       here since nothing in this port reads them yet. */
    uint16_t punch_dtime;
    uint16_t powerp_dtime;
    uint16_t powerk_dtime;
    int32_t closest_dist;
    int32_t closest_xdist;
    int32_t closest_ydist;
    int32_t closest_zdist;
    int32_t walk_fast;
    int32_t i_will_die;
    uint32_t block_time;
    uint32_t last_headhold;
    uintptr_t code_addr;         /* source CODE_ADDR pointer/token; resolver owns meaning */
    int32_t delay_butns;
    int32_t attack_type;
};

typedef struct wm_arcade_combat_callbacks {
    int (*victim_has_live_teammates)(const wm_arcade_actor_t *victim, void *user);
    void (*wrestler_hit)(wm_arcade_actor_t *attacker,
                         wm_arcade_actor_t *victim,
                         void *user);
    void (*maybe_gidd_up)(wm_arcade_actor_t *victim, void *user);
    void *user;
} wm_arcade_combat_callbacks_t;

typedef enum wm_arcade_hit_result {
    WM_HIT_REJECTED = 0,
    WM_HIT_ACCEPTED = 1
} wm_arcade_hit_result_t;

void wm_arcade_set_hurt_box(wm_arcade_actor_t *actor,
                            const wm_arcade_frame_box_t *frame);
void wm_arcade_set_attack_box(wm_arcade_actor_t *actor);
int wm_arcade_resolve_overlap(wm_arcade_actor_t *mover,
                              const wm_arcade_actor_t *other);
wm_arcade_hit_result_t wm_arcade_try_attack_hit(
    wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    const wm_arcade_combat_callbacks_t *callbacks);
int wm_arcade_check_wrestler_collisions(
    wm_arcade_actor_t **actors,
    size_t actor_count,
    uint32_t round_tick,
    const wm_arcade_combat_callbacks_t *callbacks);
void wm_arcade_wrestler_collisions_off(wm_arcade_actor_t *actor);
void wm_arcade_set_getup_time(
    const wm_arcade_actor_t *attacker,
    wm_arcade_actor_t *victim,
    const wm_arcade_combat_callbacks_t *callbacks);

/* WRESTLE.ASM:4023 update_joy_dtime's #update_but half (the direction half,
   #update_stick, isn't translated -- nothing in this port reads a stick
   hold-duration yet). Reads but_val_cur, so call this after but_val_cur is
   set for the tick (wm_human_input_commit) and before anything that reads
   punch_dtime/powerp_dtime/powerk_dtime this same tick. */
void wm_arcade_update_joy_dtime(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
