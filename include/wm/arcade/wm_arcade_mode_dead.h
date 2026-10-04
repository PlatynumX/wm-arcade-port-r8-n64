#ifndef WM_ARCADE_MODE_DEAD_H
#define WM_ARCADE_MODE_DEAD_H

#include <stdbool.h>
#include <stddef.h>

#include "wm/arcade/wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DOINK.ASM:2920 SUBR mode_dead ;9 -- the shared MODE_DEAD entry every
 * wrestler's own #mode_table[9] points at (confirmed identical in
 * BRET.ASM:1305, RAZOR.ASM:1153 and the rest: it is one real routine,
 * not per-wrestler code).
 *
 * Its job is deciding whether a freshly-dead wrestler gets a "buckoff"
 * -- the button-mashing comeback that brings him back to life as a
 * zombie. This port used to collapse every path through it to the same
 * outcome, on an argument that has since stopped being true in two of
 * its three parts:
 *
 *   - `@royal_rumble` really is always zero here: no app mode starts a
 *     rumble. That part stands.
 *   - `is_8_on_1` was argued away because belt_type and CURRENT_LADDER
 *     "belong to the same unimplemented ladder system". They do not --
 *     the belt is chosen at the pregame, the ladder is real, and the
 *     final rung of a championship run IS an eight-on-one.
 *   - `CHECK_COMBO_GO` was argued away because the port tracked no
 *     combo-meter fill. It does: add_to_combo_count writes COMBO_SIZE,
 *     and wm_arcade_check_combo_go now answers the real question.
 *
 * So the whole routine is translated, and the buckoff can actually
 * happen. The gates, in the source's own order:
 *
 *   1. Already a zombie -> the `#zmb` tail, which is
 *      wm_final_zombie_tick in wm/arcade/wm_arcade_final_battle.h and is
 *      driven from the match rather than here, because completing a
 *      transform needs init_smoves' tables and the life data.
 *   2. DID_BUCKOFF -> one per match, and he has had it.
 *   3. NO_BUCKOFF -> already checked and refused this round.
 *   4. DO_BUCKOFF -> already counting; go straight to the button count.
 *   5. `@royal_rumble` -> no buckoff in a rumble, and no flag set either.
 *   6. Is this the SECOND round he has lost? Skipped entirely in an
 *      eight-on-one, which instead applies `#ck81`: "only the player is
 *      allowed to buckoff", i.e. PLYRNUM below 2.
 *   7. Is his combo meter lit (CHECK_COMBO_GO)?
 *   8. Is he INSIDE the ring?
 *   9. Is the Undertaker's finish move running or finished? Either one
 *      refuses -- "Buckoff is NOT allowed if undertaker started his
 *      finish move or has completed his finish move!!!!"
 *
 * Pass all of them and BUCKOFF_COUNT is zeroed and DO_BUCKOFF set; fail
 * any and NO_BUCKOFF is set, which is what stops it being re-asked every
 * tick for the rest of the round.
 */

/*
 * What mode_dead needs that is not on the wrestler, and what it decides
 * that it cannot carry out.
 *
 * The source reads five globals and walks process_ptrs; this is that,
 * gathered by the caller. A NULL env is treated as "no information",
 * which fails the gates rather than passing them.
 */
typedef struct {
    /* @p1rounds / @p2rounds -- rounds lost by each SIDE. */
    int32_t rounds[2];
    /* is_8_on_1(), and @royal_rumble. */
    bool eight_on_one;
    bool royal_rumble;
    /* AWARD.ASM's combos_on powerup, for CHECK_COMBO_GO's threshold. */
    int32_t instant_combos_on;
    /* @in_finish_move and @finish_completed. */
    bool in_finish_move;
    bool finish_completed;
    /* process_ptrs, for #dobuck's three sweeps. */
    wm_arcade_actor_t *const *actors;
    size_t actor_count;
} wm_mode_dead_env_t;

typedef struct {
    /* He passed, mashed enough, and came back to life this tick. */
    bool bucked_off;
    /*
     * `FACETBL hitonground_tbl / calla change_anim1a` on himself, and
     * `FACETBL #buckoff_tbl` on whichever opponent had pinned him. Both
     * are per-wrestler animation-table dispatches, so the choice is
     * reported and the caller plays it.
     */
    bool convulse;
    wm_arcade_actor_t *pinner_to_buck;
    /* `CREATE MESSAGE_PID,MOVE_NAME_ANNC` with a10=41 -- the "second
       wind" message sliding out. Display, so it is reported. */
    bool second_wind_message;
} wm_mode_dead_result_t;

/*
 * DOINK.ASM:3067 `#count_btns` fires at 50. The count is of BUTTON
 * PRESSES, not ticks: the source pops every set bit out of BUT_VAL_DOWN
 * with `lmo`/`rl`/`sla` and adds however many it found.
 */
#define WM_BUCKOFF_TARGET 50

/* `movk 2,a0 / calla adjust_health` -- the two points he comes back with. */
#define WM_BUCKOFF_HEALTH 2

/* The "second wind" message index: `movi 41,a10`. */
#define WM_BUCKOFF_MESSAGE 41

void wm_arcade_mode_dead_ex(wm_arcade_actor_t *actor,
                            const wm_mode_dead_env_t *env,
                            wm_mode_dead_result_t *out);

/* The old entry point: no env, so the gates refuse and this is the
   NO_BUCKOFF-only behaviour it has always had. */
void wm_arcade_mode_dead(wm_arcade_actor_t *actor);

#ifdef __cplusplus
}
#endif

#endif
