#include "wm/wrestler_backend.h"
#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wm_arcade_modes.h"
#include "wm/arcade/wm_arcade_bounce.h"
#include "wm/arcade/wm_arcade_auto_pin.h"
#include "wm/arcade/wm_arcade_bozo.h"
#include "wm/wrestler_sound_labels.h"
#include "wm/wrestler_sound_tables.h"
#include "wm/arcade/wm_arcade_teammates.h"
#include "wm/award.h"
#include "wm/arcade/wm_arcade_wrestler_port.h"
#include "wm/arcade/wm_arcade_joystat.h"
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combo.h"
#include "wm/arcade/wm_arcade_pin.h"

#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_mode_dead.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
#include "wm/bret_backend.h"
#include "wm/arcade/wm_arcade_start_run.h"
#include "wm/wrestler_anim_tables.h"
#include "wm/movement.h"

#include <string.h>

/*
 * #VEL / #DVEL as each wrestler's own ASM defines them, immediately above
 * its own xxx_velocity_table (see wm/wrestler_backend.h). Bret's pair is
 * already spelled out as WM_BRET_WALK_VEL/DVEL in wm/arcade/wm_arcade_bret.h
 * and matches these exactly.
 */
#define WM_WALK_VEL_STD  0x0003a000
#define WM_WALK_DVEL_STD 0x00031000
#define WM_WALK_VEL_DNK  0x00030000
#define WM_WALK_DVEL_DNK 0x00021f0e

/* The table body itself is identical in all eight files, compass-ordered:
   .long 0,-#VEL / #DVEL,-#DVEL / #VEL,0 / #DVEL,#DVEL / 0,#VEL /
   -#DVEL,#DVEL / -#VEL,0 / -#DVEL,-#DVEL. */
#define WM_WALK_TABLE(v, d)      \
    { {  0,   -(v) },            \
      {  (d), -(d) },            \
      {  (v),  0   },            \
      {  (d),  (d) },            \
      {  0,    (v) },            \
      { -(d),  (d) },            \
      { -(v),  0   },            \
      { -(d), -(d) } }

static const wm_move_velocity_entry s_velocity_std[8] =
    WM_WALK_TABLE(WM_WALK_VEL_STD, WM_WALK_DVEL_STD);
static const wm_move_velocity_entry s_velocity_doink[8] =
    WM_WALK_TABLE(WM_WALK_VEL_DNK, WM_WALK_DVEL_DNK);

const wm_move_velocity_entry *wm_wrestler_velocity_table(int32_t wrestler_num) {
    return wrestler_num == (int32_t)WM_ROSTER_DOINK ? s_velocity_doink : s_velocity_std;
}

/* Defined below, beside the other channel plumbing. */
static void backend_change_anim_label(wm_arcade_actor_t *actor,
                                      const char *source_label, void *user);
static void backend_change_anim_restart(wm_arcade_actor_t *actor,
                                        const char *source_label, void *user);
static void backend_change_anim2_label(wm_arcade_actor_t *actor,
                                       const char *source_label, void *user);
static void backend_change_anim2_restart(wm_arcade_actor_t *actor,
                                         const char *source_label, void *user);

/*
 * The label a slot's table holds at (row, col), or NULL.
 *
 * The 4x4 tables are indexed by DIAGONAL: the source does
 * `callr convert_facing / srl 1`, so the eight compass directions
 * collapse to four (WRESTLE.ASM:4978's own comment, "only uses
 * diagonals (0-3)"). The 8x8 leg table is the full compass, no shift.
 */
static const char *slot_label(const wm_wrestler_anim_table *tables,
                              int wrestler_num, int row, int col) {
    if (wrestler_num < 0 || wrestler_num >= WM_WRESTLER_ANIM_SLOTS)
        return NULL;
    if (row < 0 || col < 0) return NULL;     /* convert_facing found none */
    return wm_wrestler_anim_label(&tables[wrestler_num], row, col);
}

/*
 * WRESTLE.ASM:4946 change_walk_anim and WRESTLE.ASM:5062 set_rotate_anim,
 * for the seven wrestlers driven by the shared program backend.
 *
 * Bret's hand-built backend has had both since his walk was wired
 * (src/core/bret_backend.c's wm_bret_backend_execute_walk); this is the
 * same two routines against the generated per-wrestler tables, so the
 * other seven stop walking and turning with whatever animation they
 * happened to be left in. Until now the shared backend called
 * wm_execute_walk and nothing else: no leg reselection, no turn, and a
 * torso frozen at whatever *_ani_init started it on.
 *
 * The structure follows the source exactly, including which half runs
 * when. change_walk_anim is reached only from the eight real-movement
 * walk_table handlers, never from #zip, so both its halves are nested
 * under MOVE_DIR != 0; set_rotate_anim is the idle path's counterpart
 * and drives the legs alone -- the source never touches the torso from
 * #zip.
 */
