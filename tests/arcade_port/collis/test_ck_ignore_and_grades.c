/*
 * Two deferred seams whose notes described routines that do not exist.
 *
 * ck_ignore_reversed was recorded as "Razor's twin of ck_ignore, taking
 * BOTH wrestlers rather than one", with its source routine and second
 * argument unidentified and a warning that pointing it at the
 * one-argument translation "would be inventing the difference away".
 * There is no twin. WRESTLE.ASM:6016 ck_ignore and :6044 ck_ignore_a8
 * have byte-identical bodies and differ only in whether the wrestler is
 * in a13 or a8, and RAZOR.ASM:631 reaches the a8 form by swapping the
 * registers around the a13 call. The difference the note refused to
 * invent away WAS the invention.
 *
 * move_grade was recorded as reporting to nobody, "wireable once there
 * is a consumer that is not display". The consumer is the announcer
 * queue, and it was already built.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_combat.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_razor.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/announce_tables.h"
#include "wm/arcade/wm_arcade_announcer.h"
#include "wm/arcade/wmania_rng.h"

/* ---- ck_ignore is one routine with one argument ----------------- */

/*
 * mv_tbl (WRESTLE.ASM:6035) maps NEW_FACING_DIR to a BIT NUMBER tested
 * in MOVE_DIR:
 *
 *   facing 5 (up-left)  and 6 (down-left)   -> MOVE_RIGHT_BIT
 *   facing 9 (up-right) and 10 (down-right) -> MOVE_LEFT_BIT
 *
 * so the bit tested is always "away from the way he is facing", and set
 * means refuse. One actor decides it; there is no opponent in the
 * routine at all, which is the whole point of these two cases.
 */
static void test_ck_ignore_reads_one_actor(void) {
    wm_arcade_actor_t a;

    /* Facing up-right, moving left: away. Refuse. */
    memset(&a, 0, sizeof a);
    a.new_facing_dir = WM_MOVE_UP_RIGHT;
    a.move_dir = WM_MOVE_LEFT;
    assert(wm_arcade_ck_ignore(&a));

    /* Facing up-right, moving right: toward. Allow. */
    memset(&a, 0, sizeof a);
    a.new_facing_dir = WM_MOVE_UP_RIGHT;
    a.move_dir = WM_MOVE_RIGHT;
    assert(!wm_arcade_ck_ignore(&a));

    /* And mirrored, for the other half of the table. */
    memset(&a, 0, sizeof a);
    a.new_facing_dir = WM_MOVE_DOWN_LEFT;
    a.move_dir = WM_MOVE_RIGHT;
    assert(wm_arcade_ck_ignore(&a));

    memset(&a, 0, sizeof a);
    a.new_facing_dir = WM_MOVE_DOWN_LEFT;
    a.move_dir = WM_MOVE_LEFT;
    assert(!wm_arcade_ck_ignore(&a));

    /* Standing still: MOVE_DIR is 0, so no bit is set -- allow. The
       source's own comment says "moving away ... or standing still"
       would be ignored, but the code it describes tests a bit that a
       zero MOVE_DIR cannot have set. The code is what shipped. */
    memset(&a, 0, sizeof a);
    a.new_facing_dir = WM_MOVE_UP_RIGHT;
    a.move_dir = 0;
    assert(!wm_arcade_ck_ignore(&a));
}

/*
 * Razor's sliding rug is the only caller the port routes through the
 * seam, and it must ask about RAZOR -- the man who pressed kick -- not
 * his opponent. The invented seam was being called with the opponent
 * first, so this is the case that would have caught filling it as
 * written.
 */
static int rzr_ignore_calls;
static const wm_arcade_actor_t *rzr_ignore_subject;
static int cap_ck_ignore(wm_arcade_actor_t *a, void *u) {
    (void)u; ++rzr_ignore_calls; rzr_ignore_subject = a;
    return wm_arcade_ck_ignore(a) ? 1 : 0;
}
static void cap_rzr_anim(wm_arcade_actor_t *a, wm_arcade_razor_anim_id_t id,
                         void *u) { (void)a; (void)id; (void)u; }
static void cap_rzr_snd(wm_arcade_actor_t *a, wm_arcade_razor_sound_id_t s,
                        void *u) { (void)a; (void)s; (void)u; }

