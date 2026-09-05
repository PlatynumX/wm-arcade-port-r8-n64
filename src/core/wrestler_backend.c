#include "wm/wrestler_backend.h"

#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_mode_dead.h"

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

void wm_wrestler_backend_tick(wm_wrestler_backend_actor *state,
                              wm_arcade_actor_t *actor) {
    (void)state;
    if (!actor) return;
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