void wm_wrestler_backend_execute_walk(wm_arcade_actor_t *actor,
                                      wm_wrestler_backend_actor *st) {
    int32_t old_facing_dir;

    if (!actor || !st) return;
    old_facing_dir = actor->facing_dir;
    wm_execute_walk(actor, st->opponent, wm_wrestler_velocity_table(st->wrestler_num));

    if (actor->move_dir != 0) {
        int move_compass = wm_convert_facing(actor->move_dir);
        int facing_compass = wm_convert_facing(actor->facing_dir);
        const char *legs = slot_label(wm_wrestler_leg_anims, st->wrestler_num,
                                      move_compass, facing_compass);

        /* The leg half (WRESTLE.ASM:5000). It never writes FACING_DIR, so
           neither does this: FACING_DIR stays frozen at its last idle
           value while walking, exactly as the source leaves it. */
        if (legs) backend_change_anim_label(actor, legs, st);

        /* The torso half (WRESTLE.ASM:4973).
         *
         * The source gates this on ANIMODE2 -- the SECOND channel's
         * mode word. This port has one mode field on the actor, not
         * two: nothing writes an ANIMODE2, so there is no second word
         * to read and inventing one here would be worse than saying so.
         * The gate below is the PRIMARY channel's MODE_UNINT, which is
         * the same approximation Bret's hand-built backend has always
         * made (src/core/bret_backend.c). It errs the same way in both:
         * a torso reselect is suppressed while the legs are in an
         * uninterruptible animation, where the source would have
         * allowed it. Splitting ANIMODE2 out is its own change. */
        if (!(actor->anim_mode & WM_MODE_UNINT)) {
            int new_compass = wm_convert_facing(actor->new_facing_dir);
            const char *torso = slot_label(
                wm_wrestler_torso_anims, st->wrestler_num,
                facing_compass >= 0 ? facing_compass >> 1 : -1,
                new_compass >= 0 ? new_compass >> 1 : -1);
            if (torso) backend_change_anim2_label(actor, torso, st);
        }
        return;
    }

    /* set_rotate_anim (WRESTLE.ASM:5062), the idle path. It reads
       FACING_DIR as it was BEFORE wm_execute_walk's WM_MOVE_ZIP catch-up
       applied the new value -- which is what the real routine sees,
       since it overwrites FACING_DIR itself afterwards. */
    {
        const char *turn = wm_wrestler_set_rotate_anim(
            actor, st->wrestler_num, old_facing_dir);
        if (turn) backend_change_anim_label(actor, turn, st);
    }
}

const char *wm_wrestler_set_rotate_anim(wm_arcade_actor_t *actor,
                                        int wrestler_num,
                                        int32_t facing_dir) {
    int old_compass, new_compass;

    if (!actor) return NULL;

    old_compass = wm_convert_facing(facing_dir);
    new_compass = wm_convert_facing(actor->new_facing_dir);

    /* WRESTLE.ASM:5082-5083, and it is not conditional on having found
       an animation: `move *a13(NEW_FACING_DIR),a14 / move a14,
       *a13(FACING_DIR)` runs between the table read and the rets. */
    actor->facing_dir = actor->new_facing_dir;

    return slot_label(wm_wrestler_rotate_anims, wrestler_num,
                      old_compass >= 0 ? old_compass >> 1 : -1,
                      new_compass >= 0 ? new_compass >> 1 : -1);
}

static void backend_execute_walk(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_execute_walk(actor,
                                     (wm_wrestler_backend_actor *)user);
}

static void backend_adjust_health(wm_arcade_actor_t *actor, int delta, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!actor) return;
    /* Same real LIFEBAR.ASM::adjust_health every other caller shares. No
       death_anim callback is passed: selecting a death animation needs real
       per-wrestler frame data this port only has for Bret, so this leaves
       the source's own death-anim dispatch unwired rather than guessing at
       a substitute -- exactly what wm_match_death_change_anim already does
       for a non-Bret victim on the REACT1.ASM hit path. No
       wm_arcade_combat_runtime_t is reachable from this self-damage path
       either, same as Bret's own backend, so DAM_MULT is skipped here. */
    wm_arcade_adjust_health(actor, (int16_t)delta, actor->who_hit_me,
                            st ? st->attract_mode : false,
                            st ? st->pcnt : 0u, NULL, NULL);
}

static void backend_mode_dead(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!st) { wm_arcade_mode_dead(actor); return; }
    wm_arcade_mode_dead_ex(actor, &st->mode_dead_env, &st->mode_dead_result);
}

/*
 * LIFEBAR.ASM:718 CHECK_COMBO_GO, for real. This used to return a flat
 * -1 ("never lit") on the grounds that the port tracked no combo-meter
 * fill; it does -- add_to_combo_count has been writing COMBO_SIZE for a
 * while -- so the gate can answer honestly now.
 */
static int backend_check_combo_go(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    return (int)wm_arcade_check_combo_go(
        actor, st ? st->instant_combos_on : 0);
}

/*
 * ANIM.ASM's two primary-animation entry points, as one body with the
 * guard switched off for the second.
 *
 * The eight dispatchers have always passed the source's own routine name
 * here, and the program registry is keyed on exactly that, so the lookup
 * is the whole join.
 *
 * `guard` is what separates :4532 change_anim1 from :4542 change_anim1a:
 *
 *      move  *a13(ANIMODE),a2
 *      btst  MODE_END_BIT,a2       ; if the animation has ENDED,
 *      jrnz  change_anim1a         ; always restart it
 *      move  *a13(ANIBASE),a2,L
 *      cmp   a0,a2
 *      jreq  #no_change            ; same animation, still running: NOP
 *
 * change_anim1a is the label on the next instruction, so entering there
 * skips both tests. st->prog.ended stands in for MODE_END_BIT and
 * st->current_label for ANIBASE.
 *
 * This used to be one function that always guarded, which quietly turned
 * every change_anim1a call site in the eight dispatchers into a
 * change_anim1 -- so a move you mash could not replay while its own
 * animation was still running.
 */
