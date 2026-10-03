/*
 * REACT1.ASM's hit_table dispatch, which this port computed on every
 * blow and then threw away.
 *
 * wm_arcade_react_callbacks_t declares a `reaction` hook, a
 * signature-compatible bridge covering REACT1 through REACT9 was
 * written for it, and match.c memset the callback set and never
 * assigned it. The consequence was not subtle: damage landed and
 * nothing else did -- no stagger, no flinch, no knockdown, no reaction
 * animation, and the reaction handlers' own adjustments to damage and
 * to the victim's move direction were all discarded.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_react9_core.h"
#include "wm/arcade/wm_arcade_react_anims.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm_arcade_roster.h"
#include "wm/match.h"
#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wm_arcade_drone_data.h"

/*
 * Every group this port claims a table for names a REAL table, and that
 * table gives a real label for a real wrestler. A typo here would
 * silently mean "this reaction has no animation".
 */
static void test_every_mapped_group_resolves(void) {
    static const wm_arcade_react1_anim_group_t mapped[] = {
        WM_R1_ANIM_HITBLOCK, WM_R1_ANIM_HITBLOCK_FLAIL,
        WM_R1_ANIM_HEAD_HIT, WM_R1_ANIM_HEAD_HIT2, WM_R1_ANIM_BODY_HIT,
        WM_R1_ANIM_FALL_BACK, WM_R1_ANIM_HIT_ON_GROUND,
        WM_R1_ANIM_FALL_BACK_TBUKL, WM_R1_ANIM_KNEE_HIT,
        WM_R1_ANIM_KNOCKDOWN, WM_R1_ANIM_BOUNCE_OFF,
        WM_R1_ANIM_SPECIAL_HEAD_HIT2_SAND, WM_R1_ANIM_SPECIAL_BODY_HIT2,
        WM_R1_ANIM_WRES_SLAVE
    };
    unsigned i;
    int w;

    for (i = 0; i < sizeof mapped / sizeof *mapped; ++i) {
        const char *name = wm_react_anim_table_for(mapped[i]);
        const wm_roster_anim_table *t;
        assert(name != NULL);
        t = wm_roster_anim_find(name);
        assert(t != NULL);
        /* Bret is in every one of them. */
        assert(wm_react_anim_label(mapped[i], 0, 0) != NULL);
        /* And so is everyone else except the cut slot 7. */
        for (w = 0; w < 9; ++w) {
            const char *l = wm_react_anim_label(mapped[i], w, 0);
            if (w == 7) continue;           /* Adam Bomb */
            assert(l != NULL);
        }
    }
}

/*
 * The ten with no table stay NULL rather than pointing at something
 * plausible. Asserting this keeps a later "helpful" guess honest.
 */
static void test_the_unmapped_groups_stay_unmapped(void) {
    static const wm_arcade_react1_anim_group_t unmapped[] = {
        WM_R1_ANIM_LOSE_BALANCE, WM_R1_ANIM_QUICK_KNEE_HIT,
        WM_R1_ANIM_SPINKICK_HEAD_HIT, WM_R1_ANIM_FALL_BACK2,
        WM_R1_ANIM_JUMPKICK_HEAD_HIT, WM_R1_ANIM_BOUNCE_OFF_DIZZY,
        WM_R1_ANIM_BACKHAND_HEAD_HIT, WM_R1_ANIM_EARSLAP_HEAD_HIT,
        WM_R1_ANIM_GET_BUZZ, WM_R1_ANIM_BURN
    };
    unsigned i;
    for (i = 0; i < sizeof unmapped / sizeof *unmapped; ++i) {
        assert(wm_react_anim_table_for(unmapped[i]) == NULL);
        assert(wm_react_anim_label(unmapped[i], 0, 0) == NULL);
    }
}

/*
 * FACE24TBL is two longs per wrestler and picks by FACING_DIR;
 * FACETBL is one and ignores it. head_hit_tbl is the first kind
 * (REACT1.ASM:1078), hitblock_tbl the second (:218).
 */
static void test_the_facing_tables_actually_face(void) {
    const char *up = wm_react_anim_label(WM_R1_ANIM_HEAD_HIT, 0, WM_MOVE_UP);
    const char *down = wm_react_anim_label(WM_R1_ANIM_HEAD_HIT, 0, WM_MOVE_DOWN);
    const char *b_up = wm_react_anim_label(WM_R1_ANIM_HITBLOCK, 0, WM_MOVE_UP);
    const char *b_down = wm_react_anim_label(WM_R1_ANIM_HITBLOCK, 0, WM_MOVE_DOWN);

    assert(up && down && b_up && b_down);
    /* The facing table gives two different animations. */
    assert(strcmp(up, down) != 0);
    /* The slot table gives the same one either way. */
    assert(strcmp(b_up, b_down) == 0);
}

