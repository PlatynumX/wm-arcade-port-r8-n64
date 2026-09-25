/*
 * The last of the deferred seams, and a measurement that replaces one.
 *
 * partner_breakout was ledgered as "the routine behind it has not been
 * traced". It is REACT1.ASM:746 and :753, two instructions and a label
 * apiece, selecting two GENERIC animations the port had already
 * extracted and never used.
 *
 * unhandled_reaction was ledgered as "diagnostic rather than behaviour.
 * Worth filling with a counter so the number is measured instead of
 * assumed." The number is measured here, and it is zero -- which is a
 * better answer than a counter, because it says WHY.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_react1_core.h"
#include "wm/arcade/wm_arcade_react4_core.h"
#include "wm/anim_program.h"

/*
 * Every value of wm_arcade_reaction_id_t, driven through the chain the
 * match actually installs -- all nine REACT files. Nothing is left over.
 *
 * So unhandled_reaction can only fire for a value outside the enum,
 * which no caller produces, and the seam is unreached rather than
 * waiting on work. If a reaction is ever added without a handler, this
 * is where it shows up.
 */
static int unhandled_count;
static wm_arcade_reaction_id_t unhandled_ids[64];
static void cap_unhandled(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                          wm_arcade_reaction_id_t r, void *u) {
    (void)a; (void)v; (void)u;
    if (unhandled_count < (int)(sizeof unhandled_ids / sizeof unhandled_ids[0]))
        unhandled_ids[unhandled_count] = r;
    ++unhandled_count;
}

static void test_every_reaction_id_is_handled_by_the_chain(void) {
    int id;
    unhandled_count = 0;

    /* WM_RXN_ONTURNBUCKLE is the last member, so the enum is 0..51. */
    assert((int)WM_RXN_ONTURNBUCKLE == 51);

    for (id = 0; id <= (int)WM_RXN_ONTURNBUCKLE; ++id) {
        wm_arcade_actor_t a, v;
        wm_arcade_react1_callbacks_t cb;
        wm_arcade_react1_context_t ctx;
        int16_t pending = -5, newdir = 0;

        memset(&a, 0, sizeof a);
        memset(&v, 0, sizeof v);
        memset(&cb, 0, sizeof cb);
        memset(&ctx, 0, sizeof ctx);
        a.active = v.active = 1;
        a.life = v.life = 100;
        a.x_int = 100; v.x_int = 120;
        a.x_fixed = 100 << 16; v.x_fixed = 120 << 16;
        v.player_mode = WM_PMODE_NORMAL;
        cb.unhandled_reaction = cap_unhandled;
        ctx.callbacks = &cb;

        wm_arcade_react123456789_reaction_callback(
            &a, &v, (wm_arcade_reaction_id_t)id, &pending, &newdir, &ctx);
    }

    if (unhandled_count != 0)
        printf("unhandled reaction id %d (and %d more)\n",
               (int)unhandled_ids[0], unhandled_count - 1);
    assert(unhandled_count == 0);

    /* And a value past the end DOES reach it -- so the zero above is a
       real result and not a seam that never gets called. */
    {
        wm_arcade_actor_t a, v;
        wm_arcade_react1_callbacks_t cb;
        wm_arcade_react1_context_t ctx;
        int16_t pending = -5, newdir = 0;
        memset(&a, 0, sizeof a); memset(&v, 0, sizeof v);
        memset(&cb, 0, sizeof cb); memset(&ctx, 0, sizeof ctx);
        a.active = v.active = 1; a.life = v.life = 100;
        v.player_mode = WM_PMODE_NORMAL;
        cb.unhandled_reaction = cap_unhandled;
        ctx.callbacks = &cb;
        unhandled_count = 0;
        wm_arcade_react123456789_reaction_callback(
            &a, &v, (wm_arcade_reaction_id_t)(WM_RXN_ONTURNBUCKLE + 1),
            &pending, &newdir, &ctx);
        assert(unhandled_count == 1);
    }
}

/*
 * partner_breakout's two animations. `xxx_` means GENERIC in this
 * source -- REACT1.ASM:1873 and :1927 define them once for the whole
 * roster, unlike every hrt_/dnk_/und_ label -- and both have been in the
 * generated programs since they were extracted.
 */
static void test_the_breakout_animations_are_real_and_generic(void) {
    assert(wm_anim_program_find("xxx_goto_stand_anim") != NULL);
    assert(wm_anim_program_find("xxx_aborted_attach_anim") != NULL);
    /* Two different animations, not one under two names. */
    assert(wm_anim_program_find("xxx_goto_stand_anim") !=
           wm_anim_program_find("xxx_aborted_attach_anim"));
}

/*
 * The mode table that picks between them (REACT1.ASM:707-731 against
 * :746/:753). Standing-ish modes go to the stand animation; modes where
 * the partner is committed to something abort the attach instead.
 */
static int breaks;
static wm_arcade_partner_breakout_t last_kind;
static wm_arcade_actor_t *last_partner;
static void cap_break(wm_arcade_actor_t *p, wm_arcade_partner_breakout_t k,
                      void *u) {
    (void)u; ++breaks; last_kind = k; last_partner = p;
}

static void run_hit(wm_arcade_actor_t *victim, wm_arcade_actor_t *partner,
                    const void *attacker_identity, uint16_t partner_mode) {
    wm_arcade_react_callbacks_t cb;
    memset(&cb, 0, sizeof cb);
    cb.partner_breakout = cap_break;
    partner->player_mode = partner_mode;
    victim->attach_proc = partner;
    partner->attach_proc = victim;
    breaks = 0; last_kind = (wm_arcade_partner_breakout_t)0;
    last_partner = NULL;
    wm_arcade_hit_stuff_identity(attacker_identity, 0, victim, &cb);
}

static void test_the_mode_table_picks_the_right_breakout(void) {
    wm_arcade_actor_t victim, partner, attacker;
    memset(&victim, 0, sizeof victim);
    memset(&partner, 0, sizeof partner);
    memset(&attacker, 0, sizeof attacker);

    run_hit(&victim, &partner, &attacker, WM_PMODE_HEADHOLD);
    assert(breaks == 1 && last_kind == WM_PARTNER_GOTO_STAND);
    assert(last_partner == &partner);

    run_hit(&victim, &partner, &attacker, WM_PMODE_ONGROUND);
    assert(breaks == 1 && last_kind == WM_PARTNER_ABORT_ATTACH_ANIM);

    run_hit(&victim, &partner, &attacker, WM_PMODE_INAIR);
    assert(breaks == 1 && last_kind == WM_PARTNER_ABORT_ATTACH_ANIM);

    /* Both wrestlers' attachments are cleared either way. */
    assert(victim.attach_proc == NULL);
    assert(partner.attach_proc == NULL);
}

/*
 * And the note's one correct claim: this needs a THIRD party. The
 * caller only fires when the partner is not the attacker, which two
 * wrestlers cannot satisfy.
 */
static void test_two_wrestlers_cannot_reach_it(void) {
    wm_arcade_actor_t victim, attacker;
    memset(&victim, 0, sizeof victim);
    memset(&attacker, 0, sizeof attacker);

    /* The attacker IS the partner: no breakout. */
    run_hit(&victim, &attacker, &attacker, WM_PMODE_HEADHOLD);
    assert(breaks == 0);
}

int main(void) {
    test_every_reaction_id_is_handled_by_the_chain();
    test_the_breakout_animations_are_real_and_generic();
    test_the_mode_table_picks_the_right_breakout();
    test_two_wrestlers_cannot_reach_it();
    printf("reaction chain and breakout tests passed\n");
    return 0;
}