static void backend_change_anim(wm_arcade_actor_t *actor,
                                const char *source_label, void *user,
                                bool guard) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    const wm_anim_program *prog;
    if (!actor || !st || !source_label) return;

    if (guard && st->current_label && st->prog.program && !st->prog.ended &&
        strcmp(st->current_label, source_label) == 0)
        return;

    prog = wm_anim_program_find(source_label);
    st->current_label = source_label;
    if (!prog) {
        /*
         * A label with no generated program: the wrestler keeps
         * whatever he was doing rather than freezing on a stale one.
         * That is the right behaviour and also a SILENT one -- an
         * invented label and a genuinely unextracted animation look
         * identical from here.
         *
         * This comment used to say the set was "a measured, reported
         * number, not a guess" and name a function called
         * wm_wrestler_backend_program_coverage. There was no such
         * function; nothing measured it. The measurement exists now,
         * as test_every_dispatcher_animation_label_resolves in
         * tests/test_source_tools.py, and it had to be a test rather
         * than a routine because the set of labels a dispatcher CAN
         * pass is a property of its source text and not of any state
         * reachable from in here.
         *
         * It found one on its first run: "fake_head_hold3", used in
         * all six shared dispatchers where the source has a
         * per-wrestler dnk_3_fake_hold_anim, und_3_fake_hold_anim and
         * so on -- so the fake head hold played nothing, for everybody.
         */
        st->prog.program = NULL;
        st->prog.ended = true;
        return;
    }
    st->anim_env.opponent = st->opponent;
    st->anim_env.pcnt = st->pcnt;
    wm_anim_exec_start(&st->prog, prog, actor, (uint16_t)st->pcnt,
                       &st->anim_env);
}

/* ANIM.ASM:4532 change_anim1 -- guarded. */
static void backend_change_anim_label(wm_arcade_actor_t *actor,
                                      const char *source_label, void *user) {
    backend_change_anim(actor, source_label, user, true);
}

/* ANIM.ASM:4542 change_anim1a -- unguarded, always from the top. */
static void backend_change_anim_restart(wm_arcade_actor_t *actor,
                                        const char *source_label, void *user) {
    backend_change_anim(actor, source_label, user, false);
}

/*
 * :4563 change_anim2 / :4573 change_anim2a on the generic backend.
 * Mirrors the primary path above, including the same `guard` split, and
 * differs in the two ways the source differs: it writes the second
 * channel, and starting it does not reset gravity.
 */
static void backend_change_anim2(wm_arcade_actor_t *actor,
                                 const char *source_label, void *user,
                                 bool guard) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    const wm_anim_program *prog;
    if (!actor || !st || !source_label) return;

    if (guard && st->torso_label && st->torso_prog.program &&
        !st->torso_prog.ended &&
        strcmp(st->torso_label, source_label) == 0)
        return;

    prog = wm_anim_program_find(source_label);
    st->torso_label = source_label;
    if (!prog) {
        st->torso_prog.program = NULL;
        st->torso_prog.ended = true;
        return;
    }
    st->anim_env.opponent = st->opponent;
    st->anim_env.pcnt = st->pcnt;
    wm_anim_exec_start_secondary(&st->torso_prog, prog, actor,
                                 (uint16_t)st->pcnt, &st->anim_env);
}

/* ANIM.ASM:4563 change_anim2 -- guarded. */
static void backend_change_anim2_label(wm_arcade_actor_t *actor,
                                       const char *source_label, void *user) {
    backend_change_anim2(actor, source_label, user, true);
}

/* ANIM.ASM:4573 change_anim2a -- unguarded. */
static void backend_change_anim2_restart(wm_arcade_actor_t *actor,
                                         const char *source_label, void *user) {
    backend_change_anim2(actor, source_label, user, false);
}

void wm_wrestler_backend_ani_init(wm_wrestler_backend_actor *state,
                                  wm_arcade_actor_t *actor) {
    const wm_ani_init_row *row;
    bool facing_right;

    if (!state || !actor) return;
    if (state->wrestler_num < 0 ||
        state->wrestler_num >= WM_ANI_INIT_SLOTS) return;
    row = &wm_ani_init_rows[state->wrestler_num];
    if (!row->stand2) return;           /* Adam Bomb: cut, no animations */

    /* `move *a13(FACING_DIR),a0 / btst PLAYER_RIGHT_BIT,a0 / jrnz #p1` */
    facing_right = (actor->facing_dir & WM_MOVE_RIGHT) != 0;
    /* BRET.ASM:1247 bret_ani_init and its seven siblings: change_anim1a
       and change_anim2a, both unguarded. */
    backend_change_anim_restart(actor,
                                facing_right ? row->stand2 : row->stand4,
                                state);
    backend_change_anim2_restart(actor,
                                 facing_right ? row->torso2 : row->torso4,
                                 state);
}

const char *wm_wrestler_backend_torso_frame(
    const wm_wrestler_backend_actor *state) {
    if (!state || !state->torso_prog.program) return NULL;
    return wm_anim_exec_frame(&state->torso_prog);
}

void wm_wrestler_backend_tick(wm_wrestler_backend_actor *state,
                              wm_arcade_actor_t *actor) {
    if (!actor) return;

    if (state && state->torso_prog.program) {
        /* The torso runs on its own clock beside the primary channel,
           which is what having two ANIPCs means. */
        state->anim_env.opponent = state->opponent;
        state->anim_env.pcnt = state->pcnt;
        wm_anim_exec_tick(&state->torso_prog, actor, (uint16_t)state->pcnt);
    }

    if (state && state->prog.program) {
        const char *frame;
        state->anim_env.opponent = state->opponent;
        state->anim_env.pcnt = state->pcnt;
        wm_anim_exec_tick(&state->prog, actor, (uint16_t)state->pcnt);
        frame = wm_anim_exec_frame(&state->prog);
        if (frame) {
            wm_arcade_frame_box_t box = wm_hurt_box_for_frame(frame);
            wm_arcade_set_hurt_box(actor, &box);
        }
        /* ANI_CHANGEANIM / ANI_IFBUTTONS: an animation that ends by
           BECOMING another does not stop. Driven straight back through
           change_anim_label so the target gets its own header, exactly as
           if the dispatcher had selected it. */
        if (state->prog.ended && state->prog.become) {
            const char *next = state->prog.become;
            state->prog.become = NULL;
            state->prog.program = NULL;
            state->current_label = NULL;
            actor->anim_mode &=
                (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
            backend_change_anim_label(actor, next, state);
        }
        /* With a real animation running, MODE_BLOCK now leaves through the
           block animation's own ANI_SETPLYRMODE,MODE_NORMAL -- the stopgap
           below is only for a wrestler with no program for its block. */
        return;
    }

    /* The block animation's own ANI_WAITRELEASE,PLAYER_BLOCK_BIT followed by
       ANI_SETPLYRMODE,MODE_NORMAL -- see wm/wrestler_backend.h. */
    if (actor->player_mode == WM_PMODE_BLOCK &&
        !(actor->but_val_cur & WM_BTN_BLOCK)) {
        actor->player_mode = WM_PMODE_NORMAL;
    }
}

/*
 * WRESTLE2.ASM:3925 can_pin, which all eight dispatchers ask before
 * playing a pin animation and which nothing in this port ever
 * answered -- so no wrestler could pin anybody, and @p1pins /
 * @p2pins never moved, and the Undertaker's finishing move refused on
 * "this must be my second pin attempt" forever.
 *
 * The victim comes from the backend's own `opponent` rather than the
 * const pointer the callback is handed, because can_pin WRITES to him
 * (PINNED, WHOPINNEDME, three velocities, the KOD clear). They are the
 * same wrestler; only one of the two spellings can be assigned
 * through.
 */
static int backend_can_pin(wm_arcade_actor_t *actor,
                           const wm_arcade_actor_t *opp, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!st || !st->opponent) return 0;
    if (opp && opp != st->opponent) return 0;
    return wm_arcade_can_pin(actor, st->opponent,
                             st->all_actors, st->all_actor_count) ? 1 : 0;
}