static void test_the_sliding_rug_asks_about_razor(void) {
    wm_arcade_actor_t a, o;
    wm_arcade_razor_callbacks_t cb;
    wm_arcade_razor_env_t e;

    memset(&e, 0, sizeof e);
    memset(&cb, 0, sizeof cb);
    cb.change_anim = cap_rzr_anim;
    cb.change_anim_restart = cap_rzr_anim;
    cb.sound = cap_rzr_snd;
    cb.ck_ignore = cap_ck_ignore;

    /*
     * Razor faces up-right and moves right -- toward the man. Allowed,
     * so the rug comes out. His OPPONENT is set up to answer the other
     * way, which is what makes this test about which actor is asked.
     */
    memset(&a, 0, sizeof a);
    memset(&o, 0, sizeof o);
    a.active = o.active = 1;
    a.wrestler_num = WM_ROSTER_RAZOR;
    a.player_mode = WM_PMODE_NORMAL;
    a.new_facing_dir = a.facing_dir = WM_MOVE_UP_RIGHT;
    a.move_dir = WM_MOVE_RIGHT;
    o.player_mode = WM_PMODE_NORMAL;
    o.new_facing_dir = WM_MOVE_UP_RIGHT;
    o.move_dir = WM_MOVE_LEFT;          /* would REFUSE if asked */
    rzr_ignore_calls = 0; rzr_ignore_subject = NULL;

    assert(wm_arcade_razor_fire_monitor(&a, &o, WM_RZR_MON_SLIDING_RUG,
                                        &e, 0, &cb) == 1);
    assert(rzr_ignore_calls == 1);
    assert(rzr_ignore_subject == &a);   /* Razor, not the opponent */

    /* Now Razor himself backs off: refused, whatever his opponent does. */
    a.move_dir = WM_MOVE_LEFT;
    o.move_dir = WM_MOVE_RIGHT;
    rzr_ignore_calls = 0; rzr_ignore_subject = NULL;
    assert(wm_arcade_razor_fire_monitor(&a, &o, WM_RZR_MON_SLIDING_RUG,
                                        &e, 0, &cb) == 0);
    assert(rzr_ignore_calls == 1);
    assert(rzr_ignore_subject == &a);
}

/* ---- the grades are announcer lines ---------------------------- */

/*
 * DCSSOUND.ASM:3484 CALL_AVERAGE_MOVE and :3689 CALL_NASTY_MOVE are two
 * instructions and a CREATE, and the process sleeps ten ticks and calls
 * ADD_IF_SILENT with a table and 500. Both tables and both caller rows
 * are generated data; the seam only had to reach them.
 */
static void test_both_grade_callers_are_real_announcer_rows(void) {
    const wm_announce_call *avg = wm_announce_call_find("CALL_AVERAGE_MOVE");
    const wm_announce_call *nas = wm_announce_call_find("CALL_NASTY_MOVE");
    const wm_announce_call *ani =
        wm_announce_call_find("CALL_ANI_AVERAGE_MOVE");

    assert(avg && nas && ani);
    /* `MOVI 500,A0` and `SLEEP 10` in both processes. */
    assert(avg->percent == 500 && nas->percent == 500);
    assert(avg->sleep == 10 && nas->sleep == 10);
    /* Both are personal, so A5 -- the wrestler number -- selects which
       of the per-wrestler voice tables a drawn row can reach. That is
       why which register each reads matters. */
    assert(avg->personal && nas->personal);

    /* Two DIFFERENT tables, with different crowd reactions. */
    assert(strcmp(avg->table, "AVERAGE_MOVE") == 0);
    assert(strcmp(nas->table, "NASTY_MOVE") == 0);
    assert(wm_announce_table_find("AVERAGE_MOVE") != NULL);
    assert(wm_announce_table_find("NASTY_MOVE") != NULL);

    /* CALL_ANI_AVERAGE_MOVE draws the SAME table as CALL_AVERAGE_MOVE.
       It exists only because it reads a13 where the other reads a10 --
       one form per register convention, exactly as ck_ignore and
       ck_ignore_a8 are, which is what makes the a10/a13 split above a
       reading rather than a guess. */
    assert(strcmp(ani->table, avg->table) == 0);
}

/*
 * And they really queue. This is the whole consumer the note said did
 * not exist.
 */
static void test_a_grade_queues_a_line(void) {
    const char *names[2] = { "CALL_AVERAGE_MOVE", "CALL_NASTY_MOVE" };
    unsigned i;

    for (i = 0; i < 2; ++i) {
        const wm_announce_call *call = wm_announce_call_find(names[i]);
        const wm_announce_table *t;
        wm_announcer_state an;
        wm_announce_ctx ctx;
        WmRng rng;

        assert(call != NULL);
        t = wm_announce_table_find(call->table);
        assert(t != NULL);

        wm_announcer_init(&an);
        wm_rng_init(&rng, 0x9abcu, NULL, NULL, NULL);
        memset(&ctx, 0, sizeof ctx);
        ctx.rng = &rng;
        ctx.wrestler_num = 0;
        assert(wm_announce_from_table(&an, t, call->percent, true, &ctx) >= 1);
        /* ADD_IF_SILENT declines while he is still talking. */
        assert(wm_announce_from_table(&an, t, call->percent, true, &ctx) == 0);
    }
}

int main(void) {
    test_ck_ignore_reads_one_actor();
    test_the_sliding_rug_asks_about_razor();
    test_both_grade_callers_are_real_announcer_rows();
    test_a_grade_queues_a_line();
    printf("ck_ignore and move-grade tests passed\n");
    return 0;
}
