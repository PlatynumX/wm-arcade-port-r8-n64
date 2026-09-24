/*
 * WRESTLE.ASM:4851 check_secret_moves, for the seven wrestlers who are
 * not Bret.
 *
 * THE POINT OF THIS FILE IS THAT THERE ARE TWO MECHANISMS, and only one
 * of them was live. The source drives secret moves two different ways:
 *
 *   - check_secret_moves is the FIRST thing every move_xxx does --
 *     `movi xxx_secret_moves,a11 / calla check_secret_moves`, before the
 *     mode table is even indexed. It scans that wrestler's pattern table
 *     against wrest_joystat, the per-player ring buffer of
 *     (round_tickcount, joy+buttons) entries, and jumps to the matched
 *     entry. Button sequences: grab-fling, hip toss, ear slap.
 *
 *   - init_smoves (WRESTLE2.ASM) inserts one SMOVE_PID process per entry
 *     of xxx_smove_table at match start, and match_tick_smoves_for
 *     drives those. Different table, different driver, different moves.
 *
 * Only Bret's half of the first was wired. The seam was declared in
 * wm_arcade_roster.h and wm_arcade_razor.h, called at the top of all
 * seven other dispatchers, and filled by nobody -- so seven wrestlers
 * had no button-sequence secret moves at all, while their smove
 * monitors worked and made the mechanism look covered.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"
#include "wm/arcade/wm_arcade_joystat.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wm_arcade_wrestler_port.h"
#include "wm/arcade/wmania_rng.h"
#include "wm/wrestler_sound_labels.h"

static int sound_calls;
static void cap_sound(void *u, uint16_t call) { (void)u; (void)call; ++sound_calls; }

static uint32_t hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }

static WmRng rng;
static uint32_t rng_t = 1;

static void setup(wm_wrestler_backend_actor *st, wm_arcade_actor_t *me,
                  wm_arcade_actor_t *him, int wrestler) {
    memset(st, 0, sizeof *st);
    memset(me, 0, sizeof *me);
    memset(him, 0, sizeof *him);
    me->active = him->active = 1;
    me->wrestler_num = wrestler;
    me->player_mode = WM_PMODE_NORMAL;
    me->facing_dir = me->new_facing_dir = WM_MOVE_RIGHT;
    him->player_side = 1;
    him->player_mode = WM_PMODE_NORMAL;
    st->opponent = him;
    st->wrestler_num = wrestler;
    st->pcnt = 100;
    wm_rng_init(&rng, 0x2468ACE0u, hc, spf, &rng_t);
    st->anim_env.rng = &rng;
    st->anim_env.sound = cap_sound;
}

/* One tick of "away on the stick and punch", which is Doink's
   hip_toss2 -- his only single-step pattern, and so the one that needs
   no history to trigger. */
static void press_away_punch(wm_arcade_actor_t *me) {
    me->stick_val_cur = WM_MOVE_LEFT;
    me->stick_val_down = WM_MOVE_LEFT;
    me->but_val_down = WM_BTN_PUNCH;
}

static void test_the_seam_is_filled(void) {
    wm_wrestler_backend_actor st;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_bret_backend_actor bva;
    wm_arcade_bret_callbacks_t bret_cb;

    memset(&st, 0, sizeof st);
    memset(&bva, 0, sizeof bva);
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    bret_cb = wm_bret_backend_callbacks(&bva);

    /* Bret's was always filled; the other two never were. */
    assert(bret_cb.check_secret_moves != NULL);
    assert(roster_cb.check_secret_moves != NULL);
    assert(razor_cb.check_secret_moves != NULL);
}

static void test_a_pattern_reaches_its_handler(void) {
    wm_wrestler_backend_actor st;
    wm_arcade_roster_callbacks_t cb;
    wm_arcade_actor_t me, him;
    const wm_arcade_wrestler_profile_t *p;

    setup(&st, &me, &him, WM_ROSTER_DOINK);
    cb = wm_wrestler_roster_callbacks(&st);
    p = wm_arcade_roster_profile(WM_ROSTER_DOINK);
    assert(p && p->secret_count > 0);

    press_away_punch(&me);
    sound_calls = 0;
    cb.check_secret_moves(&me, p->secrets, p->secret_count, cb.user);

    /*
     * The handler ran. Its own `snd(a,"GRABFLING",c)` is the observable
     * end of the chain -- pattern match, wm_arcade_port_fire_secret,
     * the per-wrestler handler, and out through the sound seam that was
     * itself wired one commit ago. An RNG is supplied because almost
     * every per-wrestler MASTER_SOUND_TABLE entry has table_sound's
     * random bit set, and without one a draw resolves to silence.
     */
    assert(sound_calls > 0);
}