/*
 * mode_waitanim's `call a0` (BRET.ASM:2550, copied verbatim into every
 * other wrestler file), for the whole shared backend.
 *
 * Every wrestler dispatcher already read CODE_ADDR here and handed it to
 * this callback; there was simply nothing on the other side of it, and
 * nothing wrote the field either, so WM_PMODE_WAITANIM was a state a
 * wrestler could only be put into and never leave. The three routines
 * that really do write CODE_ADDR are the climb deferrals in
 * WRESTLE2.ASM -- climb_turnbuckle (:200), ck_climb_out_side (:578) and
 * ck_climb_in_side (:702), see wm/arcade/wm_arcade_confine.h -- so the
 * token is a WmRingClimbContinuation and this is where it comes back
 * out.
 *
 * Unknown tokens are ignored rather than called: the source's `call a0`
 * would jump to whatever was there, which is not a behaviour worth
 * reproducing with a C function pointer.
 */
static void backend_code_addr(wm_arcade_actor_t *actor, uint32_t token,
                              void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    const char *label;

    if (!actor) return;
    switch ((WmRingClimbContinuation)token) {
    case WM_RING_CLIMB_CONT_TURNBUCKLE:
    case WM_RING_CLIMB_CONT_OUT_SIDE:
    case WM_RING_CLIMB_CONT_IN_SIDE:
        break;
    default:
        return;
    }

    label = wm_arcade_climb_continue(actor, (WmRingClimbContinuation)token);
    /* The continuation's own `calla change_anim1a`. */
    if (label) backend_change_anim_restart(actor, label, st);
    /* CODE_ADDR is not cleared by the source, but PLYRMODE has just
       left WAITANIM, so it is never read again until the next deferral
       overwrites it. Cleared here anyway: a stale token that a later
       SETMODE WAITANIM from somewhere else picked up would be a bug
       that looked like a climb. */
    actor->code_addr = 0;
}

/*
 * climb_turnbuckle (WRESTLE2.ASM:103), the callback every dispatcher's
 * mode_normal already called and nobody supplied -- so no wrestler on
 * this backend could climb a turnbuckle at all.
 *
 * The deferral is the same shape as the rope climbs': if he is not
 * already facing the corner (and #face_turnbuckle says which way that
 * is per wrestler -- four of the nine climb with their BACK to it), the
 * turn animation goes first and the climb itself waits in CODE_ADDR.
 */
static int backend_climb_turnbuckle(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    wm_climb_turnbuckle_result_t r;

    if (!actor || !st) return 0;
    r = wm_arcade_climb_turnbuckle(actor, st->all_actors,
                                   st->all_actor_count);
    if (!r.handled) return 0;          /* `clrc` */

    /* Every change_anim in WRESTLE2.ASM's climb routines -- :197, :211,
       :300, :359, :417, :431, :492, :575, :588, :699, :713 -- is
       change_anim1a. set_rotate_anim's own call is commented out at
       WRESTLE.ASM:5086, so the caller picks the entry point, and here
       it picks the unguarded one. */
    if (r.anim) {
        backend_change_anim_restart(actor, r.anim, st);
    } else if (r.rotate_then != WM_RING_CLIMB_CONT_NONE) {
        const char *turn = wm_wrestler_set_rotate_anim(
            actor, st->wrestler_num, actor->facing_dir);
        if (turn) backend_change_anim_restart(actor, turn, st);
        actor->code_addr = (uintptr_t)r.rotate_then;
    }
    return 1;                          /* `setc` */
}

/*
 * The three PLYRMODE handlers the whole roster shares
 * (wm/arcade/wm_arcade_modes.h). Every dispatcher called these three
 * seams and neither backend filled any of them, and unlike
 * keep_attached there is no fallback behind them -- so a wrestler who
 * entered MODE_PUPPET, MODE_INAIR2 or MODE_CHOKING never left it.
 */
static void backend_mode_puppet(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    wm_mode_puppet_result_t r;

    if (!actor || !st) return;
    r = wm_arcade_mode_puppet(actor, st->pcnt);
    /* The watchdog's own `calla change_anim1a`. It is the only thing
       this routine ever starts, and only when it has barked. */
    if (r.glitched_to_stand && r.stand_anim)
        backend_change_anim_restart(actor, r.stand_anim, st);
}

static void backend_mode_inair2(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    wm_arcade_mode_inair2(actor);
}

