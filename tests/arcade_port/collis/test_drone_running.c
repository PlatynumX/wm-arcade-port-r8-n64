/*
 * A running drone, in the ring and out of it.
 *
 * DRONE.ASM:2294 drone_chkrun ("Check if running would be OK") and :2382
 * drn_run ("Control runner") each branch on INRING first. Both outside
 * arms were written off as unreachable while nobody could leave the
 * ring; confine_wrestler's #outring changed that, and the quiet-bout
 * work recorded them as reachable and untranslated. They are translated
 * now.
 *
 * And the in-ring arm of drn_run was read inside out. `cmpi
 * RING_X_CENTER+210,a3 / jrlt #rpok ;Won't hit R rope?` sends a runner
 * who is CLEAR of the rope to #rpok, where he considers a strike; the
 * port sent him to #rsk, which only steers. A running drone in the ring
 * therefore thought about hitting anybody only when it was about to
 * bounce. Measured over the twenty attract bouts: the longest unbroken
 * RUNNING spell fell from 3776 ticks to 194, and the quietest bout went
 * from one damage event to 263.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/match.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_drone_data.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wmania_ring_geometry.h"

static WmRng g_rng;
static wm_arcade_drone_callbacks_t g_cb;

static void actor_at(wm_arcade_actor_t *a, int32_t x, int32_t z)
{
    memset(a, 0, sizeof *a);
    a->active = 1;
    a->life = 163;
    a->in_ring = 1;
    a->x_int = x;
    a->z_int = z;
    a->facing_dir = WM_MOVE_RIGHT;
}

static int chkrun(wm_arcade_actor_t *self, wm_arcade_actor_t *opp)
{
    wm_arcade_drone_state_t d;
    wm_arcade_drone_init(&d, 15);
    return g_cb.script_call(self, opp, &d, "drone_chkrun", g_cb.user);
}

/*
 * drone_chkrun's #out (:2329): running is bad -- the script's run
 * buttons are skipped -- if it would carry him into the crowd (500 past
 * the ring's centre the way he faces), or, level with the ring in Z,
 * into the near side of the ring itself (within 300 short of centre).
 */
static void test_running_outside_the_ring_is_checked_for_walls(void)
{
    wm_arcade_actor_t self, opp;
    const int32_t c = WM_RING_X_CENTER, zmid = WM_RING_Z_CENTER;

    actor_at(&opp, c, zmid);
    opp.player_mode = WM_PMODE_NORMAL;

    /* Facing right. */
    actor_at(&self, c + 500, WM_RING_TOP - 100);
    self.in_ring = 0;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_SKIP_NEXT);   /* crowd */
    self.x_int = c + 499;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_CONTINUE);    /* above the ring */
    self.x_int = c - 100;
    self.z_int = zmid;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_SKIP_NEXT);   /* ring side */
    self.x_int = c - 301;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_CONTINUE);    /* far enough */
    self.x_int = c;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_CONTINUE);    /* past centre */
    self.x_int = c - 100;
    self.z_int = WM_RING_BOT + 11;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_CONTINUE);    /* below the band */

    /* Facing left: the mirror. */
    self.facing_dir = WM_MOVE_LEFT;
    self.z_int = zmid;
    self.x_int = c - 500;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_SKIP_NEXT);
    self.x_int = c + 100;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_SKIP_NEXT);
    self.x_int = c + 301;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_CONTINUE);
    self.x_int = c;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_CONTINUE);

    /* In the ring the old Z/X test still applies, and these positions
       would have been "bad" outside but are fine in here. */
    self.in_ring = 1;
    self.facing_dir = WM_MOVE_RIGHT;
    self.x_int = c - 100;
    self.closest_zdist = 200;
    self.closest_xdist = 400;
    assert(chkrun(&self, &opp) == WM_DRONE_CALL_CONTINUE);
}

/* A runner and his opponent, set up for drn_run. */
static void runner(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                   int32_t sx, int32_t ox, int32_t zdist, int in_ring)
{
    actor_at(self, sx, 1200);
    actor_at(opp, ox, 1200 + zdist);
    self->player_mode = WM_PMODE_RUNNING;
    self->in_ring = in_ring;
    opp->in_ring = in_ring;
    opp->player_mode = WM_PMODE_NORMAL;
    self->x_vel = 2 << 16;                  /* running right */
    self->closest_zdist = zdist;
    self->closest_xdist = ox > sx ? ox - sx : sx - ox;
}

static int run_once(wm_arcade_actor_t *self, wm_arcade_actor_t *opp,
                    wm_arcade_drone_state_t *d)
{
    /* drn_run opens with a 1-in-512 breakout roll. Fix the RNG so the
       roll is the same on every run of this test -- and skip the first
       draw after seeding, which is always 0 and so would always break
       out. */
    wm_rng_init(&g_rng, 3u, 0, 0, 0);
    (void)wm_rng_rndrng0(&g_rng, 0x1ffu);
    wm_arcade_drone_init(d, 15);
    d->script = "drn_run";
    d->script_mode = WM_PMODE_RUNNING;
    return g_cb.script_call(self, opp, d, "drn_run", g_cb.user);
}

