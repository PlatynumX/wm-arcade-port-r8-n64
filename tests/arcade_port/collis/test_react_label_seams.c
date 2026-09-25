/*
 * The three seams closed out of port/seam_ledger.json's deferred list,
 * all of which were declared, NULL-checked at their call sites and filled
 * by nobody -- so each compiled, ran, and did nothing.
 *
 *   attacker_uses_lex_flykick_anim  REACT1.ASM:1453
 *   attacker_anim_tag               REACT4.ASM:137
 *   jump_rope_audio                 DCSSOUND.ASM:2914, and it turned out
 *                                   to be TWO seams
 *
 * The first two compare the ATTACKER's ANIBASE against named animation
 * labels, so what they need is the label the attacker is currently
 * playing. These tests drive the REACT cores with recorders standing in
 * for the backends, which is how a label comparison can be checked
 * without a whole match around it.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_react1_core.h"
#include "wm/arcade/wm_arcade_react.h"
#include "wm/arcade/wm_arcade_react4_core.h"
#include "wm/announce_tables.h"
#include "wm/arcade/wm_arcade_announcer.h"

/* ---- the two label comparisons --------------------------------- */

static const char *playing;          /* what the "backend" says is playing */
static int drop_kicks, mid_hits;
static wm_arcade_react1_sound_t last_sound;
static int sounds;

static int cap_lex_flykick(const wm_arcade_actor_t *a, void *u) {
    (void)a; (void)u;
    if (!playing) return 0;
    return strcmp(playing, "lex_flying_kick_anim") == 0 ||
           strcmp(playing, "lex_super_kick_anim") == 0;
}
static void cap_impact(wm_arcade_actor_t *a, wm_arcade_actor_t *v,
                       wm_arcade_react1_impact_t im, void *u) {
    (void)a; (void)v; (void)u;
    if (im == WM_R1_IMPACT_DROP_KICK) ++drop_kicks;
    if (im == WM_R1_IMPACT_MID) ++mid_hits;
}
static void cap_sound(wm_arcade_actor_t *v, wm_arcade_react1_sound_t s,
                      void *u) {
    (void)v; (void)u; last_sound = s; ++sounds;
}
static void cap_anim(wm_arcade_actor_t *v, wm_arcade_react1_anim_group_t g,
                     void *u) { (void)v; (void)g; (void)u; }

static wm_arcade_react_anim_tag_t cap_tag(const wm_arcade_actor_t *a,
                                          void *u) {
    static const struct { const char *l; wm_arcade_react_anim_tag_t t; } rows[] = {
        { "shn_combo_run_stomp_anim",     WM_R_ANIMTAG_SHN_COMBO_RUN_STOMP },
        { "shn_run_stomp_anim",           WM_R_ANIMTAG_SHN_RUN_STOMP },
        { "dnk_belly_anim",               WM_R_ANIMTAG_DNK_BELLY },
        { "und_flying_butt_drop_anim",    WM_R_ANIMTAG_UND_FLYING_BUTT_DROP },
        { "lex_flying_ground_punch_anim", WM_R_ANIMTAG_LEX_FLYING_GROUND_PUNCH }
    };
    size_t i;
    (void)a; (void)u;
    if (!playing) return WM_R_ANIMTAG_OTHER;
    for (i = 0; i < sizeof rows / sizeof rows[0]; ++i)
        if (strcmp(playing, rows[i].l) == 0) return rows[i].t;
    return WM_R_ANIMTAG_OTHER;
}

static int ropes_shaken, screen_shakes, screams;
static void cap_ropes(void *u) { (void)u; ++ropes_shaken; }
static void cap_shaker(int n, void *u) { (void)n; (void)u; ++screen_shakes; }
static void cap_triple(wm_arcade_actor_t *v, uint16_t id, void *u) {
    (void)v; (void)u; if (id == 0x43) ++screams;
}
static int32_t cap_health(const wm_arcade_actor_t *v, void *u) {
    (void)u; return v ? v->life : 0;
}

static void ctx_reset(wm_arcade_react1_context_t *ctx,
                      wm_arcade_react1_callbacks_t *cb) {
    memset(cb, 0, sizeof *cb);
    cb->change_anim = cap_anim;
    cb->play_sound = cap_sound;
    cb->impact = cap_impact;
    cb->triple_sound = cap_triple;
    cb->get_health = cap_health;
    cb->attacker_uses_lex_flykick_anim = cap_lex_flykick;
    cb->attacker_anim_tag = cap_tag;
    cb->shake_all_ropes = cap_ropes;
    cb->shaker2 = cap_shaker;
    memset(ctx, 0, sizeof *ctx);
    ctx->callbacks = cb;
    drop_kicks = mid_hits = sounds = ropes_shaken = screen_shakes = 0;
    screams = 0;
    last_sound = (wm_arcade_react1_sound_t)0;
}