static void backend_mode_choking(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    /* The looping-sound kill it reports is DCSSOUND.ASM's, and this
       backend has no sound queue to send it to; the state change is
       applied to the actor either way, which is what gets him loose. */
    (void)wm_arcade_mode_choking(actor);
}

/*
 * bounce_off_ropes (WRESTLE.ASM:5115), the rope rebound every
 * wrestler's mode_running calls -- and, until now, the seam with
 * nothing behind it, so a running wrestler crossed the ropes and kept
 * going (wm/arcade/wm_arcade_bounce.h).
 */
static void backend_bounce_off_ropes(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    wm_bounce_result_t r;

    if (!actor || !st) return;
    r = wm_arcade_bounce_off_ropes(actor);
    /* `calla change_anim1a` on his own bounce animation. */
    if (r.bounced && r.bounce_anim)
        backend_change_anim_label(actor, r.bounce_anim, st);
}

/*
 * The auto-pin's return path and the victory pose
 * (wm/arcade/wm_arcade_auto_pin.h). All three seams sat empty in every
 * dispatcher, which mattered the moment auto_pin_check could actually
 * hand a human to the drone AI: nothing turned him back.
 */
static int backend_raisearm_check(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!actor || !st) return 0;
    return wm_arcade_raisearm_check(actor, st->all_actors, st->all_actor_count,
                                   st->anim_env.royal_rumble,
                                   /* raisearm_check only asks whether
                                      the queue is empty, and peeks
                                      without advancing it. */
                                   wm_final_queue_empty(
                                       st->anim_env.final_battle)) ? 1 : 0;
}

static void backend_set_raisearm_bit(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    wm_arcade_set_raisearm_bit(actor);
}

/*
 * DOINK.ASM:3316 bozo_check and DCSSOUND.ASM's FIND_AND_KILL_ENDLESS.
 *
 * Both seams were declared in wm/arcade/wm_arcade_roster.h and filled by
 * nobody. find_and_kill_endless is NULL-checked at fifteen call sites
 * across the eight dispatchers; bozo_check was worse off than that --
 * six of the eight dispatchers do not even call it, so the head-hold
 * power move and the head-held reversal were missing outright rather
 * than merely inert.
 */
/*
 * SOUND.H's WRSND, reached through the dispatchers' string seam.
 *
 * wm_arcade_roster_callbacks_t::sound_label was declared once and
 * assigned by nobody, so all fifty `snd(a,"...",c)` calls across the six
 * shared dispatchers were dropped. The tables behind it have been here
 * since the sound work (wm/wrestler_sound_tables.h is the whole of
 * WRSNDX, MASTER_SOUND_TABLE and its random sub-tables); what was
 * missing was the mnemonic-to-move-index step and this assignment.
 */
static void backend_sound_label(wm_arcade_actor_t *actor,
                                const char *source_label, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    wm_sndlabel_t s;

    if (!actor || !st || !source_label || !st->anim_env.sound) return;
    s = wm_wrsnd_label(source_label);
    switch (s.kind) {
    case WM_SNDLABEL_WRSND:
        (void)wm_wrsndx((int)actor->wrestler_num, s.move1, s.move2,
                        st->anim_env.rng, st->anim_env.sound_user,
                        st->anim_env.sound);
        break;
    case WM_SNDLABEL_FIXED:
        st->anim_env.sound(st->anim_env.sound_user, s.call);
        break;
    case WM_SNDLABEL_UNKNOWN:
        /* Nothing is invented for a name the table does not carry. A
           source-tools test walks every label the dispatchers use and
           refuses one that lands here, so this arm means the table is
           behind the callers rather than that the sound is silent. */
        break;
    }
}

/*
 * Razor's sound seam, which takes a typed id rather than a string --
 * his module is the one that selects by id everywhere. Every id maps to
 * a WRSND form read straight out of RAZOR.ASM's own calls.
 */
static const char *razor_sound_label(wm_arcade_razor_sound_id_t id) {
    switch (id) {
    case WM_RZR_SND_PUNCH:            return "PUNCH";
    case WM_RZR_SND_HDBUTT:           return "HDBUTT";
    case WM_RZR_SND_LBOWDROP:         return "LBOWDROP";
    case WM_RZR_SND_BLOCK_WOOSH:      return "BLOCK_WOOSH";
    case WM_RZR_SND_UPRCUT:           return "UPRCUT";
    /* `WRSND W_RAZOR,UPRCUT_T2` and `WRSND W_RAZOR,KICK_T2` -- the
       one-sound form, twice each in RAZOR.ASM. */
    case WM_RZR_SND_UPRCUT_T2:        return "UPRCUT_T2";
    case WM_RZR_SND_KICK:             return "KICK";
    case WM_RZR_SND_KICK_T2:          return "KICK_T2";
    case WM_RZR_SND_FLYKICK:          return "FLYKICK";
    case WM_RZR_SND_GRABHOLD:         return "GRABHOLD";
    /* `WRSND W_RAZOR,GRABFLING_T1,PUNCH_T2` -- the grab and then the
       punch, which is what the compound id is named after. */
    case WM_RZR_SND_GRABFLING_PUNCH:  return "GRABFLING_PUNCH";
    case WM_RZR_SND_PUSH:             return "PUSH";
    case WM_RZR_SND_TURNDIVE:         return "TURNDIVE";
    case WM_RZR_SND_NONE:             break;
    }
    return NULL;
}

static void backend_razor_sound(wm_arcade_actor_t *actor,
                                wm_arcade_razor_sound_id_t id, void *user) {
    backend_sound_label(actor, razor_sound_label(id), user);
}

/*
 * WRESTLE2.ASM:3253 ck_teammate_pin, reached through the dispatchers'
 * own seam rather than from the #raisearm branch's inline test.
 *
 * `wm_ck_teammate_pin` has been translated since the teammate work; the
 * seam in front of it was declared in three headers and filled by
 * nobody, so the first test in every wrestler's #raisearm branch
 * ("if a teammate has pinned, raise yer arm") always answered no.
 */
