#ifndef WM_ARCADE_LIFEBAR_H
#define WM_ARCADE_LIFEBAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_react1_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * wm_arcade_adjust_health's own death-anim hook. Only ever invoked with
 * WM_R1_ANIM_FALL_BACK -- the same wrestler-agnostic id REACT1.ASM's own
 * hit-reaction code already uses for this identical per-wrestler anim
 * (LIFEBAR.ASM's fallbacks_t and REACT1.ASM's fall_back_anim dispatches
 * are the same real animation data, just invoked from two different
 * subroutines), so this doesn't invent a second naming scheme for it. May
 * be NULL (no wired backend for this victim -- true for every wrestler but
 * Bret in this port).
 */
typedef struct wm_arcade_death_anim_callback {
    void (*change_anim)(wm_arcade_actor_t *victim,
                        wm_arcade_react1_anim_group_t anim,
                        void *user);
    void *user;
} wm_arcade_death_anim_callback_t;

/* LIFEBAR.ASM:135 LIFE_MAX equ 163 (green pixels in life bar). */
#define WM_ARCADE_LIFE_MAX 163

/* GAME.EQU _85PCT equ 218 (8.8 fixed, ~0.85*256): LIFEBAR.ASM's
   damage_mod_table[0] entry -- see wm_arcade_adjust_health's comment for
   why this port's fixed 2-actor matches always resolve to that row. */
#define WM_ARCADE_DAMAGE_MOD_85PCT 218

/*
 * LIFEBAR.ASM:183 #timer_table[ADJSPEED-1], Q16.16 fixed. This port has no
 * live operator-settings/CMOS system to read a real ADJSPEED (1-5) from, so
 * it uses the arcade's own factory-shipped default instead of inventing
 * one: AUDIT.ASM's FACTORY_TABLE lists ADJSPEED (adjustment id 25) as 3,
 * exactly matching LIFEBAR.ASM:161's own BADCHK fallback value for an
 * out-of-range read. #timer_table[3-1] (0x10000*1) is the entry literally
 * commented "normal damage (default)" in the source -- i.e. the factory
 * default is precisely 1.0x, a mathematical identity multiply.
 */
#define WM_ARCADE_SPEED_ADJUSTMENT_16_16 0x10000L

