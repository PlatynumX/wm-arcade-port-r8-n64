#ifndef WM_ARCADE_COFFIN_H
#define WM_ARCADE_COFFIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FINISEQ.ASM:580 und_coffin_up and the routines under it -- the coffin
 * the Undertaker's finish raises through the ring floor.
 *
 * This is the one finishing move the game actually assembled.
 * GAME.EQU:580-587 sets seven of the eight NUM_*_FINISHES to 0 and
 * NUM_TAKER_FINISHES to 1, so TAKER.ASM:57's `.if` is taken, :64's
 * `.ref und_coffin_up` is real, and FINISEQ.ASM:1653's
 * `WLWWWW ANI_CREATEPROC,und_coffin_up` is reached. Everything in this
 * header is live code; nothing here is cut content.
 *
 * What the source is, structurally: three processes and one animation
 * talking to each other through four globals. und_coffin_up drives the
 * mat, do_up_coffin drives the coffin, hover_coffin bobs it, and
 * und_2_raise_dead_anim (the Undertaker's own animation) polls the
 * globals with ANI_CODE routines and pushes them forward. The
 * choreography -- BEGINOBJ, obj_aniq, DELOBJA8, SLEEPK -- is the
 * display layer's, and is ledgered rather than translated. What is
 * here is the part that decides anything: the handshake, the
 * velocities, the ramps and the random draws.
 *
 * The four globals (FINISEQ.ASM:41-47):
 *
 *   @close_the_door   0 coffin still rising / door shut
 *                     1 door open        (do_up_coffin, :750)
 *                     2 close it, please (close_door, :915, from the
 *                       Undertaker's animation)
 *                     3 door is shut     (do_up_coffin, :811)
 *   @close_the_floor  0 hole still open
 *                     1 coffin is gone, close the mat (do_up_coffin, :866)
 *                     2 mat is shut      (und_coffin_up, :660)
 *                     3 tombstone is up  (und_coffin_up, :695)
 *   @guy_in           the dead man is inside (guy_is_in, :1550)
 *   @guy_up           the dead man is standing (guy_is_up, :1009)
 *
 * @guy_up is a fact about the dead wrestler and lives on the actor as
 * WM_STATUS_GUY_UP (wm_arcade_combat_defs.h); the other three are facts
 * about the sequence and live here.
 */
typedef struct {
    int32_t close_the_door;   /* @close_the_door, 0..3 as above. */
    int32_t close_the_floor;  /* @close_the_floor, 0..3 as above. */
    bool guy_in;              /* @guy_in. */
    /* @finish_completed. und_coffin_up stores the literal 3 into it
       (FINISEQ.ASM:696, sharing the a14 it just wrote to
       close_the_floor), so it is a count, not a flag -- kept as the
       source's value rather than reduced to a bool. */
    int32_t finish_completed;
    /*
     * @dead_wrestler (FINISEQ.ASM:43). TAKER.ASM:661 latches it once,
     * from the Undertaker's own WHOIHIT, before anything else in the
     * finish runs: `move *a8(WHOIHIT),a14,L / move a14,@dead_wrestler,L`.
     * Everything afterwards reads the latch rather than WHOIHIT, so the
     * port keeps the latch too rather than re-reading WHOIHIT at each
     * use and hoping it has not moved.
     *
     * NULL until something starts a finish, and then the routines fall
     * back to the caller's WHOIHIT -- which is where the latch came
     * from, so the two name the same wrestler.
     */
    wm_arcade_actor_t *dead_wrestler;
} wm_coffin_state_t;

/* FINISEQ.ASM:423-437, the sequence's own constants. */
#define WM_COFFIN_HOLE_XPOS 1250
#define WM_COFFIN_HOLE_YPOS 197
/*
 * MAT_BACK_Z .equ ((RING_Z_CENTER+20)|1000h). The 1000h bit is a
 * display-layer flag, which is why WRES_Z masks it back off with
 * &0fffh. The source keeps a commented-out older line above it,
 * `((RING_TOP+20)|1000h)`, and the comment at :419-421 -- "MUST start
 * at a Z of greater than 1430h and end up at a Z of 1428h" -- belongs
 * to that older line, not to this one: with RING_Z_CENTER the coffin's
 * Z is 14d0h and the wrestler's 4d0h. The comment is stale; the
 * equates below are what the assembler computed.
 */
#define WM_COFFIN_MAT_BACK_Z   ((WM_RING_Z_CENTER + 20) | 0x1000)  /* 14b4h */
#define WM_COFFIN_TMBSTN_Z     WM_COFFIN_MAT_BACK_Z
#define WM_COFFIN_MAT_FRONT_Z  (WM_COFFIN_MAT_BACK_Z + 27)         /* 14cfh */
#define WM_COFFIN_BACK_Z       (WM_COFFIN_MAT_BACK_Z + 28)         /* 14d0h */
#define WM_COFFIN_FRONT_Z      (WM_COFFIN_BACK_Z + 2)              /* 14d2h */
#define WM_COFFIN_DOOR_CLOSE_Z (WM_COFFIN_FRONT_Z + 2)             /* 14d4h */
#define WM_COFFIN_WRES_Z       (WM_COFFIN_BACK_Z & 0x0fff)         /*  4d0h */
#define WM_COFFIN_EXP_Z        (WM_COFFIN_MAT_FRONT_Z + 1)         /* 14d0h */
#define WM_COFFIN_VEL          4
#define WM_COFFIN_NUM_PUFFS    25
/* FINISEQ.ASM:1543 TIME_FOR_MOVE -- ticks to push the man into the box. */
#define WM_COFFIN_TIME_FOR_MOVE 16
/* do_up_coffin's ramp endpoints (FINISEQ.ASM:727, :836). */
#define WM_COFFIN_FULL_SIZEY   136
/* The coffin and the objects sit at HOLE_XPOS+7 (FINISEQ.ASM:704). */
#define WM_COFFIN_XPOS (WM_COFFIN_HOLE_XPOS + 7)

/* und_coffin_up's first two stores (FINISEQ.ASM:583-585): both cleared
   before anything else runs. guy_in and finish_completed are BSS, so
   the port zeroes them here too rather than leaving them stale from a
   previous match. */
void wm_coffin_reset(wm_coffin_state_t *st);

/* ---- the handshake --------------------------------------------- */

/* FINISEQ.ASM:915 close_door. One store; the Undertaker's animation
   calls it when he has finished shoving the man in. */
void wm_coffin_close_door(wm_coffin_state_t *st);

/* FINISEQ.ASM:1550 guy_is_in, from push_in_anim once he lands. */
void wm_coffin_guy_is_in(wm_coffin_state_t *st);

/* FINISEQ.ASM:924 is_door_open. The MODE_STATUS answer: any non-zero
   @close_the_door counts as open, including 2 and 3 -- the source
   tests `jrz`, not `cmpi 1`. */
bool wm_coffin_is_door_open(const wm_coffin_state_t *st);

/* FINISEQ.ASM:956 is_he_in. */
bool wm_coffin_is_he_in(const wm_coffin_state_t *st);

/* FINISEQ.ASM:940 is_guy_up, which reads @guy_up -- kept on the dead
   wrestler as WM_STATUS_GUY_UP, set by anim_code's guy_is_up. */
bool wm_coffin_is_guy_up(const wm_arcade_actor_t *dead);

/* @dead_wrestler: the latch above when a finish has set one, else the
   asker's own WHOIHIT, which is what TAKER.ASM latched it from. */
wm_arcade_actor_t *wm_coffin_dead_wrestler(const wm_coffin_state_t *st,
                                           wm_arcade_actor_t *asker);

/*
 * FINISEQ.ASM:985 make_wres_disappear. The Undertaker's animation
 * polls this every 5 ticks (`#d_loop`, :1716-1719) until it answers
 * yes. It answers yes exactly when the door has finished shutting --
 * @close_the_door == 3, not >= 3 -- and on that one tick it also
 * switches the dead man to disappear_wrestler.
 *
 * Returns the MODE_STATUS the source ORs in, and writes the animation
 * to start into *anim (NULL when there is none), the same hand-back
 * shape the rest of this port uses instead of calling change_anim1a.
 */
bool wm_coffin_make_wres_disappear(const wm_coffin_state_t *st,
                                   const char **anim);

/* The animation it starts, FINISEQ.ASM:972's own label. */
#define WM_COFFIN_DISAPPEAR_ANIM "disappear_wrestler"
/* FINISEQ.ASM:1626 push_to_coffin and :1519 raise_dead both start one
   animation on @dead_wrestler and return. */
#define WM_COFFIN_PUSH_IN_ANIM "push_in_anim"
#define WM_COFFIN_RAISE_DEAD_ANIM "raise_dead_anim"

/* ---- set_speeds ------------------------------------------------- */

/*
 * FINISEQ.ASM:1560 set_speeds, called from push_in_anim. Sets the dead
 * wrestler's X and Z velocities so he arrives at the coffin in
 * TIME_FOR_MOVE ticks. No Y velocity, despite the comment: the source
 * writes OBJ_XVEL and OBJ_ZVEL only, and push_in_anim supplies the
 * 27-pixel hop itself with ANI_SET_YVEL,1b0000h.
 *
 * The Z half has a clamp with a consequence worth stating: if he is
 * behind WRES_Z the source SNAPS his Z to WRES_Z and then computes the
 * velocity from the snapped value, so the velocity comes out zero. He
 * teleports that distance in one tick rather than sliding it.
 */
void wm_coffin_set_speeds(wm_arcade_actor_t *dead);

/* ---- the coffin's rise and fall --------------------------------- */

/*
 * do_up_coffin's `#mv_up_lp` (FINISEQ.ASM:726) and `#mv_dn_lp`
 * (:838). The object grows by COFFIN_VEL and its top edge moves the
 * same amount the other way, so it looks like it is coming through the
 * floor rather than growing in place.
 *
 * Up: start OSIZEY at 4, and each tick, if it is not exactly 136, add
 * 4 and subtract 4 from OYPOS. The test is `jrz`, an equality: 136 is
 * reached exactly because 136 is a multiple of 4, and any other start
 * would run away. Returns false when the ramp is finished.
 *
 * Down: start OSIZEY at 136 and stop when it is <= 1 (`cmpi 1 / jrle`),
 * which with a step of 4 means it stops at 0, after 34 steps.
 */
bool wm_coffin_rise_step(int32_t *sizey, int32_t *ypos);
bool wm_coffin_fall_step(int32_t *sizey, int32_t *ypos);

/* ---- hover_coffin ----------------------------------------------- */

/*
 * FINISEQ.ASM:879 hover_coffin, one iteration of `#do_agin`. It runs
 * every 7 ticks until the process that made it is killed.
 *
 * The delta is asymmetric, and not by accident of reading: going down
 * it is +1, and the source turns it around with `movk 1,a8 / not a8`.
 * NOT is a ones complement, so a8 becomes -2, not the -1 the comment
 * beside it claims. The coffin therefore drifts down in single pixels
 * and snaps back up in pairs.
 *
 * Both objects move by the delta; the turnaround is decided by the
 * FRONT object's distance from ITS start (a10/a11), and triggers when
 * that distance exceeds 2. Turning from + to - also fires eight puffs
 * of smoke at once; turning from - to + fires none.
 *
 * `delta` and `front_y` are updated in place. `front_start_y` is the
 * a11 the process latched before its first sleep. Returns the number
 * of ltl_exp processes to create this iteration: 0 or 8.
 */
int wm_coffin_hover_step(int32_t *delta, int32_t *back_y, int32_t *front_y,
                         int32_t front_start_y);

/* hover_coffin's two deltas, as the source computes them. */
#define WM_COFFIN_HOVER_DOWN 1
#define WM_COFFIN_HOVER_UP   (~1)   /* `movk 1,a8 / not a8` == -2 */
/* How many ltl_exp the + -> - turnaround creates (FINISEQ.ASM:902-909). */
#define WM_COFFIN_HOVER_PUFFS 8

/* ---- the door-slam shake ---------------------------------------- */

/*
 * do_up_coffin's `#shk_lp` (FINISEQ.ASM:817), 16 iterations of one
 * tick each. Both axes jitter about the ORIGINAL position, which the
 * process saved before the loop, so the shake does not walk.
 *
 * The two axes are not symmetric: X draws RNDRNG0(4) and subtracts 2,
 * giving -2..+2, but Y draws RNDRNG0(2) and subtracts the same 2,
 * giving -2..0. The coffin only ever jumps upward.
 */
void wm_coffin_shake_offset(WmRng *rng, int32_t base_x, int32_t base_y,
                            int32_t *out_x, int32_t *out_y);
#define WM_COFFIN_SHAKES 16

/* ---- ltl_exp ----------------------------------------------------- */

/*
 * FINISEQ.ASM:474 ltl_exp, the puff of smoke. Every one of these is a
 * separate process, and the first thing it does is sleep a random
 * number of ticks so that twenty-five created in one loop do not all
 * fire together.
 *
 * All six draws, in the source's order -- the order matters, because
 * they come off one shared RNDRNG0 stream:
 *
 *   delay  RNDRNG0(10) + 1         ticks before it appears
 *   x      RNDRNG0(80) - 40 + HOLE_XPOS
 *   y      RNDRNG0(30) - 25 + HOLE_YPOS
 *   z      RNDRNG0(10) + EXP_Z
 *   vel    RNDRNG0(7)  + 2         pixels per frame, UPWARD
 *   which  RNDRNG0(1)              0 = exp1_anim, 1 = exp2_anim
 *
 * Note the y draw: RNDRNG0(30) - 25 spans -25..+5, so a puff is
 * usually above the hole but can be up to 5 pixels below it. And
 * `vel` is subtracted from OYPOS each frame (:531), so it rises.
 */
typedef struct {
    int32_t delay;
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t vel;
    int32_t which;    /* index into ltl_puff: 0 or 1 */
} wm_coffin_puff_t;

void wm_coffin_ltl_exp_params(WmRng *rng, wm_coffin_puff_t *out);

/* The explosion sound ltl_exp plays once the object exists
   (FINISEQ.ASM:525, `movi 1252,a3 / calla SNDSND`). */
#define WM_COFFIN_PUFF_SOUND 1252
/* The frame is held 3 ticks (FINISEQ.ASM:528 SLEEPK 3). */
#define WM_COFFIN_PUFF_FRAME_TICKS 3

/* ---- the frame lists --------------------------------------------- */

/*
 * FINISEQ.ASM:552-577. These are `.long` lists of image headers, and
 * the port has no image headers yet, so they are kept as the source's
 * own symbol names. What is translated here is their SHAPE, which is
 * where the surprises are:
 *
 *   #mat_anim2 (:557) and #cof_anim2 (:565) are not animations. Each
 *   is a bare `.long 0` sitting immediately after the list before it,
 *   used as the END pointer: `#fc_loop` (:652) and `#close_loop`
 *   (:786) walk backwards from it with `move *-a9` until a9 reaches
 *   the list's start label. So closing plays the opening frames in
 *   reverse -- there is no second list.
 *
 *   #tstone_anim (:571) is one frame, TMBSTN01, and #tstone_test
 *   (:573) starts at TMBSTN02. und_coffin_up:668 has the
 *   `movi #tstone_anim,a9` COMMENTED OUT with `movi #tstone_test,a9`
 *   beneath it, so the tombstone that ships skips its own first frame.
 *   Both are given below because both are real source text; the live
 *   one is wm_coffin_tstone_frames.
 */
extern const char *const wm_coffin_mat_frames[];      /* MATCOF01..04 */
extern const size_t wm_coffin_mat_frame_count;
/* The mat's open ends on MATCOF05B/MATCOF05A, a split back/front pair
   (FINISEQ.ASM:610-620) rather than a fifth list entry. */
#define WM_COFFIN_MAT_OPEN_BACK  "MATCOF05B"
#define WM_COFFIN_MAT_OPEN_FRONT "MATCOF05A"

extern const char *const wm_coffin_door_frames[];     /* COFFIN02..05 */
extern const size_t wm_coffin_door_frame_count;
/* The shut coffin, before the door opens and after it closes
   (FINISEQ.ASM:710, :806). */
#define WM_COFFIN_CLOSED_IMAGE "COFFIN01"
/* The door's front half, created once the door is open
   (FINISEQ.ASM:761). */
#define WM_COFFIN_DOOR_FRONT_IMAGE "COFFIN6A"

extern const char *const wm_coffin_tstone_frames[];   /* TMBSTN02..08 */
extern const size_t wm_coffin_tstone_frame_count;
/* The frame `movi #tstone_anim,a9` would have added in front, left
   commented out at FINISEQ.ASM:668. */
#define WM_COFFIN_TSTONE_UNUSED_FIRST "TMBSTN01"

extern const char *const wm_coffin_exp1_frames[];     /* SMOKE01..10 */
extern const char *const wm_coffin_exp2_frames[];     /* SMOKEB01..10 */
extern const size_t wm_coffin_exp_frame_count;        /* 10, both lists */

/*
 * The close sequences, as the backwards walks actually produce them.
 * wm_coffin_close_frame(list, count, i) gives the i'th image the
 * closing loop shows, 0 <= i < count.
 */
const char *wm_coffin_close_frame(const char *const *frames, size_t count,
                                  size_t i);

#ifdef __cplusplus
}
#endif
#endif