static void pair(wm_arcade_actor_t *a, wm_arcade_actor_t *v) {
    memset(a, 0, sizeof *a);
    memset(v, 0, sizeof *v);
    a->active = v->active = 1;
    a->life = v->life = 100;
    a->x_int = 100; v->x_int = 120;
    a->x_fixed = 100 << 16; v->x_fixed = 120 << 16;
    v->player_mode = WM_PMODE_NORMAL;
}

/*
 * REACT1.ASM:1453, the source's own comment: "HACK! - Lex's flying kicks
 * don't knock you down. Use CALL_MID_HIT." Two labels and only Lex's.
 * With the seam empty every flying kick took CALL_DROP_KICK, so a rule
 * about one wrestler applied to nobody.
 */
static void test_only_lex_flying_kicks_take_the_mid_hit(void) {
    wm_arcade_actor_t a, v;
    wm_arcade_react1_callbacks_t cb;
    wm_arcade_react1_context_t ctx;

    playing = "lex_flying_kick_anim";
    ctx_reset(&ctx, &cb); pair(&a, &v);
    assert(wm_arcade_react1_apply(&a, &v, WM_RXN_FLYKICK,
                                  NULL, NULL, &ctx) != 0);
    assert(mid_hits == 1 && drop_kicks == 0);

    playing = "lex_super_kick_anim";
    ctx_reset(&ctx, &cb); pair(&a, &v);
    assert(wm_arcade_react1_apply(&a, &v, WM_RXN_FLYKICK,
                                  NULL, NULL, &ctx) != 0);
    assert(mid_hits == 1 && drop_kicks == 0);

    /* Anybody else's flying kick, and Lex's OTHER animations, knock down. */
    playing = "hrt_flying_kick_anim";
    ctx_reset(&ctx, &cb); pair(&a, &v);
    assert(wm_arcade_react1_apply(&a, &v, WM_RXN_FLYKICK,
                                  NULL, NULL, &ctx) != 0);
    assert(drop_kicks == 1 && mid_hits == 0);

    playing = "lex_flying_ground_punch_anim";
    ctx_reset(&ctx, &cb); pair(&a, &v);
    assert(wm_arcade_react1_apply(&a, &v, WM_RXN_FLYKICK,
                                  NULL, NULL, &ctx) != 0);
    assert(drop_kicks == 1 && mid_hits == 0);

    /* No backend answer at all is "not that one", not a crash. */
    playing = NULL;
    ctx_reset(&ctx, &cb); pair(&a, &v);
    assert(wm_arcade_react1_apply(&a, &v, WM_RXN_FLYKICK,
                                  NULL, NULL, &ctx) != 0);
    assert(drop_kicks == 1 && mid_hits == 0);
}

/*
 * REACT4.ASM:137 hit_stomp. Shawn's two run stomps take DO_SCREAM
 * instead of the LBOWDROP pair and `triple_sound 43h`; three other
 * labels take a bounce that throws the ATTACKER back up off the downed
 * man's chest, shaking the ropes and the screen with him.
 */
static void test_hit_stomp_reads_the_attacker_label(void) {
    wm_arcade_actor_t a, v;
    wm_arcade_react1_callbacks_t cb;
    wm_arcade_react1_context_t ctx;
    int16_t pending;
    unsigned i;
    static const char *bouncers[3] = { "dnk_belly_anim",
                                       "und_flying_butt_drop_anim",
                                       "lex_flying_ground_punch_anim" };

    /* An ordinary stomp: the normal sounds, no bounce. */
    playing = "hrt_stomp_anim";
    ctx_reset(&ctx, &cb); pair(&a, &v);
    v.player_mode = WM_PMODE_ONGROUND;
    pending = 5;
    assert(wm_arcade_react4_apply(&a, &v, WM_RXN_STOMP,
                                  &pending, NULL, &ctx) != 0);
    assert(screams == 1);               /* triple_sound 43h */
    assert(ropes_shaken == 0 && screen_shakes == 0);
    assert(a.y_vel == 0);

    /* Shawn's run stomps: DO_SCREAM instead, so NO triple_sound 43h. */
    playing = "shn_run_stomp_anim";
    ctx_reset(&ctx, &cb); pair(&a, &v);
    v.player_mode = WM_PMODE_ONGROUND;
    pending = 5;
    assert(wm_arcade_react4_apply(&a, &v, WM_RXN_STOMP,
                                  &pending, NULL, &ctx) != 0);
    assert(screams == 0);
    assert(ropes_shaken == 0);

    playing = "shn_combo_run_stomp_anim";
    ctx_reset(&ctx, &cb); pair(&a, &v);
    v.player_mode = WM_PMODE_ONGROUND;
    pending = 5;
    assert(wm_arcade_react4_apply(&a, &v, WM_RXN_STOMP,
                                  &pending, NULL, &ctx) != 0);
    assert(screams == 0);

    /* The three bouncers all throw the attacker up and shake both. */
    for (i = 0; i < 3; ++i) {
        playing = bouncers[i];
        ctx_reset(&ctx, &cb); pair(&a, &v);
        v.player_mode = WM_PMODE_ONGROUND;
        pending = 5;
        assert(wm_arcade_react4_apply(&a, &v, WM_RXN_STOMP,
                                  &pending, NULL, &ctx) != 0);
        assert(ropes_shaken == 1);
        assert(screen_shakes == 1);
        assert(a.y_vel != 0);
        assert(a.x_vel == 0);
    }

    /* Lex's is #bounce3: the bounce, then a DIFFERENT Y velocity. */
    {
        int32_t plain_y, lex_y;
        playing = "dnk_belly_anim";
        ctx_reset(&ctx, &cb); pair(&a, &v);
        v.player_mode = WM_PMODE_ONGROUND; pending = 5;
        assert(wm_arcade_react4_apply(&a, &v, WM_RXN_STOMP,
                                  &pending, NULL, &ctx) != 0);
        plain_y = a.y_vel;

        playing = "lex_flying_ground_punch_anim";
        ctx_reset(&ctx, &cb); pair(&a, &v);
        v.player_mode = WM_PMODE_ONGROUND; pending = 5;
        assert(wm_arcade_react4_apply(&a, &v, WM_RXN_STOMP,
                                  &pending, NULL, &ctx) != 0);
        lex_y = a.y_vel;
        assert(plain_y != lex_y);
    }
}

