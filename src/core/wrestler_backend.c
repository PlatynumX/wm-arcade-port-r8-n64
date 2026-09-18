#include "wm/wrestler_backend.h"
#include "wm/arcade/wm_arcade_confine.h"
#include "wm/arcade/wm_arcade_modes.h"
#include "wm/arcade/wm_arcade_bounce.h"
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
static void backend_change_anim2_label(wm_arcade_actor_t *actor,
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
 * wm_arcade_roster_callbacks_t.change_anim_label.
 *
 * The six label-based dispatchers have always passed the source's own
 * routine name here, and the program registry is keyed on exactly that, so
 * this is the whole join: look the label up, and run it.
 */
static void backend_change_anim_label(wm_arcade_actor_t *actor,
                                      const char *source_label, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    const wm_anim_program *prog;
    if (!actor || !st || !source_label) return;

    /* Selecting the animation already playing does not restart it -- the
       dispatchers call change_anim every tick they stay in the same mode. */
    if (st->current_label && st->prog.program && !st->prog.ended &&
        strcmp(st->current_label, source_label) == 0)
        return;

    prog = wm_anim_program_find(source_label);
    st->current_label = source_label;
    if (!prog) {
        /* A label with no generated program: the wrestler keeps whatever
           it was doing rather than freezing on a stale one. Which labels
           these are is a measured, reported number, not a guess -- see
           wm_wrestler_backend_program_coverage. */
        st->prog.program = NULL;
        st->prog.ended = true;
        return;
    }
    st->anim_env.opponent = st->opponent;
    st->anim_env.pcnt = st->pcnt;
    wm_anim_exec_start(&st->prog, prog, actor, (uint16_t)st->pcnt,
                       &st->anim_env);
}

/*
 * change_anim2 on the generic backend. Mirrors the primary path above,
 * including its "already playing this" guard, and differs in the two
 * ways the source differs: it writes the second channel, and starting
 * it does not reset gravity.
 */
static void backend_change_anim2_label(wm_arcade_actor_t *actor,
                                       const char *source_label, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    const wm_anim_program *prog;
    if (!actor || !st || !source_label) return;

    if (st->torso_label && st->torso_prog.program && !st->torso_prog.ended &&
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
    backend_change_anim_label(actor,
                              facing_right ? row->stand2 : row->stand4, state);
    backend_change_anim2_label(actor,
                               facing_right ? row->torso2 : row->torso4, state);
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
    if (label) backend_change_anim_label(actor, label, st);
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

    if (r.anim) {
        backend_change_anim_label(actor, r.anim, st);
    } else if (r.rotate_then != WM_RING_CLIMB_CONT_NONE) {
        const char *turn = wm_wrestler_set_rotate_anim(
            actor, st->wrestler_num, actor->facing_dir);
        if (turn) backend_change_anim_label(actor, turn, st);
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
        backend_change_anim_label(actor, r.stand_anim, st);
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

wm_arcade_roster_callbacks_t wm_wrestler_roster_callbacks(
    wm_wrestler_backend_actor *state) {
    wm_arcade_roster_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.execute_walk = backend_execute_walk;
    cb.adjust_health = backend_adjust_health;
    cb.mode_dead = backend_mode_dead;
    cb.check_combo_go = backend_check_combo_go;
    cb.change_anim_label = backend_change_anim_label;
    cb.can_pin = backend_can_pin;
    cb.code_addr = backend_code_addr;
    cb.climb_turnbuckle = backend_climb_turnbuckle;
    cb.mode_puppet = backend_mode_puppet;
    cb.mode_inair2 = backend_mode_inair2;
    cb.mode_choking = backend_mode_choking;
    cb.bounce_off_ropes = backend_bounce_off_ropes;
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

wm_arcade_razor_callbacks_t wm_wrestler_razor_callbacks(
    wm_wrestler_backend_actor *state) {
    wm_arcade_razor_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.change_anim = backend_razor_change_anim;
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
    cb.user = state;
    return cb;
}
