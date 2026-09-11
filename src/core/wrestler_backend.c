#include "wm/wrestler_backend.h"
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
        int old_compass = wm_convert_facing(old_facing_dir);
        int new_compass = wm_convert_facing(actor->new_facing_dir);
        const char *turn = slot_label(
            wm_wrestler_rotate_anims, st->wrestler_num,
            old_compass >= 0 ? old_compass >> 1 : -1,
            new_compass >= 0 ? new_compass >> 1 : -1);
        if (turn) backend_change_anim_label(actor, turn, st);
    }
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
    (void)user;
    wm_arcade_mode_dead(actor);
}

/* LIFEBAR.ASM's CHECK_COMBO_GO: this port never tracks per-player
   combo-meter fill at all, so it always reports "not lit" (negative) --
   see wm/arcade/wm_arcade_mode_dead.h's own already-established finding,
   and wm_arcade_bret_drone.c's identical gate for DRONE.ASM's drn_combo. */
static int backend_check_combo_go(wm_arcade_actor_t *actor, void *user) {
    (void)actor; (void)user;
    return -1;
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
    /* WRESTLE2.ASM:3443 start_run_anim is state setup with no WL frames of
       its own, so there is no program to run -- what it does IS the setup.
       Bret's backend takes the same fork for the same reason. */
    if (strcmp(label, "start_run_anim") == 0) {
        wm_arcade_start_run(actor);
        actor->anim_mode |= (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
        return;
    }
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
    cb.user = state;
    return cb;
}