/*
 * WRESTLE.ASM:6044 ck_ignore_a8 -- "If player is moving away from
 * opponent, or standing still, tell the calling routine to ignore
 * button press". BRET.ASM:596 and DOINK.ASM:1399 both gate the flying
 * kick on it: you cannot launch one while backing off.
 *
 * wm_arcade_ck_ignore has been translated since the combat work, and
 * unlike keep_attached the call sites have NO fallback -- they simply
 * skip the test when the seam is empty, so the refusal never fired and
 * a wrestler could launch a flying kick while walking away.
 */
/*
 * WRESTLE.ASM:4851 check_secret_moves, for the seven wrestlers who are
 * not Bret.
 *
 * THIS IS A SECOND MECHANISM, not a second route into the one that
 * already works, and establishing that was the point of the trace this
 * came out of. The source has two:
 *
 *   - check_secret_moves (here) is the FIRST thing every move_xxx does,
 *     `movi xxx_secret_moves,a11 / calla check_secret_moves` before the
 *     mode table is even indexed. It walks that wrestler's own pattern
 *     table against wrest_joystat, a per-player ring buffer of
 *     (round_tickcount, joy+buttons) entries, and JUMPS to the matched
 *     entry's code. Button sequences: grab-fling, hip toss, ear slap.
 *
 *   - init_smoves (WRESTLE2.ASM) GETPRC_INSERTs one SMOVE_PID process
 *     per entry of xxx_smove_table at match start, and those watchdogs
 *     are what match_tick_smoves_for drives. Different table, different
 *     driver, different moves.
 *
 * Only Bret's half of the first one was wired. The seam was declared in
 * wm_arcade_roster.h and wm_arcade_razor.h, called at the top of all
 * seven other dispatchers, and filled by nobody -- so seven wrestlers
 * had no button-sequence secret moves at all, while their smove
 * monitors worked and made it look as though the mechanism was covered.
 *
 * The body is Bret's, generalised: his version is the same routine
 * against his own typed tables, and the parts that differ per wrestler
 * -- which button charges, for how long, and what each pattern fires --
 * are all data the profile and the dispatcher already carry.
 */
static uint16_t backend_charge_dtime(const wm_arcade_actor_t *actor,
                                     uint16_t button) {
    /* WRESTLE.ASM's get_punch_dtime and its siblings, by button. */
    switch (button) {
    case WM_BTN_PUNCH:  return actor->punch_dtime;
    case WM_BTN_BLOCK:  return actor->block_dtime;
    case WM_BTN_SPUNCH: return actor->powerp_dtime;
    case WM_BTN_KICK:   return actor->kick_dtime;
    case WM_BTN_SKICK:  return actor->powerk_dtime;
    default:            return 0;
    }
}

static void backend_check_secret_moves(
    wm_arcade_actor_t *actor, const wm_arcade_input_pattern_t *patterns,
    size_t count, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    const wm_arcade_wrestler_profile_t *profile;
    wm_arcade_wrestler_port_bindings_t bind;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    uint16_t now, charge_dtime;
    size_t i;

    if (!actor || !st || !patterns) return;
    profile = wm_arcade_roster_profile(
        (wm_arcade_roster_id_t)actor->wrestler_num);
    if (!profile) return;

    now = (uint16_t)st->pcnt;
    wm_arcade_joystat_update(&st->joystat, actor, now);

    /* Captured BEFORE update_joy_dtime, for the reason Bret's copy
       spells out: a release tick's dtime has to be the duration
       accumulated through the previous tick, not this tick's reset. */
    charge_dtime = backend_charge_dtime(actor, profile->charge_button);
    wm_arcade_update_joy_dtime(actor);

    /* WRESTLE.ASM:4853-4862, the four top-of-function gates. */
    if (actor->immobilize_time) return;
    if (actor->player_mode == WM_PMODE_DIZZY ||
        actor->player_mode == WM_PMODE_WAITANIM) return;
    if (actor->getup_time) return;

    memset(&bind, 0, sizeof(bind));
    roster_cb = wm_wrestler_roster_callbacks(st);
    razor_cb = wm_wrestler_razor_callbacks(st);
    bind.razor = &razor_cb;
    bind.taker = &roster_cb;
    bind.yoko = &roster_cb;
    bind.shawn = &roster_cb;
    bind.bam = &roster_cb;
    bind.doink = &roster_cb;
    bind.lex = &roster_cb;

    for (i = 0; i < count; ++i) {
        /*
         * `move *a11+,a0,L / call a0 / jrc #done` -- the table's FIRST
         * entry is executable code rather than a value/mask row, and it
         * is checked every tick and takes priority. In this port that
         * entry carries a NULL step list and its label is the charge's
         * own (charge_buzz, firepnch, charge_salt, ...), which is
         * exactly what wm_arcade_port_release_charge dispatches on.
         */
        if (!patterns[i].steps) {
            if ((actor->but_val_up & profile->charge_button) &&
                wm_arcade_port_release_charge(profile, actor, st->opponent,
                                              patterns[i].source_label,
                                              charge_dtime, &bind))
                return;
            continue;
        }

        /* "only check if newest entry in queue is fresh". Tested here
           rather than before the loop because the charge probe above
           does not depend on it -- the source checks it between the
           `call a0` and #next_table for the same reason. */
        if (st->joystat.entries[0].tickcount != now) return;

        /*
         * The two step structs are the same two uint16_t fields; the
         * matcher carries Bret's type name only because his secret
         * moves were translated first. Cast rather than duplicated so
         * there is one matcher and one place for its rules.
         */
        if (wm_arcade_joystat_matches(
                &st->joystat, now,
                (const wm_arcade_bret_sequence_step_t *)patterns[i].steps,
                patterns[i].step_count, patterns[i].max_ticks)) {
            (void)wm_arcade_port_fire_secret(profile, actor, st->opponent,
                                             patterns[i].source_label,
                                             (uint32_t)st->pcnt, &bind);
            return;
        }
    }
}

