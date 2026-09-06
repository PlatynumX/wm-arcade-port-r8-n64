#include "wm/wrestler_backend.h"

#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_mode_dead.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
#include "wm/bret_backend.h"

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

static void backend_execute_walk(wm_arcade_actor_t *actor, void *user) {
    wm_wrestler_backend_actor *st = (wm_wrestler_backend_actor *)user;
    if (!actor || !st) return;
    wm_execute_walk(actor, st->opponent, wm_wrestler_velocity_table(st->wrestler_num));
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

void wm_wrestler_backend_tick(wm_wrestler_backend_actor *state,
                              wm_arcade_actor_t *actor) {
    if (!actor) return;

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

wm_arcade_roster_callbacks_t wm_wrestler_roster_callbacks(
    wm_wrestler_backend_actor *state) {
    wm_arcade_roster_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.execute_walk = backend_execute_walk;
    cb.adjust_health = backend_adjust_health;
    cb.mode_dead = backend_mode_dead;
    cb.check_combo_go = backend_check_combo_go;
    cb.change_anim_label = backend_change_anim_label;
    cb.user = state;
    return cb;
}

wm_arcade_razor_callbacks_t wm_wrestler_razor_callbacks(
    wm_wrestler_backend_actor *state) {
    wm_arcade_razor_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.execute_walk = backend_execute_walk;
    cb.adjust_health = backend_adjust_health;
    cb.mode_dead = backend_mode_dead;
    cb.check_combo_go = backend_check_combo_go;
    cb.user = state;
    return cb;
}
