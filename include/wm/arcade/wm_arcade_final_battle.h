#ifndef WM_ARCADE_FINAL_BATTLE_H
#define WM_ARCADE_FINAL_BATTLE_H

#include <stdbool.h>
#include <stdint.h>

struct wm_arcade_actor;


#ifdef __cplusplus
extern "C" {
#endif

/*
 * PROGRESS.ASM:131 FINAL_BATTLE_LINEUP and :137 FINAL_PTR -- the queue
 * that makes the last rung of the WWF ladder an EIGHT-on-one instead of
 * the three-on-one it looks like.
 *
 * The source keeps both as globals because PROGRESS writes them and ANIM
 * reads them; here the pregame owns the struct and lends it to the match
 * through the animation env, the same shape wm_coffin_state_t already
 * uses for FINISEQ's three globals.
 *
 * `BSSX FINAL_BATTLE_LINEUP,10*8` is ten BYTES: eight wrestler numbers
 * and a two-byte terminator, written by get_final_lineup as
 * `movi -1,a14 / move a14,*a1,W` after the eight. Both terminator bytes
 * read back as -1 either way the game reads them -- ANIM.ASM:3090 takes
 * a signed byte (`movb *a0,a1 / jrn #die`) and WRESTLE2.ASM:3691 a word
 * -- so the negative entry, not the index, is what ends the queue.
 */
#define WM_FINAL_LINEUP_WRESTLERS 8u
#define WM_FINAL_LINEUP_SLOTS     10u

/*
 * Where FINAL_PTR starts. Both writers agree and both say why in the
 * same words: PROGRESS.ASM:4263 `addi 3*8,a0` sets it "to the next guy
 * to fight" right after building the rung out of entries 0, 1 and 2, and
 * WRESTLE.ASM:1607 `movi FINAL_BATTLE_LINEUP+24,a14` puts it back there
 * as the match starts. 24 is a BIT address: three 8-bit entries.
 */
#define WM_FINAL_PTR_START 3u

typedef struct {
    /*
     * Signed because the terminator is, and because the whole queue
     * protocol is "read one, stop if it is negative".
     */
    int8_t lineup[WM_FINAL_LINEUP_SLOTS];
    /* FINAL_PTR, as an index into lineup[] rather than a bit address. */
    uint8_t ptr;
} wm_final_battle_state_t;

/*
 * PROGRESS.ASM:1593 get_final_lineup. INIT_TEMP_TABLE and
 * RANDOMIZE_ORDER -- the same two the ladder builder opens with -- then
 * "just copy 8 bytes from TEMP_LADDER to FINAL_BATTLE_LINEUP" and set
 * the end-of-battle marker. All eight wrestlers, in one shuffled order:
 * the first three are the rung you see, the other five are the queue.
 *
 * `shuffled` is the eight bytes INIT_TEMP_TABLE/RANDOMIZE_ORDER
 * produced. Taking them as a parameter rather than drawing them here is
 * what keeps this file from carrying a second copy of a shuffle that
 * pregame.c already has, and it is the same `TEMP_LADDER` either way.
 */
void wm_final_set_lineup(wm_final_battle_state_t *fb,
                         const uint8_t shuffled[WM_FINAL_LINEUP_WRESTLERS]);

/*
 * PROGRESS.ASM:1535 get_royal_lineup. A rumble's two named entrants are
 * @index1 and @index2, and neither may also be one of the first two
 * wrestlers in the lineup. The fix is deliberately cheap and is not a
 * search: if either index matches either of the first two entries, SWAP
 * the two longs; look at the (new) first long again, and if one still
 * matches, ROTATE it 16 bits -- which on a long of four packed bytes
 * exchanges its two halves, putting entries 2 and 3 in front of 0 and 1.
 * Nothing checks a third time, so a lineup can still collide; the source
 * accepts that and so does this.
 */
void wm_final_royal_fixup(wm_final_battle_state_t *fb,
                          uint8_t index1, uint8_t index2);

/*
 * WRESTLE.ASM:1607 and PROGRESS.ASM:4263 both: FINAL_PTR back to the
 * fourth entry. The source re-does it at match start rather than
 * leaving NEXT_IN_LADDER's copy alone, and says why: "if we're speeding
 * through the rounds, that can happen while wrestler processes from the
 * previous round are still active and DEAD, so they gobble up the first
 * three slots and we end up with a 1v5 match."
 */
void wm_final_reset_ptr(wm_final_battle_state_t *fb);

/*
 * ANIM.ASM:3089 `move @FINAL_PTR,a0,L / movb *a0,a1 / jrn #die`, and the
 * `addk 8,a0` that follows on the live path. Returns the next wrestler
 * number and advances the queue, or -1 when the terminator has been
 * reached -- and then the pointer does NOT advance, because the source
 * never gets past the `jrn`.
 *
 * The 7->8 hack is the caller's, not this function's: ANIM applies it
 * after the read (`cmpi 7,a1 / jrne #vok / movk 8,a1`), the same packed
 * slot 7 -> live wrestler 8 mapping SORT_OUT_WRESTLER_NUM does
 * everywhere else.
 */
int wm_final_next_wrestler(wm_final_battle_state_t *fb);

/* The same read WITHOUT advancing: WRESTLE2.ASM:3691's raisearm_check
   hack only asks whether the queue is empty. */
bool wm_final_queue_empty(const wm_final_battle_state_t *fb);

/*
 * ANIM.ASM:3096-3131, everything `#fin` does to a wrestler once the
 * queue has handed it a number: the 7->8 hack, NEW_WRESTLERNUM,
 * M_ZOMBIE, a cleared ZOMBIE_TIME, and the Z-edge nudge that keeps him
 * off the ropes so he can roll.
 *
 * Split out from the queue read because the queue is the pregame's and
 * this is the wrestler's, and because a test can then check one without
 * standing up the other.
 *
 * `wrestler` is the raw queue entry; the 7->8 mapping happens here, as
 * it does in the source, after the read.
 */
void wm_final_make_zombie(struct wm_arcade_actor *actor, int wrestler);

/*
 * The Z nudge on its own: "if we're right up against either Z edge of
 * the ring, move away a few pixels so we can roll." Returns the Z the
 * wrestler should now be at.
 *
 * Reading the three-way branch off the source needs care, because the
 * comments and the arithmetic point opposite ways: `jrle #mvdn` on
 * RING_TOP+7 means "at or above the top edge", and #mvdn ADDS 7 -- Z
 * grows toward RING_BOT, so "down" is +7 and "up" is -7. The middle
 * case, at or above RING_BOT-7, moves nothing at all.
 */
int32_t wm_final_zombie_z_nudge(int32_t z);

/*
 * DOINK.ASM:3247 #run_speeds, the per-wrestler X run velocity a zombie
 * leaves on. The values are PLYR.EQU:452-475's own xxx_XRUN equs, in
 * WRESTLERNUM order, and slot 7 -- the spare the roster never fills --
 * is a literal `.long 0` in the table.
 */
/* DOINK.ASM:3014 `cmpi TSEC*10,a14` -- ten seconds of being a zombie
   before the source stops waiting for the trip to the arena edge. */
#define WM_FINAL_ZOMBIE_TIMEOUT (53 * 10)

#define WM_FINAL_RUN_SPEEDS 9
extern const int32_t wm_final_run_speeds[WM_FINAL_RUN_SPEEDS];

/*
 * WRESTLE2.ASM:3852 #init_positions, and the search above it: "possible
 * starting positions. hunt until you find one that's offscreen, then use
 * it. If none of them are offscreen (should never happen,) use the last
 * entry in the table."
 *
 * Offscreen means X at or left of WORLDTLX-30, or at or right of
 * WORLDTLX+430 -- the source computes the second as (WORLDTLX-30)+460
 * and writes the intent beside it as "WORLDTLX+400+30".
 *
 * `world_tlx` is WORLDTLX already taken down to integer pixels
 * (`sra 16`). Returns the index of the entry to use, always in range.
 */
typedef struct {
    int16_t x;
    int16_t z;
    int16_t y;        /* OBJ_YPOSINT and GROUND_Y both */
    int16_t in_ring;
} wm_final_start_pos_t;

#define WM_FINAL_START_POSITIONS 7
extern const wm_final_start_pos_t wm_final_start_positions[WM_FINAL_START_POSITIONS];

unsigned wm_final_choose_start_position(int32_t world_tlx);

/*
 * WRESTLE2.ASM:3772 change_wrestler, "Change into another wrestler and
 * re-enter the battle", minus the parts that are the display's.
 *
 * Translated here: WRESTLERNUM from NEW_WRESTLERNUM, MODE_NORMAL, the
 * whole STATUS_FLAGS long cleared (which is what takes M_ZOMBIE and
 * M_CAN_XFORM back off), I_WILL_DIE cleared, the offscreen start
 * position, and the three velocities zeroed.
 *
 * Left to the caller, because they need the match's own tables and the
 * wrestler's visual backend: init_smoves, choose_pal/pal_getf and the
 * palette sweep over OBJ_BASE's pieces, change_anim1a/change_anim2a onto
 * the new wrestler's stand4/torso4 pair, and init_wres_life_data.
 *
 * One thing NOT to read into the call site: WRESTLE.ASM:3729 does
 * `movk 1,a0 / calla change_wrestler`, and change_wrestler never looks
 * at a0. The 1 is dead.
 */
void wm_final_change_wrestler(struct wm_arcade_actor *actor,
                              int32_t world_tlx);

/*
 * DOINK.ASM:3008, mode_dead's `#zmb` tail -- one tick of being a zombie.
 * Returns true when the wrestler should transform NOW, which happens on
 * the timeout; the caller runs change_wrestler and the visual half.
 *
 * The rest of it: ZOMBIE_TIME counts up and at TSEC*10 the source gives
 * up on the trip to the edge ("something has probably gone wrong") and
 * transforms where he stands. Before that it waits for MODE_END -- "we
 * wait for the MODE_END bit to get set, which tells us that we're
 * standing up, outside, with a clear lane to either side" -- and only
 * then sets CAN_XFORM and starts him running toward whichever side of
 * the arena is farther from the camera.
 *
 * `run_left` comes back true when he should head left, which is what
 * `cmpi RING_X_CENTER-200,a14 / jrge #run_left` decides off WORLDTLX.
 * `x_vel` is his run velocity, negated for the left. Both are only
 * meaningful when the return is false and `*started_run` is true.
 */
bool wm_final_zombie_tick(struct wm_arcade_actor *actor,
                          int32_t world_tlx,
                          bool *started_run,
                          bool *run_left,
                          int32_t *x_vel);

#ifdef __cplusplus
}
#endif

#endif