/*
 * Razor's copy. NOT a cast of the one above, though it started as one:
 * wm_arcade_razor_secret_pattern_t leads with a typed id where
 * wm_arcade_input_pattern_t leads with a source-label string, so the
 * two are the same SHAPE and different LAYOUT, and reinterpreting one
 * as the other would have read an enum as a pointer. The step rows
 * genuinely are identical and are still shared.
 *
 * His table also has no charge entry -- all six rows carry real steps,
 * and his two charge probes live outside it (RAZOR.ASM's
 * charge_flying_kick and rzr_charge_slashes, reached through
 * wm_arcade_port_release_charge) -- so the executable-first-entry arm
 * has nothing to do here.
 */
static void backend_razor_check_secret_moves(
    wm_arcade_actor_t *actor, const wm_arcade_razor_secret_pattern_t *patterns,
    size_t count, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    wm_arcade_razor_callbacks_t razor_cb;
    uint16_t now;
    size_t i;

    if (!actor || !st || !patterns) return;

    now = (uint16_t)st->pcnt;
    wm_arcade_joystat_update(&st->joystat, actor, now);
    wm_arcade_update_joy_dtime(actor);

    if (actor->immobilize_time) return;
    if (actor->player_mode == WM_PMODE_DIZZY ||
        actor->player_mode == WM_PMODE_WAITANIM) return;
    if (actor->getup_time) return;
    if (st->joystat.entries[0].tickcount != now) return;

    razor_cb = wm_wrestler_razor_callbacks(st);
    for (i = 0; i < count; ++i) {
        if (!patterns[i].steps) continue;
        if (wm_arcade_joystat_matches(
                &st->joystat, now,
                (const wm_arcade_bret_sequence_step_t *)patterns[i].steps,
                patterns[i].step_count, patterns[i].max_ticks)) {
            (void)wm_arcade_razor_fire_secret(actor, st->opponent,
                                              patterns[i].id,
                                              (uint32_t)st->pcnt, &razor_cb);
            return;
        }
    }
}

static int backend_ck_ignore(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    return wm_arcade_ck_ignore(actor) ? 1 : 0;
}

static int backend_teammate_pin(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!actor || !st) return 0;
    return wm_ck_teammate_pin(actor, st->all_actors, st->all_actor_count)
               ? 1 : 0;
}

/*
 * DCSSOUND.ASM:3534 DO_REVERSAL and LIFEBAR.ASM:3574 DO_REVERSAL_MESS,
 * the two calls every dispatcher makes when a head hold is reversed.
 *
 * DO_REVERSAL is FIND_AND_KILL_ENDLESS and then one announcer line drawn
 * from its REVERSAL table at 500 per mille. That table and that call row
 * are both already extracted -- src/generated/announce_tables.c carries
 * `{ "DO_REVERSAL", "REVERSAL", 0, 500, true }` -- and the ANI_CODE
 * dispatcher resolves the announcer group by NAME against that data, so
 * routing the seam through it plays exactly what the animation path
 * plays. Its SLEEP of 0 is why the comment beside announce_call says
 * DO_REVERSAL does the work inline.
 *
 * DO_REVERSAL_MESS is three things and this port has two of them: the
 * round award (`RND_AWARD a8,REVERSAL_AWD`), a voice line
 * (`movi 15Ch,a0 / calla ADD_VOICE`), and CREATE_REVERSAL_MESS, which
 * draws "REVERSAL" on the screen and is ledgered as display. The award
 * is the part that was silently missing: a reversal scored nothing.
 */
static void backend_do_reversal(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!actor || !st) return;
    (void)wm_anim_code_run(actor, &st->anim_env, "DO_REVERSAL", NULL, 0);
}

/* `movi 15Ch,a0 / calla ADD_VOICE`. */
#define WM_REVERSAL_VOICE 0x15Cu

static void backend_do_reversal_message(wm_arcade_actor_t *actor,
                                        void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!actor || !st) return;
    if (st->round_award)
        st->round_award(st->round_award_user, (int)actor->player_num,
                        (int)WM_AWARD_REVERSAL);
    if (st->anim_env.sound)
        st->anim_env.sound(st->anim_env.sound_user, WM_REVERSAL_VOICE);
}

static void backend_find_and_kill_endless(wm_arcade_actor_t *actor,
                                          void *user) {
    (void)actor;
    (void)user;
    wm_anim_code_find_and_kill_endless();
}

static int backend_bozo_check(wm_arcade_actor_t *actor, void *user) {
    wm_bozo_env_t env;
    (void)user;
    memset(&env, 0, sizeof(env));
    env.find_and_kill_endless = backend_find_and_kill_endless;
    return wm_arcade_bozo_check(actor, &env) ? 1 : 0;
}

static void backend_drone_change_back(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    (void)wm_arcade_drone_change_back(actor);
}