/* ---- the turnbuckle announcer ---------------------------------- */

/*
 * The seam that was recorded as "a WRSND whose move mnemonic has not been
 * read off BRET.ASM yet". It is not a WRSND: DCSSOUND.ASM:2914
 * ADD_IF_SILENT takes a SPEECH TABLE and queues a random phrase from it.
 *
 * And it was two seams wearing one name. CLIMB_ROPES is queued from
 * mode_normal's climb branch and JUMP_ROPES from mode_turn's dive, nine
 * call sites each, and the tables differ in every way a table can:
 * row count, row width, and the crowd reaction each carries.
 */
static void test_the_two_turnbuckle_tables_are_different(void) {
    const wm_announce_table *climb = wm_announce_table_find("CLIMB_ROPES");
    const wm_announce_table *jump = wm_announce_table_find("JUMP_ROPES");

    /* Both are emitted now; wlvoice.py used to leave them out. */
    assert(climb != NULL);
    assert(jump != NULL);

    /* DCSSOUND.ASM:3243 `.WORD 12,010H` versus :3266 `.WORD 6,020H`:
       RNDRNG0's inclusive maximum and the words per row. */
    assert(climb->last_index == 12);
    assert(jump->last_index == 6);
    assert(climb->stride == 1);
    assert(jump->stride == 2);

    /* The crowd `.LONG` above each header: CRESCENDO_TABLE versus
       ROPES_CHEER. This is what would be lost by pointing one seam at
       both tables. */
    assert(climb->crowd && strcmp(climb->crowd, "CRESCENDO_TABLE") == 0);
    assert(jump->crowd && strcmp(jump->crowd, "ROPES_CHEER") == 0);
    assert(strcmp(climb->crowd, jump->crowd) != 0);

    /* :3241 `.WORD -1` versus :3265 `.WORD 0` -- the repeat-reset flag. */
    assert(climb->reset_repeat);
    assert(!jump->reset_repeat);
}

/*
 * And the tables really speak: ADD_IF_SILENT at 1000 queues a line, and
 * only when the announcer is quiet.
 */
static void test_the_turnbuckle_tables_queue_a_line(void) {
    const wm_announce_table *tabs[2];
    wm_announcer_state an;
    wm_announce_ctx ctx;
    WmRng rng;
    unsigned i;

    tabs[0] = wm_announce_table_find("CLIMB_ROPES");
    tabs[1] = wm_announce_table_find("JUMP_ROPES");
    for (i = 0; i < 2; ++i) {
        assert(tabs[i] != NULL);
        wm_announcer_init(&an);
        wm_rng_init(&rng, 0x1234u, NULL, NULL, NULL);
        memset(&ctx, 0, sizeof ctx);
        ctx.rng = &rng;
        ctx.wrestler_num = 0;
        assert(wm_announce_from_table(&an, tabs[i], 1000, true, &ctx) >= 1);
        /* ADD_IF_SILENT, not ADD_TO_QUEUE: a second call while he is
           still talking declines. */
        assert(wm_announce_from_table(&an, tabs[i], 1000, true, &ctx) == 0);
    }
}

int main(void) {
    test_only_lex_flying_kicks_take_the_mid_hit();
    test_hit_stomp_reads_the_attacker_label();
    test_the_two_turnbuckle_tables_are_different();
    test_the_turnbuckle_tables_queue_a_line();
    printf("react label seam tests passed\n");
    return 0;
}