/*
 * LIFEBAR.ASM::adjust_health's damage-application tail (LIFEBAR.ASM:
 * 1429-1670), shared by every real caller of it in this port: the REACT1.ASM
 * hit path (wm_arcade_wrestler_hit -> wm_match's wrestler_hit callback) and
 * BRET.ASM mode_normal's own "I_WILL_DIE amidst a combo" self-death case
 * (wm_arcade_move_bret, via wm_arcade_bret_callbacks_t.adjust_health) --
 * both call the same shared SUBR in the original, so this is one real
 * function rather than two copies.
 *
 * Translated:
 *   - LIFEBAR.ASM:1429-1466, applied to delta before anything else below:
 *     if damage_source's COMBO_COUNT is nonzero, delta is replaced entirely
 *     (not scaled) by -max(10-COMBO_COUNT, 4) and *dam_mult is cleared --
 *     "doing a combo" damage, unrelated to the original hit's own damage
 *     value. Otherwise, if *dam_mult is nonzero, delta is scaled by
 *     (delta*(1+*dam_mult))>>1 (DAM_MULT 2/3/4+ => x1.5/x2/x2.5, source's
 *     own repeated-addition-then-halve loop) and *dam_mult is cleared.
 *     dam_mult may be NULL (no DAM_MULT tracking in this call context --
 *     wm_bret_backend_callbacks' self-death path has none; only
 *     wm_arcade_wrestler_hit ever actually sets a runtime's dam_mult to
 *     2 or 4, via wm_match's wm_arcade_combat_runtime_t).
 *   - LIFEBAR.ASM:1471-1521, applied next, only when delta is still
 *     negative ("unless we're adding life"): delta *= WM_ARCADE_DAMAGE_MOD_
 *     85PCT (218/256, an 8.8 fixed multiply+shift identical to the
 *     unsigned-multiply-then-signed-shift the source uses on a negative
 *     delta) then >>=8. The real source scales this by active-drone-count
 *     and whether the victim is a drone or a player; this port's actors[]
 *     is always the fixed pair wm_match_start_attract/selected create, so
 *     that count is always 0 and both of the table's 0-drones columns are
 *     the same _85PCT anyway -- no PLYR_TYPE branch needed.
 *   - LIFEBAR.ASM:1524-1528, applied next unconditionally (healing and
 *     damage alike, unlike the damage_mod_table step): delta = (delta *
 *     WM_ARCADE_SPEED_ADJUSTMENT_16_16) >> 16. See that macro's own comment
 *     for why its value (the arcade's factory-default ADJSPEED setting) is
 *     exactly 0x10000 -- a 1.0x identity, so this line has no effect today,
 *     but is real and ready for a live value once this port has an operator-
 *     settings system to read one from.
 *   life = victim->life + delta (the possibly-transformed value above),
 *   clamped to [0, LIFE_MAX], except:
 *     - LIFEBAR.ASM:1561-1569 "fudge": a killing hit of 20+ points that
 *       doesn't overkill by 10 or more points bumps life to 5 instead of 0.
 *     - LIFEBAR.ASM:1578-1581 "if we're in attract mode, don't die!":
 *       attract_mode snaps life back to LIFE_MAX instead of letting it
 *       reach 0.
 *     - LIFEBAR.ASM:1659-1670: when life would genuinely reach 0 (neither
 *       fudged nor attract-mode-saved) and damage_source's COMBO_COUNT is
 *       nonzero, death is deferred: life becomes 1 and victim->i_will_die
 *       is set instead, matching mode_normal's own I_WILL_DIE resolution
 *       (BRET.ASM:1325-1350, translated in every wm_arcade_move_* wrestler
 *       dispatcher already). This is real but currently unreachable in this
 *       port: nothing yet increments combo_count anywhere (which also
 *       means the delta-override branch above is currently unreachable the
 *       same way).
 *     - LIFEBAR.ASM:1591-1725: a genuine death (life reaches 0, not
 *       deferred) runs the real death-dispatch tail before finally setting
 *       player_mode to WM_PMODE_DEAD and turning off further hit checks
 *       (wm_arcade_wrestler_collisions_off, matching SETMODE DEAD + calla
 *       wres_collis_off):
 *         - victim->roll_pos is reset to 0 (LIFEBAR.ASM:1608).
 *         - LIFEBAR.ASM:1691-1710's own pre-checks: if damage_source's
 *           attack_mode is WM_AMODE_BLBOWDROP/BSTOMP/BUTTSTOMP, or it's
 *           WM_AMODE_BUZZ and victim's own player_mode (read here, before
 *           it's overwritten to DEAD below) wasn't WM_PMODE_BLOCK, or
 *           victim->status_flags has WM_STATUS_DEAD_ANIM set, the whole
 *           death-animation dispatch below is skipped entirely (velocities
 *           untouched). None of Bret's wired attacks use those attack_mode
 *           ids and nothing in this port ever sets WM_STATUS_DEAD_ANIM on a
 *           path the live match reaches, so this is always false in
 *           practice today -- translated anyway since it's real, cheap, and
 *           correct the moment either changes.
 *         - Otherwise, victim's own player_mode (again, before being
 *           overwritten) selects the dispatch, exactly like LIFEBAR.ASM's
 *           own #fall check: WM_PMODE_NORMAL/RUNNING/INAIR/INAIR2/BOUNCING/
 *           ONTURNBKL/BLOCK/DIZZY/CLIMBTURNBKL (LIFEBAR.ASM's #fallbk) --
 *           every player_mode Bret's own real dispatcher (wm/arcade/
 *           wm_arcade_bret.h) can actually be in at the moment of death --
 *           fire death_anim->change_anim(victim, WM_R1_ANIM_FALL_BACK, ...)
 *           (the real hrt_fall_back_anim, already wired for real as
 *           WM_BRET_ANIM_FALL_BACK by Bret's own I_WILL_DIE self-death
 *           case) and apply LIFEBAR.ASM:1739-1753's own knockback: unless
 *           victim->x_vel is already > 2.0 px/tick, force it to +/-2.0 away
 *           from damage_source->x_int (matching the source's own default-
 *           then-override-by-side construction exactly, including its real
 *           quirk of only checking rightward speed -- a wrestler already
 *           flying left fast gets its velocity reduced to a flat -2.0, not
 *           left alone). Any other player_mode (LIFEBAR.ASM's own
 *           unmatched-mode catch-all, WM_PMODE_ATTACHED included) zeroes
 *           all three velocities and fires no anim, matching the source
 *           exactly for that catch-all case.
 *         - death_anim may be NULL (no wired backend for this victim --
 *           true for every wrestler but Bret in this port); the anim/
 *           knockback step is then simply skipped, same as if the pre-
 *           checks above had matched.
 *   - LIFEBAR.ASM:1593-1595 "update LAST_DAMAGE": unconditionally, on every
 *     call (fudged, attract-saved, genuinely dying, or a plain clamp
 *     alike), victim->last_damage is stamped with pcnt. This is what makes
 *     wm_arcade_wrestler_hit's own reduced_damage window real: it already
 *     read last_damage (REACT1.ASM's "elapsed_word(pcnt, last_damage)<=50"
 *     rapid-hit check) but nothing ever wrote it before this, so repeated
 *     attacks always dealt full_damage regardless of timing.
 *
 * NOT translated: CHECK_COMBO_GO (LIFEBAR.ASM:718 -- now actually located,
 * see wm/arcade/wm_arcade_mode_dead.h's own full derivation for why it's
 * provably always "not lit" in this port: no per-player combo-meter-fill
 * tracking at all, and instant_combos_on's 0-threshold bypass is an
 * AWARD.ASM credit-screen powerup toggle this port's credit system never
 * sets either -- so this function always allowing both combo_count
 * branches above, rather than gating them on CHECK_COMBO_GO like the
 * source does, is stricter than the source in a way that provably never
 * matters here), the lifebar flash-warning process (needs `ck_any_
 * teammates` and a `flash_obj`/`FLASH_PID` rendering process this port
 * doesn't have -- see wm_arcade_calc_closest's own note on the fixed
 * 2-actor boundary), ACTUAL_PLYRNUM/royal-rumble teammate propagation,
 * 8-on-1 wrestler_count bookkeeping, death sound (LIFEBAR.ASM's own
 * `triple_sound 034h` -- real, but this port's Bret backend leaves every
 * other real sound call a no-op too, wm_arcade_bret_callbacks_t.sound is
 * never wired anywhere, so adding a bridge for only this one call would be
 * inconsistent with every other already-real Bret sound cue), the #grnd
 * convulse_t/hitonground dispatch (real, but only reachable when a
 * wrestler's own player_mode is already WM_PMODE_ONGROUND or WM_PMODE_DEAD
 * at the moment of death, which this port's Bret dispatcher never sets --
 * so not guessed at, since no hrt_hitonground_anim data has been extracted
 * either), the #will_die HEADHELD deferral (sets i_will_die=180 without
 * immediately dying, but needs an eventual consumer this port doesn't have
 * and WM_PMODE_HEADHELD is likewise never set on Bret -- folded into the
 * same immediate catch-all as every other unmatched mode instead of
 * guessing at that consumer), and flash_red (pure lifebar-rendering
 * feedback, no rendering system exists here).
 */