wm_arcade_roster_callbacks_t wm_wrestler_roster_callbacks(
    wm_wrestler_backend_actor *state) {
    wm_arcade_roster_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.execute_walk = backend_execute_walk;
    cb.adjust_health = backend_adjust_health;
    cb.mode_dead = backend_mode_dead;
    cb.check_combo_go = backend_check_combo_go;
    cb.change_anim_label = backend_change_anim_label;
    cb.change_anim_restart = backend_change_anim_restart;
    /*
     * The second channel. No dispatcher reaches either of these yet:
     * the only guarded change_anim2 call sites in the eight wrestler
     * files are BAM.ASM:2444, LEX.ASM:2263 and YOKO.ASM:2273, all in
     * mode_oppoverhead, and mode 10 is not translated in any
     * dispatcher. They are bound rather than left NULL so that when it
     * is, the seam is already the right shape -- and so that
     * change_torso_label, which was declared and never assigned by
     * anybody, stops being a field that only looks wired.
     */
    cb.change_torso_label = backend_change_anim2_label;
    cb.change_torso_restart = backend_change_anim2_restart;
    cb.sound_label = backend_sound_label;
    cb.can_pin = backend_can_pin;
    cb.code_addr = backend_code_addr;
    cb.climb_turnbuckle = backend_climb_turnbuckle;
    cb.mode_puppet = backend_mode_puppet;
    cb.mode_inair2 = backend_mode_inair2;
    cb.mode_choking = backend_mode_choking;
    cb.bounce_off_ropes = backend_bounce_off_ropes;
    cb.raisearm_check = backend_raisearm_check;
    cb.set_raisearm_bit = backend_set_raisearm_bit;
    cb.drone_change_back = backend_drone_change_back;
    cb.bozo_check = backend_bozo_check;
    cb.teammate_pin = backend_teammate_pin;
    cb.ck_ignore = backend_ck_ignore;
    cb.check_secret_moves = backend_check_secret_moves;
    cb.do_reversal = backend_do_reversal;
    cb.do_reversal_message = backend_do_reversal_message;
    cb.find_and_kill_endless = backend_find_and_kill_endless;
    cb.user = state;
    return cb;
}

/*
 * wm_arcade_razor_callbacks_t.change_anim.
 *
 * Razor is the one wrestler whose dispatcher selects by a typed id rather
 * than by the source's routine name, so his ids are turned back into those
 * names (src/core/arcade/wm_arcade_razor_anim_labels.c) and then take the
 * identical path the other six do.
 */
/*
 * wm_arcade_razor_callbacks_t.change_torso_anim -- the SECOND animation
 * channel, which Razor's dispatcher selects at two sites
 * (WM_RZR_ANIM_TORSO2 and TORSO4) and which was filled by nobody.
 *
 * The seam was invisible to the seam audit because it keys on a name
 * and Bret's copy of it IS filled, so `change_torso_anim` read as
 * covered while Razor's torso never moved. The tool reports that shape
 * as a heuristic now (declared in more structs than it has assignment
 * sites); this is one of the four it flagged.
 *
 * Nothing new is needed behind it: his ids already resolve to the
 * source's own routine names, and the shared backend already drives the
 * torso channel for everybody else.
 */
static void backend_razor_change_torso_anim(wm_arcade_actor_t *actor,
                                            wm_arcade_razor_anim_id_t id,
                                            void *user) {
    const char *label = wm_arcade_razor_anim_label(id);
    if (!actor || !label) return;
    /* RAZOR.ASM:1105 and :1119, razor_ani_init: change_anim2a. */
    backend_change_anim2_restart(actor, label, user);
}

static void backend_razor_change_anim(wm_arcade_actor_t *actor,
                                      wm_arcade_razor_anim_id_t id,
                                      void *user) {
    const char *label = wm_arcade_razor_anim_label(id);
    if (!actor || !label) return;
    /*
     * start_run_anim used to be special-cased here, calling
     * wm_arcade_start_run and returning. That was right about the
     * routine having no WL frames and wrong about what follows: the
     * source's #setup_run ENDS by selecting #run_anims[WRESTLERNUM] and
     * changing to it, and returning early meant Razor entered MODE
     * RUNNING and then stood there -- no run animation, and so no
     * ANI_ATTACK_ON,AMODE_RUN, which is the only thing that ever sets
     * that attack mode. The program path handles all of it now
     * (src/core/anim_code.c's #setup_run), so this takes the same
     * ordinary route as every other label.
     */
    backend_change_anim_label(actor, label, user);
}

/* RAZOR.ASM's change_anim1a call sites -- forty-nine of them against four
   guarded ones (:1510, :1560, :1740, :1966). */
static void backend_razor_change_anim_restart(wm_arcade_actor_t *actor,
                                              wm_arcade_razor_anim_id_t id,
                                              void *user) {
    const char *label = wm_arcade_razor_anim_label(id);
    if (!actor || !label) return;
    backend_change_anim_restart(actor, label, user);
}

wm_arcade_razor_callbacks_t wm_wrestler_razor_callbacks(
    wm_wrestler_backend_actor *state) {
    wm_arcade_razor_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.change_anim = backend_razor_change_anim;
    cb.change_anim_restart = backend_razor_change_anim_restart;
    cb.change_torso_anim = backend_razor_change_torso_anim;
    cb.sound = backend_razor_sound;
    cb.execute_walk = backend_execute_walk;
    cb.adjust_health = backend_adjust_health;
    cb.mode_dead = backend_mode_dead;
    cb.check_combo_go = backend_check_combo_go;
    cb.can_pin = backend_can_pin;
    cb.code_addr = backend_code_addr;
    cb.climb_turnbuckle = backend_climb_turnbuckle;
    cb.mode_puppet = backend_mode_puppet;
    cb.mode_inair2 = backend_mode_inair2;
    cb.mode_choking = backend_mode_choking;
    cb.bounce_off_ropes = backend_bounce_off_ropes;
    cb.raisearm_check = backend_raisearm_check;
    cb.set_raisearm_bit = backend_set_raisearm_bit;
    cb.drone_change_back = backend_drone_change_back;
    cb.bozo_check = backend_bozo_check;
    cb.teammate_pin = backend_teammate_pin;
    cb.ck_ignore = backend_ck_ignore;
    cb.check_secret_moves = backend_razor_check_secret_moves;
    cb.do_reversal = backend_do_reversal;
    cb.do_reversal_message = backend_do_reversal_message;
    cb.find_and_kill_endless = backend_find_and_kill_endless;
    cb.user = state;
    return cb;
}