/*
 * drn_run outside the ring (:2412). Too far from the opponent -> #ering;
 * running away -> #brkrun; Z too far -> #brkseek; otherwise on to #cont.
 * #ering and #brkseek both tap away for two ticks and then DS_JMP.
 */
static void test_running_outside_the_ring_heads_back_or_seeks(void)
{
    wm_arcade_actor_t self, opp;
    wm_arcade_drone_state_t d;
    const wm_arcade_drone_script_t *run = g_cb.resolve_script("drn_run", g_cb.user);
    assert(run != NULL);

    /* #ering: 400 away, and the opponent is in the ring -- otherwise
       drn_enterring's own "Opp out?" guard would end it at once. */
    runner(&self, &opp, 900, 1300, 0, 0);
    opp.in_ring = 1;
    assert(run_once(&self, &opp, &d) == WM_DRONE_CALL_REDIRECTED);
    assert(d.script != NULL && strcmp(d.script, "drn_run") == 0);
    assert(d.script_pc == 1u);
    /* ...and stepping it plays `.word L_M,2` -- backwards for a man
       facing right -- then jumps into drn_enterring. */
    wm_rng_init(&g_rng, 3u, 0, 0, 0);
    (void)wm_rng_rndrng0(&g_rng, 0x1ffu);
    wm_arcade_drone_init(&d, 15);
    d.script = "drn_run";
    d.script_mode = WM_PMODE_RUNNING;
    (void)wm_arcade_drone_script_step(&self, &opp, &d, run, &g_cb);
    assert(d.joy == WM_MOVE_LEFT);
    assert(d.delay == 2);
    assert(d.script != NULL && strcmp(d.script, "drn_run") == 0);
    (void)wm_arcade_drone_script_step(&self, &opp, &d, run, &g_cb);
    assert(d.script != NULL && strcmp(d.script, "drn_enterring") == 0);

    /* #brkrun: close, but running away from him. */
    runner(&self, &opp, 1000, 900, 0, 0);
    assert(run_once(&self, &opp, &d) == WM_DRONE_CALL_ABORT);
    assert(d.joy == WM_MOVE_LEFT);
    assert(d.but == 0);

    /* #brkseek: close in X, toward him, but 50 off in Z. */
    runner(&self, &opp, 900, 1000, 50, 0);
    assert(run_once(&self, &opp, &d) == WM_DRONE_CALL_REDIRECTED);
    assert(d.script_pc == 3u);

    /* On to #cont and a strike: close in both. */
    runner(&self, &opp, 900, 960, 10, 0);
    assert(run_once(&self, &opp, &d) == WM_DRONE_CALL_ABORT);
    assert(d.but != 0);
}

/*
 * drn_run in the ring (:2426). Clear of the rope, close to a standing
 * opponent: #rpok, then #cont, and he strikes. The old reading sent this
 * exact case to #rsk and pressed nothing.
 */
static void test_a_runner_clear_of_the_rope_considers_a_strike(void)
{
    wm_arcade_actor_t self, opp;
    wm_arcade_drone_state_t d;

    runner(&self, &opp, 1000, 1060, 10, 1);
    assert(self.x_int + 16 < WM_RING_X_CENTER + 210);   /* clear of the rope */
    assert(run_once(&self, &opp, &d) == WM_DRONE_CALL_ABORT);
    assert(d.but == WM_BTN_KICK || d.but == WM_BTN_SKICK ||
           d.but == WM_BTN_SPUNCH);
}

/*
 * #chkopp into #oprun for a standing opponent: about to hit the rope,
 * opponent within 180 in X and under 90 in Z -> #brkrun. The old reading
 * went straight to #rpok for a standing opponent and skipped the Z test,
 * so this case steered instead of breaking off.
 */
static void test_a_runner_at_the_rope_breaks_off_near_a_standing_man(void)
{
    wm_arcade_actor_t self, opp;
    wm_arcade_drone_state_t d;
    const int32_t rope = WM_RING_X_CENTER + 210;

    runner(&self, &opp, rope - 10, rope - 110, 50, 1);
    assert(run_once(&self, &opp, &d) == WM_DRONE_CALL_ABORT);
    assert(d.joy == WM_MOVE_LEFT);
    assert(d.but == 0);
}

int main(void)
{
    wm_rng_init(&g_rng, 3u, 0, 0, 0);
    g_cb = wm_arcade_drone_data_callbacks(&g_rng);
    test_running_outside_the_ring_is_checked_for_walls();
    test_running_outside_the_ring_heads_back_or_seeks();
    test_a_runner_clear_of_the_rope_considers_a_strike();
    test_a_runner_at_the_rope_breaks_off_near_a_standing_man();
    printf("drone running: #out, #ering, #brkseek, #brkrun, #rpok, #oprun\n");
    return 0;
}