void wm_arcade_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                             wm_arcade_actor_t *damage_source,
                             bool attract_mode, uint32_t pcnt,
                             int32_t *dam_mult,
                             const wm_arcade_death_anim_callback_t *death_anim);

/*
 * LIFEBAR.ASM:5310 SUBR clear_lifebar -- zero this wrestler's PLT_LIFE and
 * refresh his meter. Only the life half is translated; update_meter is
 * pure rendering and there is no meter to draw.
 *
 * ANIM.ASM:2990 _ani_waitroll is what calls it, and it is also the
 * consumer the note above says i_will_die lacked: adjust_health's #will_die
 * path defers a death by setting i_will_die instead of killing outright,
 * and ANI_WAITROLL is where that debt comes due once IMMOBILIZE_TIME runs
 * out.
 */
void wm_arcade_clear_lifebar(wm_arcade_actor_t *a);

/*
 * LIFEBAR.ASM:3650 SUBRP is_perfect -- did the winning side come
 * through the match untouched? It sweeps every active wrestler process,
 * skips the ones on the other side, and reports "not perfect" the
 * moment it finds one of its own whose get_health is anything but
 * LIFE_MAX. The caller (LIFEBAR.ASM:2821) hands out PERFECT_AWD and
 * spawns CREATE_PERFECT on a yes.
 *
 * Two details that are easy to get backwards. It returns through the
 * CARRY flag and sets it for perfect, so the caller's `jrnc` is the
 * NOT-perfect branch; and its first act is `calla is_8_on_1 / jrc
 * #final`, which clears carry -- so in the game's own final battle
 * nobody is ever perfect, whatever the healths say. is_8_on_1 always
 * reports "no" in this port for the reasons set out at length in
 * wm/arcade/wm_arcade_mode_dead.h, so `eight_on_one` is a parameter
 * rather than a call: passing true still gives the source's answer.
 *
 * `actors` is the process table the source walks; a NULL slot is an
 * inactive process and is skipped exactly as `jrz #nxt` skips it.
 */
