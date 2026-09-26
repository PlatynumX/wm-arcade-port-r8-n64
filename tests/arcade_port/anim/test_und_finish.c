/*
 * TAKER.ASM:527 und_finish_move1 -- the game's only finishing move.
 *
 * Every wrestler's smove table carries finishing-move entries, and
 * GAME.EQU:580-587 turns seven of the eight switches off. The
 * assembler skipped fifteen of the sixteen *_finish_move routines
 * along with their table entries; it built this one, and the two
 * helpers inside the same `.if` block.
 */
#include "wm/arcade/wm_arcade_und_finish.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int in_finish;
    int shakes_started;
    int shakes_killed;
    int origin_writes;
    int32_t last_tlx, last_tly;
} spy;

static void on_finish(int on, void *user) { ((spy *)user)->in_finish = on; }
static void on_origin(int32_t x, int32_t y, void *user) {
    spy *s = (spy *)user;
    s->last_tlx = x; s->last_tly = y; ++s->origin_writes;
}
static void on_shake(void *user) { ++((spy *)user)->shakes_started; }
static void on_kill(void *user) { ++((spy *)user)->shakes_killed; }

static void setup(wm_arcade_actor_t *taker, wm_arcade_actor_t *victim,
                  wm_arcade_und_finish_env_t *env)
{
    memset(taker, 0, sizeof(*taker));
    memset(victim, 0, sizeof(*victim));
    memset(env, 0, sizeof(*env));
    taker->life = 163;
    taker->x_int = WM_RING_X_CENTER;
    taker->x_fixed = (int32_t)taker->x_int << 16;
    victim->x_int = WM_RING_X_CENTER;
    victim->player_mode = WM_PMODE_DEAD;
    env->my_pins = 2;
    env->player_type = 0;
    env->ring_time = 10;
}

static void test_all_seven_guards(void)
{
    wm_arcade_actor_t t, v;
    wm_arcade_und_finish_env_t e;

    setup(&t, &v, &e);
    assert(wm_arcade_und_finish_allowed(&t, &v, &e));

    /* The opponent has to be dead. */
    setup(&t, &v, &e); v.player_mode = WM_PMODE_NORMAL;
    assert(!wm_arcade_und_finish_allowed(&t, &v, &e));

    /* Second pin attempt: `cmpi 2,a0 / jrlt` is two OR MORE. */
    setup(&t, &v, &e); e.my_pins = 1;
    assert(!wm_arcade_und_finish_allowed(&t, &v, &e));
    setup(&t, &v, &e); e.my_pins = 3;
    assert(wm_arcade_und_finish_allowed(&t, &v, &e));

    /* Not once the autopin has made him a drone. */
    setup(&t, &v, &e); e.player_type = 1;
    assert(!wm_arcade_und_finish_allowed(&t, &v, &e));

    /* RING_TIME's sign is the test, so 0 is still inside. */
    setup(&t, &v, &e); e.ring_time = -1;
    assert(!wm_arcade_und_finish_allowed(&t, &v, &e));
    setup(&t, &v, &e); e.ring_time = 0;
    assert(wm_arcade_und_finish_allowed(&t, &v, &e));

    /* Not if he already pinned this opponent. */
    setup(&t, &v, &e); t.status_flags |= WM_STATUS_DID_PIN;
    assert(!wm_arcade_und_finish_allowed(&t, &v, &e));

    /* Neither may be right of centre+100. `jrgt` is strictly greater,
       so the boundary itself is allowed. */
    setup(&t, &v, &e); v.x_int = WM_UND_FINISH_MAX_X;
    assert(wm_arcade_und_finish_allowed(&t, &v, &e));
    setup(&t, &v, &e); v.x_int = WM_UND_FINISH_MAX_X + 1;
    assert(!wm_arcade_und_finish_allowed(&t, &v, &e));
    setup(&t, &v, &e); t.x_int = WM_UND_FINISH_MAX_X + 1;
    t.x_fixed = t.x_int << 16;
    assert(!wm_arcade_und_finish_allowed(&t, &v, &e));

    /* It is a SIGNED compare: far to the left is fine. */
    setup(&t, &v, &e); t.x_int = -400; v.x_int = -400;
    t.x_fixed = t.x_int << 16;
    assert(wm_arcade_und_finish_allowed(&t, &v, &e));
}

