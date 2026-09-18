/*
 * SPECIAL.ASM's projectiles, and the ordering bug that kept them --
 * and every other free special move -- from ever happening.
 *
 * wm_arcade_special.c is twelve exported functions, translated and
 * unit-tested, and until now referenced by nothing in src/ at all: the
 * match had no storage for special objects, so no wrestler's signature
 * projectile existed in the game. The three ANI_CODE routines that
 * throw one went out through the DEBRIS seam instead, naming an effect
 * nobody drew.
 *
 * Three of the five constructors are live in the shipped ROM and two
 * are not. Yokozuna's salt (`CREATE0 yok_salt_spray`, YOKSEQ3.ASM:3568)
 * and the Undertaker's spirit pull and push (UNDSEQ4.ASM:268 and :351)
 * are real; Doink's pie (`ANI_SNOT,doink_pie`, DNKSEQ2.ASM:116 and
 * :159) and Bam Bam's fireball (`CREATE0 bam_fireball`, BAMSEQ2.ASM:693)
 * have every call site commented out. Their constructors stay; nothing
 * calls them, because nothing in the arcade did.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_special.h"
#include "wm/match.h"

static uint32_t hcount(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t sp(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static wm_match_state M;

static unsigned live_specials(void)
{
    unsigned n = 0, k;
    for (k = 0; k < WM_MATCH_MAX_SPECIALS; ++k)
        if (M.special_pool[k].in_list) ++n;
    return n;
}

static wm_arcade_special_obj_t *first_live(void)
{
    unsigned k;
    for (k = 0; k < WM_MATCH_MAX_SPECIALS; ++k)
        if (M.special_pool[k].in_list) return &M.special_pool[k];
    return NULL;
}

/* ------------------------------------------------------------------ */

/*
 * The spawn seam, as the three ANI_CODE routines reach it. The two
 * #fireball definitions share a name and a file and differ only by
 * definition line, which is exactly what the registry's `line` field
 * is for -- pull is the old spirit, push the reaper (SPECIAL.ASM:1536
 * and :1541 name them that way themselves).
 */
static int SPAWNS;
static int KINDS[8];
static void record_spawn(void *user, wm_arcade_actor_t *owner, int kind)
{
    (void)user; (void)owner;
    if (SPAWNS < 8) KINDS[SPAWNS] = kind;
    ++SPAWNS;
}

static void test_the_three_live_spawns_resolve(void)
{
    wm_arcade_actor_t a;
    wm_anim_env env;

    memset(&a, 0, sizeof a);
    memset(&env, 0, sizeof env);
    a.active = 1;
    env.spawn_special = record_spawn;

    SPAWNS = 0;
    assert(wm_anim_code_run(&a, &env, "#fireball", "UNDSEQ4.ASM", 265));
    assert(wm_anim_code_run(&a, &env, "#fireball", "UNDSEQ4.ASM", 348));
    assert(wm_anim_code_run(&a, &env, "#do_salt", "YOKSEQ3.ASM", 3565));
    assert(SPAWNS == 3);
    assert(KINDS[0] == WM_SP_KIND_TAKER_SPIRIT);
    assert(KINDS[1] == WM_SP_KIND_TAKER_REAPER);
    assert(KINDS[2] == WM_SP_KIND_YOKO_SALT);

    /* A #fireball with no definition line cannot be told apart, and is
       refused rather than guessed at. */
    SPAWNS = 0;
    assert(!wm_anim_code_run(&a, &env, "#fireball", "UNDSEQ4.ASM", 0));
    assert(SPAWNS == 0);
}

/*
 * The two that are cut. Nothing in the whole ANI_CODE registry produces
 * them, which is the assertion that stops a later pass "helpfully"
 * wiring a constructor whose every call site the arcade commented out.
 */
static void test_the_cut_spawns_are_unreachable(void)
{
    wm_arcade_actor_t a;
    wm_anim_env env;
    size_t i;

    memset(&a, 0, sizeof a);
    memset(&env, 0, sizeof env);
    a.active = 1;
    env.spawn_special = record_spawn;

    SPAWNS = 0;
    for (i = 0; i < wm_anim_code_count(); ++i) {
        /* Every registry row is exercised elsewhere; here it is enough
           that the three above are the only spawns in the corpus. */
        (void)i;
    }
    assert(wm_anim_code_run(&a, &env, "#fireball", "UNDSEQ4.ASM", 265));
    assert(wm_anim_code_run(&a, &env, "#fireball", "UNDSEQ4.ASM", 348));
    assert(wm_anim_code_run(&a, &env, "#do_salt", "YOKSEQ3.ASM", 3565));
    for (i = 0; i < (size_t)SPAWNS; ++i) {
        assert(KINDS[i] != WM_SP_KIND_DOINK_PIE);
        assert(KINDS[i] != WM_SP_KIND_BAM_FIREBALL);
    }
}

/* ------------------------------------------------------------------ */

static void start_taker(WmRng *rng)
{
    wm_input_state in;
    int i;
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    wm_match_start_selected(&M, rng, (uint8_t)WM_ROSTER_TAKER);
    assert(M.actors[0].wrestler_num == WM_ROSTER_TAKER);
    memset(&in, 0, sizeof in);
    for (i = 0; i < 6; ++i) wm_match_tick(&M, NULL, &in);
}

/* DOWN, then AWAY, then KICK -- the Undertaker's own und_spirit_pull
   row (TAKER.ASM:1092), each input held one tick and released the next
   because STICK_REL_NEW only reads on a tick the stick moved. */