bool wm_arcade_is_perfect(const wm_arcade_actor_t *winner,
                          const wm_arcade_actor_t *const *actors,
                          size_t actor_count, bool eight_on_one);


/* =================================================================
 * LIFEBAR.ASM:4039 SHIFT_BARS_IN_Z -- keeping the bars in front of
 * the ring when the camera rises.
 *
 * The life bars, their frames, their name plates and the backing
 * plate all live in a narrow Z band just behind the wrestlers. When
 * the camera pans high enough, the top rope would draw over them, so
 * the whole band is pushed 1800h forward in Z; when it comes back
 * down, the band is pushed the same distance back.
 *
 * The routine that does it is a four-tick poll with a single flag of
 * hysteresis. Three things are worth pinning down:
 *
 *   The compare is `MOVE @WORLDTLY,A0,L / MOVE @ZFLIP_POS_VAR,A1,L /
 *   CMP A1,A0 / JRLT` -- WORLDTLY BELOW the threshold means shift
 *   back, at or above means shift forward. Both are 16.16 and signed.
 *
 *   LAST_FLIP is the hysteresis, and it is worth being precise about
 *   what it buys. The band MOVES with the shift -- SHIFT_BARS_BACK
 *   tests [bak_z + Z_FORWARD, name_z + Z_FORWARD] -- so a repeated
 *   forward shift finds nothing in range and is already a no-op.
 *   What the flag saves is walking the whole object list every
 *   fourth tick to discover that.
 *
 *   The threshold is a variable, not a constant, and the end of a
 *   round sets it to ZFLIP_FOR_SURE (0F0F00000h) -- a value so
 *   negative that WORLDTLY can never be below it. That is how the
 *   bars are forced forward for the winner's announcement without a
 *   second code path.
 * ================================================================= */

/* GAME.EQU:513-514. */
#define WM_ZFLIP_POS 0x000D8000
#define WM_ZFLIP_FOR_SURE ((int32_t)0xF0F00000)
/* LIFEBAR.ASM:4037 and :499-502. The band is inclusive at both ends
   and covers bak_z (the backing plate) through name_z (the name
   plates), taking bar_z and frame_z with it. */
#define WM_ZFLIP_FORWARD 0x1800
#define WM_ZFLIP_BAK_Z 200
#define WM_ZFLIP_NAME_Z 203
/* `SLOOP 4` -- the poll runs every fourth tick. */
#define WM_ZFLIP_POLL_TICKS 4

typedef struct {
    /* @LAST_FLIP: false means the band is at its resting Z. */
    bool flipped;
    int32_t timer;
    /* Raised for the tick a shift is due, with its direction. */
    bool shifted;
    int32_t shift_by;
} wm_zflip_t;

void wm_zflip_begin(wm_zflip_t *z);

/*
 * One tick. `worldtly` and `threshold` are both 16.16 signed.
 * Returns true if a shift is due this tick, with `shift_by` set to
 * +WM_ZFLIP_FORWARD or -WM_ZFLIP_FORWARD; the caller applies it with
 * wm_zflip_apply. The process never ends -- `SLOOP` is an infinite
 * loop -- so there is no dead state to return.
 */
bool wm_zflip_tick(wm_zflip_t *z, int32_t worldtly, int32_t threshold);

/*
 * SHIFT_BARS_FORWARD / SHIFT_BARS_BACK: walk the object list and add
 * `shift_by` to every OZPOS inside the band. The band for the way
 * back is the forward band PLUS the shift, which is what makes the
 * two exactly undo each other.
 *
 * `zpos` is the array of object Z positions; returns how many moved.
 */
size_t wm_zflip_apply(int32_t *zpos, size_t count, int32_t shift_by);

/* Whether one Z is inside the band for a shift of `shift_by`. */
bool wm_zflip_in_band(int32_t z, int32_t shift_by);