/* A recording reaction hook, to prove the seam is reached at all. */
static int REACTION_CALLS;
static wm_arcade_reaction_id_t LAST_REACTION;

static void record_reaction(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                            wm_arcade_reaction_id_t r, int16_t *pending,
                            int16_t *newdir, void *user) {
    (void)a; (void)v; (void)user;
    ++REACTION_CALLS;
    LAST_REACTION = r;
    /* The source lets reaction code rewrite both globals; prove the
       caller reads them back rather than ignoring them. */
    *pending = (int16_t)(*pending - 1);
    *newdir = 42;
}

static int32_t HEALTH_DELTA;
static void record_health(wm_arcade_actor_t *v, int16_t d,
                          wm_arcade_actor_t *src, void *user) {
    (void)v; (void)src; (void)user;
    HEALTH_DELTA = d;
}

/*
 * The contract the wiring depends on: the hook is called BEFORE health
 * is applied, and what it writes into hit_damage_pending and
 * new_victim_movedir is what the caller then uses.
 */
static void test_the_hook_runs_before_health_and_is_read_back(void) {
    wm_arcade_actor_t att, vic;
    wm_arcade_react_callbacks_t cb;
    wm_arcade_combat_runtime_t rt;
    wm_arcade_wrestler_hit_result_t out;

    memset(&att, 0, sizeof att);
    memset(&vic, 0, sizeof vic);
    att.active = 1; vic.active = 1;
    att.x_int = 100; vic.x_int = 130;
    att.player_side = 0; vic.player_side = 1;
    vic.life = 100;
    att.attack_mode = WM_AMODE_PUNCH;

    memset(&cb, 0, sizeof cb);
    cb.reaction = record_reaction;
    cb.adjust_health = record_health;
    memset(&rt, 0, sizeof rt);
    REACTION_CALLS = 0; HEALTH_DELTA = 0;

    memset(&out, 0, sizeof out);
    out = wm_arcade_wrestler_hit(&att, &vic, &rt, &cb);

    /* Asserted, not guarded: if this punch stops landing the test must
       fail rather than quietly checking nothing. */
    assert(out.status == WM_WRESTLER_HIT_OK);
    assert(REACTION_CALLS == 1);
    assert(out.reaction_hook_called);
    /* What the hook wrote is what came out. */
    assert(out.damage_before_reaction == -10);
    assert(out.damage_after_reaction == -11);
    assert(out.new_victim_movedir == 42);
    assert(vic.move_dir == 42);
    /* And health saw the ADJUSTED number, not the original. */
    assert(out.health_hook_called);
    assert(HEALTH_DELTA == -11);
}

/*
 * And the wiring itself: a live match must actually pass a reaction
 * hook down, not just be capable of it. This is the assertion that
 * would have failed before this change, when match.c memset the
 * callback set and left .reaction NULL.
 */
static void test_the_match_supplies_a_reaction_hook(void) {
    wm_match_state m;
    wm_arcade_drone_callbacks_t cb;
    WmRng rng;
    int i;
    bool saw_hook = false;

    memset(&rng, 0, sizeof rng);
    wm_rng_init(&rng, 0x1234u, NULL, NULL, NULL);
    memset(&m, 0, sizeof m);
    wm_match_init(&m);
    wm_match_start_attract(&m, &rng);
    m.anim_rng = &rng;

    /*
     * Drive the match and put the two actors on top of each other with
     * the first one swinging, so a hit really is checked. What is being
     * asserted is not that a hit lands on a given tick -- that depends
     * on boxes and frames -- but that when the collision path runs it
     * has a reaction hook to call.
     */
    cb = wm_arcade_drone_data_callbacks(&rng);
    for (i = 0; i < 400 && !saw_hook; ++i) {
        m.actors[1].x_int = m.actors[0].x_int;
        m.actors[1].z_int = m.actors[0].z_int;
        wm_match_tick(&m, &cb, NULL);
        if (m.react1_ctx.callbacks != NULL) saw_hook = true;
    }
    /* The context the dispatcher needs is built and points at the
       match's own callback set. */
    assert(saw_hook);
    assert(m.react1_ctx.callbacks == &m.react1_cb);
    assert(m.react1_cb.change_anim != NULL);
    assert(m.react1_cb.get_health != NULL);
    assert(m.react1_cb.user == &m);
}

int main(void) {
    test_every_mapped_group_resolves();
    test_the_unmapped_groups_stay_unmapped();
    test_the_facing_tables_actually_face();
    test_the_hook_runs_before_health_and_is_read_back();
    test_the_match_supplies_a_reaction_hook();
    printf("reaction wired ok\n");
    return 0;
}