static void test_the_four_gates(void) {
    wm_wrestler_backend_actor st;
    wm_arcade_roster_callbacks_t cb;
    wm_arcade_actor_t me, him;
    const wm_arcade_wrestler_profile_t *p = wm_arcade_roster_profile(WM_ROSTER_DOINK);
    int i;

    /* WRESTLE.ASM:4853-4862: IMMOBILIZE_TIME, MODE_DIZZY, MODE_WAITANIM
       and GETUP_TIME each refuse before a single pattern is looked at --
       "No secret moves if getup time is set - out of control runs". */
    for (i = 0; i < 4; ++i) {
        setup(&st, &me, &him, WM_ROSTER_DOINK);
        cb = wm_wrestler_roster_callbacks(&st);
        press_away_punch(&me);
        switch (i) {
        case 0: me.immobilize_time = 1; break;
        case 1: me.player_mode = (uint16_t)WM_PMODE_DIZZY; break;
        case 2: me.player_mode = (uint16_t)WM_PMODE_WAITANIM; break;
        case 3: me.getup_time = 30; break;
        }
        sound_calls = 0;
        cb.check_secret_moves(&me, p->secrets, p->secret_count, cb.user);
        assert(sound_calls == 0);
    }
}

static void test_the_queue_must_be_fresh(void) {
    wm_wrestler_backend_actor st;
    wm_arcade_roster_callbacks_t cb;
    wm_arcade_actor_t me, him;
    const wm_arcade_wrestler_profile_t *p = wm_arcade_roster_profile(WM_ROSTER_DOINK);

    /* "only check if newest entry in queue is fresh": with no input this
       tick, nothing was recorded, so no pattern can be the one that just
       completed -- however good the history behind it. */
    setup(&st, &me, &him, WM_ROSTER_DOINK);
    cb = wm_wrestler_roster_callbacks(&st);
    press_away_punch(&me);
    sound_calls = 0;
    cb.check_secret_moves(&me, p->secrets, p->secret_count, cb.user);
    assert(sound_calls > 0);

    /* A later tick with nothing pressed: the same queue, now stale. */
    me.stick_val_down = 0;
    me.but_val_down = 0;
    st.pcnt = 140;
    sound_calls = 0;
    cb.check_secret_moves(&me, p->secrets, p->secret_count, cb.user);
    assert(sound_calls == 0);
}

static void test_every_wrestler_has_a_table(void) {
    static const int ids[] = { WM_ROSTER_DOINK, WM_ROSTER_LEX, WM_ROSTER_TAKER,
                               WM_ROSTER_YOKO, WM_ROSTER_SHAWN, WM_ROSTER_BAM };
    size_t i;

    /* All six shared dispatchers pass a real table to the seam, so
       filling it once serves all of them. Razor's is his own typed one
       and has its own adapter -- his pattern struct leads with an id
       where the shared one leads with a label, so the two are the same
       shape and a different layout. */
    for (i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        const wm_arcade_wrestler_profile_t *p =
            wm_arcade_roster_profile((wm_arcade_roster_id_t)ids[i]);
        assert(p != NULL);
        assert(p->secrets != NULL && p->secret_count > 0);
        /* And a charge button to hold, which is the table's own first
           entry -- executable code in the source rather than a
           value/mask row. */
        assert(p->charge_button != 0 && p->charge_ticks > 0);
    }
}

/* ------------------------------------------------------------------
 * The two seams the audit's own heuristic turned up.
 *
 * seam_audit.py keys on a seam's NAME, so one declared in several
 * callback structs and filled in only one of them reads as filled.
 * check_secret_moves was exactly that. The heuristic it grew afterwards
 * -- declared in more structs than it has assignment sites -- flagged
 * four, and two of them were real.
 * ------------------------------------------------------------------ */