/* =================================================================
 * LIFEBAR.ASM:880 rewire_monitor -- which wrestler the second life
 * bar is showing.
 *
 * The source's own comment: "Use in a 1-player game when there are
 * multiple drones on the other team. This process keeps the other
 * lifebar/name/combo meter display up-to-date. It won't do a rewire
 * within #LATENCY ticks of the last rewire unless the currently
 * displayed wrestler is dead."
 *
 * There are three completely different loops behind that one name,
 * chosen once at startup and never revisited:
 *
 *   BUDDY  -- two humans against two drones. Display 0 follows
 *             player 1 or, if he is dead, wrestler 2; display 1
 *             follows player 2 or wrestler 3. Each display sticks
 *             with a dead wrestler if his stand-in is dead too.
 *
 *   RUMBLE -- the royal rumble. One display alternates between two
 *             wrestlers, holding each for TSEC*4/10 polls or until
 *             it dies, and refusing to toggle onto a dead one.
 *
 *   NORMAL -- one human against a team. The display follows whoever
 *             the human's CLOSEST_NUM says he is fighting, with the
 *             latency above so the bar does not flicker between two
 *             drones taking turns.
 *
 * And two ways to exit before any of that: PSTATUS == 3 (two humans
 * and not buddy mode -- both bars are already spoken for) and
 * NUM_OPPS == 1 (nothing to switch between).
 *
 * All three poll on `SLOOP 10`.
 * ================================================================= */

/* `#LATENCY .equ TSEC/2`. */
#define WM_REWIRE_LATENCY (53 / 2)
/* `SLOOP 10` -- every tenth tick, in all three loops. */
#define WM_REWIRE_POLL_TICKS 10
/* `movk TSEC*4/10,a8` -- how many POLLS the rumble holds a wrestler,
   not how many ticks: a8 is decremented once per SLOOP. */
#define WM_REWIRE_RUMBLE_HOLD (53 * 4 / 10)

typedef enum {
    WM_REWIRE_MODE_DIE = 0,
    WM_REWIRE_MODE_NORMAL,
    WM_REWIRE_MODE_RUMBLE,
    WM_REWIRE_MODE_BUDDY
} wm_rewire_mode_t;

/*
 * The branch at the top, in the source's own order: buddy first,
 * then rumble, then the two reasons to die.
 */
wm_rewire_mode_t wm_rewire_mode(bool buddy_mode_on, bool royal_rumble,
                                int32_t pstatus, int32_t num_opps);

/*
 * What the monitor reads about each wrestler. The port keeps these
 * on its own actor records; this is the slice the monitor touches.
 */
typedef struct {
    bool active;          /* a non-NULL process_ptrs slot */
    int32_t plyrnum;
    int32_t closest_num;
    int32_t plyr_side;
    bool dead;            /* PLYRMODE == MODE_DEAD */
} wm_rewire_actor_t;

typedef struct {
    wm_rewire_mode_t mode;
    int32_t timer;
    bool started;

    /* NORMAL: a10, the human being followed, and a11, who his bar
       is currently showing. */
    int32_t watcher;
    int32_t showing;
    /* @#LAST_REWIRE, in PCNT units. */
    int32_t last_rewire;
    bool have_last_rewire;

    /* RUMBLE: a9, the wrestler on display, and a8, the hold. */
    int32_t rumble_at;
    int32_t rumble_hold;
    /* BUDDY: a8 and a9, what each display is currently showing. */
    int32_t buddy_show0;
    int32_t buddy_show1;

    /*
     * The rewire_meter calls due this tick. The buddy loop checks
     * both displays in one pass and can rewire both, so this is a
     * list rather than a flag.
     */
    size_t rewire_count;
    struct {
        int32_t display;
        int32_t wrestler;
    } rewires[2];
} wm_rewire_t;

/*
 * `actors` is process_ptrs; `count` its length. Returns false if the
 * monitor DIEs immediately, which is both exit paths at the top and
 * the NORMAL branch finding no live process at all.
 */
bool wm_rewire_begin(wm_rewire_t *r, wm_rewire_mode_t mode,
                     const wm_rewire_actor_t *actors, size_t count);

/*
 * One tick. `pcnt` is the game's frame counter, read only by the
 * NORMAL branch. Returns true while the monitor is alive;
 * `rewire_count` is how many rewire_meter calls are due this tick,
 * in the order the source makes them.
 */
bool wm_rewire_tick(wm_rewire_t *r, const wm_rewire_actor_t *actors,
                    size_t count, int32_t pcnt);

#ifdef __cplusplus
}
#endif

#endif