static void test_beginning_it(void)
{
    wm_arcade_actor_t t, v;
    wm_arcade_und_finish_env_t e;
    wm_arcade_und_finish_callbacks_t cb;
    spy s;
    const char *anim;

    setup(&t, &v, &e);
    memset(&s, 0, sizeof(s));
    memset(&cb, 0, sizeof(cb));
    cb.set_in_finish_move = on_finish;
    cb.set_world_origin = on_origin;
    cb.start_shake = on_shake;
    cb.kill_shake = on_kill;
    cb.user = &s;

    /* The victim keeps his buckoff until the move actually starts. */
    v.status_flags |= WM_STATUS_DO_BUCKOFF;

    anim = wm_arcade_und_finish_move1(&t, &v, &e, &cb);
    assert(anim != NULL);
    assert(strcmp(anim, "und_2_raise_dead_anim") == 0);
    assert(s.in_finish == 1);
    /* "clear victim's DO_BUCKOFF bit and set his NO_BUCKOFF bit" */
    assert(!(v.status_flags & WM_STATUS_DO_BUCKOFF));
    assert(v.status_flags & WM_STATUS_NO_BUCKOFF);
    /* adjust_view ran its 32 steps and started the shaker. */
    assert(s.origin_writes == 32);
    assert(s.shakes_started == 1);

    /* A refused move does none of it. */
    setup(&t, &v, &e);
    memset(&s, 0, sizeof(s));
    e.my_pins = 0;
    assert(wm_arcade_und_finish_move1(&t, &v, &e, &cb) == NULL);
    assert(s.in_finish == 0);
    assert(s.origin_writes == 0);
    assert(s.shakes_started == 0);
}

static void test_adjust_view_stops_near_the_edge(void)
{
    wm_arcade_actor_t t, v;
    wm_arcade_und_finish_env_t e;
    wm_arcade_und_finish_callbacks_t cb;
    spy s;
    int steps;

    setup(&t, &v, &e);
    memset(&s, 0, sizeof(s));
    memset(&cb, 0, sizeof(cb));
    cb.set_world_origin = on_origin;
    cb.start_shake = on_shake;
    cb.user = &s;

    /* Already within 100 of the ring's right edge: no scroll at all --
       but the shaker still starts, because the source's early exit
       jumps to #av_exit, which is where the CREATE lives. */
    e.world_tlx = (int32_t)WM_UND_VIEW_STOP_X << 16;
    steps = wm_arcade_und_adjust_view(&t, &e, &cb);
    assert(steps == 0);
    assert(s.origin_writes == 0);
    assert(s.shakes_started == 1);

    /* Further left: the full 32 steps, moving toward the midpoint
       between the Undertaker and x=1200. */
    memset(&s, 0, sizeof(s));
    e.world_tlx = 0;
    t.x_int = 200;
    t.x_fixed = 200 << 16;
    steps = wm_arcade_und_adjust_view(&t, &e, &cb);
    assert(steps == 32);
    assert(s.origin_writes == 32);
    /* 32 steps of ((mid - x) >> 5) lands on the difference itself. */
    assert(s.last_tlx == ((((200 + 1200) << 16) >> 1) - (200 << 16))
                         / 32 * 32);
    assert(s.shakes_started == 1);
}

static void test_the_shake_is_about_a_fixed_point(void)
{
    wm_arcade_und_finish_callbacks_t cb;
    WmRng rng;
    spy s;
    int i;
    const int32_t base_x = 100 << 16, base_y = 50 << 16;

    memset(&s, 0, sizeof(s));
    memset(&cb, 0, sizeof(cb));
    memset(&rng, 0, sizeof(rng));
    cb.set_world_origin = on_origin;
    cb.rng = &rng;
    cb.user = &s;

    /* RNDRNG0(4) is 0..4 and the offset is 2 minus it, so every write
       lands within two pixels of the base on each axis -- and it is
       the BASE, not the last position, so it cannot wander. */
    for (i = 0; i < 50; ++i) {
        wm_arcade_und_shake_world(base_x, base_y, &cb);
        assert(s.last_tlx >= base_x - (2 << 16));
        assert(s.last_tlx <= base_x + (2 << 16));
        assert(s.last_tly >= base_y - (2 << 16));
        assert(s.last_tly <= base_y + (2 << 16));
    }
    assert(s.origin_writes == 50);
}

int main(void)
{
    test_all_seven_guards();
    test_beginning_it();
    test_adjust_view_stops_near_the_edge();
    test_the_shake_is_about_a_fixed_point();
    printf("Undertaker's finishing move: all checks passed\n");
    return 0;
}