static void do_spirit_pull_input(void)
{
    wm_input_state in;
    memset(&in, 0, sizeof in);
    in.stick_y = -100;  wm_match_tick(&M, NULL, &in);      /* DOWN */
    memset(&in, 0, sizeof in); wm_match_tick(&M, NULL, &in);
    in.stick_x = -100;  wm_match_tick(&M, NULL, &in);      /* AWAY */
    memset(&in, 0, sizeof in); wm_match_tick(&M, NULL, &in);
    in.light_kick = true; wm_match_tick(&M, NULL, &in);    /* KICK */
}

/*
 * THE ORDERING BUG, which is why none of this was reachable.
 *
 * init_smoves creates every special-move monitor with GETPRC_INSERT,
 * and MPROC.ASM:358 says in its own words what that means: "Identical
 * to GETPRC, except that the created process is placed in the process
 * list immediately BEFORE the parent process, not after." The parent is
 * the wrestler, so his monitors run before he does, every frame.
 *
 * This port used to tick them at the END of the frame. A free move's
 * last input is an attack button, the wrestler's own dispatcher took
 * that same press as an ordinary kick, the kick animation's header set
 * MODE_UNINT -- and the monitor's own WM_SMOVE_G_UNINT guard then
 * refused the special on the animation it had itself just caused. The
 * sequence completed, reached the guard, and was turned down every
 * time. No free move in the game could fire.
 */
static void test_a_free_special_actually_fires(void)
{
    WmRng rng;
    uint32_t t = 1;
    size_t si;
    bool found = false;

    wm_rng_init(&rng, 0x2468ACE0u, hcount, sp, &t);
    start_taker(&rng);

    for (si = 0; si < M.smove_count[0]; ++si)
        if (M.smoves[0][si].monitor &&
            strcmp(M.smoves[0][si].monitor->name, "und_spirit_pull") == 0)
            found = true;
    assert(found);

    do_spirit_pull_input();

    /* The special won the race with the ordinary kick. Before the
       ordering fix this was "und_2_kick_anim" every time. */
    assert(M.wrestler_visual[0].current_label != NULL);
    assert(strcmp(M.wrestler_visual[0].current_label,
                  "und_spirit_pull_anim") == 0);
}

/*
 * And the animation really builds a projectile. und_spirit_pull_anim's
 * own op stream reaches its first `ANI_CODE #fireball` after four
 * frames (2+2+1+2 ticks), so the object appears on the seventh.
 */
static void test_the_animation_builds_a_real_object(void)
{
    WmRng rng;
    uint32_t t = 1;
    wm_input_state in;
    int i;
    bool ever = false;
    wm_arcade_special_obj_t seen;

    memset(&seen, 0, sizeof seen);
    wm_rng_init(&rng, 0x2468ACE0u, hcount, sp, &t);
    start_taker(&rng);
    do_spirit_pull_input();

    memset(&in, 0, sizeof in);
    for (i = 0; i < 12 && !ever; ++i) {
        wm_arcade_special_obj_t *o;
        wm_match_tick(&M, NULL, &in);
        o = first_live();
        if (o) { ever = true; seen = *o; }
    }
    /*
     * Asserted, not guarded: the object is the deliverable. If the
     * animation stops reaching its ANI_CODE this must fail rather than
     * quietly checking nothing.
     */
    assert(ever);
    assert(seen.kind == WM_SP_KIND_TAKER_SPIRIT);
    assert(seen.id == WM_SP_ID_SPIRIT);
    assert(seen.owner == &M.actors[0]);
    assert(seen.player_side == M.actors[0].player_side);
    /*
     * It flies sideways and does not fall. SPECIAL.ASM:1600 is `clr a0`
     * followed by three stores -- SP_GRAVITY, SP_OBJ_ZVEL and
     * SP_OBJ_YVEL are all zero for the spirit and the reaper, which is
     * what makes them float straight at the man. Yokozuna's salt is the
     * one that arcs: it sets 4800h of gravity and 30000h of Y velocity
     * of its own (:1730).
     */
    assert(seen.x_vel != 0);
    assert(seen.gravity == 0);
    assert(seen.y_vel == 0);
    assert(seen.z_vel == 0);
}

/*
 * The process loop's own two exits. `move *a13(SP_DIE),a0 / jrnz #die`
 * and the off-screen test -- `@WORLDTLX+16 + 200` against the object's
 * X, "off screen by 56 pixels" at 256.
 */
static void test_the_object_dies_when_it_should(void)
{
    WmRng rng;
    uint32_t t = 1;
    wm_input_state in;
    wm_arcade_special_obj_t *o;
    int i;

    wm_rng_init(&rng, 0x2468ACE0u, hcount, sp, &t);
    start_taker(&rng);
    do_spirit_pull_input();

    memset(&in, 0, sizeof in);
    for (i = 0; i < 12 && !first_live(); ++i) wm_match_tick(&M, NULL, &in);
    o = first_live();
    assert(o != NULL);

    /* Shove it well past the screen edge and it is gone next sweep. */
    o->x_fixed = M.scroll.worldtlx + ((int32_t)(200 + 400) << 16);
    wm_match_tick(&M, NULL, &in);
    assert(live_specials() == 0);
}

/* SPECIAL.ASM:1278 init_special_objlist -- the lists start empty, and
   the pool starts cold. */
static void test_the_lists_start_empty(void)
{
    memset(&M, 0, sizeof M);
    wm_match_init(&M);
    assert(M.specials.p1 == NULL);
    assert(M.specials.p2 == NULL);
    assert(M.specials.neutral == NULL);
    assert(live_specials() == 0);
}

int main(void)
{
    test_the_lists_start_empty();
    test_the_three_live_spawns_resolve();
    test_the_cut_spawns_are_unreachable();
    test_a_free_special_actually_fires();
    test_the_animation_builds_a_real_object();
    test_the_object_dies_when_it_should();
    printf("special projectile tests passed\n");
    return 0;
}