static void test_the_partially_filled_pair(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t me;

    memset(&st, 0, sizeof st);
    memset(&bva, 0, sizeof bva);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    bret_cb = wm_bret_backend_callbacks(&bva);

    /*
     * change_torso_anim: declared for Bret and for Razor, filled only
     * for Bret. Razor's dispatcher selects WM_RZR_ANIM_TORSO2/TORSO4 at
     * two sites, so his SECOND animation channel never moved.
     */
    assert(razor_cb.change_torso_anim != NULL);

    /*
     * check_combo_go: declared in four structs and filled in three. The
     * missing one was Bret's, and his call site does not merely skip the
     * gate when it is empty -- `if (!cb->check_combo_go ||
     * cb->check_combo_go(a, cb->user) < 0) return 0;` REFUSES the move.
     * Every one of his head-hold combos was turned down before it
     * started.
     */
    assert(bret_cb.check_combo_go != NULL);

    /* And it answers rather than merely existing: an empty combo meter
       is below the threshold, which is a refusal and not an error. */
    memset(&me, 0, sizeof me);
    me.active = 1;
    (void)bret_cb.check_combo_go(&me, bret_cb.user);
}

/* ------------------------------------------------------------------
 * And what the secret handlers actually DO.
 *
 * Wiring check_secret_moves made these reachable, and reachable turned
 * out to expose a mistranslation rather than a missing piece: fifteen
 * handlers called startsp(), the port's "start a special-move process"
 * helper, where the source ends in a plain `calla change_anim1a`.
 * DOINK.ASM:583 #scrt_hiptoss is `FACE24 dnk,hiptoss_anim / calla
 * change_anim1a / WRSND W_DOINK,HIPTOSS_T1,PUNCH_T2` -- an animation
 * and a sound, and SPECIAL_MOVE_ADDR is never touched.
 * ------------------------------------------------------------------ */

static const char *fired_anim;
static const char *fired_snd;
static void cap_anim(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; fired_anim = l;
}
static void cap_label_snd(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; fired_snd = l;
}

static void test_the_hip_toss_plays_an_animation(void) {
    wm_wrestler_backend_actor st;
    wm_arcade_roster_callbacks_t cb;
    wm_arcade_actor_t me, him;
    const wm_arcade_wrestler_profile_t *p;

    setup(&st, &me, &him, WM_ROSTER_DOINK);
    cb = wm_wrestler_roster_callbacks(&st);
    p = wm_arcade_roster_profile(WM_ROSTER_DOINK);

    press_away_punch(&me);
    cb.check_secret_moves(&me, p->secrets, p->secret_count, cb.user);

    /*
     * Observed on the BACKEND rather than through a capture hung on the
     * callback struct, and that is not incidental: the adapter rebuilds
     * the callbacks from the backend state before it dispatches, so a
     * caller cannot inject one. backend_change_anim_label records the
     * label it selected, which is the same thing from the other side.
     */
    fired_anim = st.current_label;

    /*
     * Doink faces right, so FACE24's second column -- the source's own
     * MOVE_UP_BIT-clear form. Before this the handler called
     * startsp("hip_toss"), whose two seams are empty, and the move
     * fired and did nothing at all.
     */
    assert(fired_anim != NULL);
    assert(strstr(fired_anim, "hiptoss") != NULL);
    /*
     * And the sound is the source's MIXED pair, SOUND.H's HIPTOSS_T1
     * (40) with PUNCH_T2 (1) -- not the GRABFLING every one of these
     * sites used to name, and not a pair from one family.
     */
    {
        wm_sndlabel_t sl = wm_wrsnd_label("HIPTOSS_PUNCH");
        assert(sl.kind == WM_SNDLABEL_WRSND);
        assert(sl.move1 == 40 && sl.move2 == 1);
    }
    (void)fired_snd;
    (void)cap_anim;
    (void)cap_label_snd;
}

int main(void) {
    test_the_seam_is_filled();
    test_a_pattern_reaches_its_handler();
    test_the_four_gates();
    test_the_queue_must_be_fresh();
    test_every_wrestler_has_a_table();
    test_the_partially_filled_pair();
    test_the_hip_toss_plays_an_animation();
    printf("secret moves ok\n");
    return 0;
}
