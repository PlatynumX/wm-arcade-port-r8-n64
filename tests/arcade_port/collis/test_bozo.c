/*
 * DOINK.ASM:3316 bozo_check -- the last of the routines the ledger had
 * open, and the game's answer to a player who has stopped wrestling and
 * started mashing.
 *
 * The routine is thirty lines. What took the work was that six of the
 * eight ported dispatchers never called it: the seam existed in
 * wm/arcade/wm_arcade_roster.h, Razor's and Bret's modules called it,
 * and the other six had no bozo branch at all -- so the head-hold power
 * move and the head-held reversal were missing outright rather than
 * merely inert.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/arcade/wm_arcade_bozo.h"
#include "wm/wrestler_backend.h"
#include "wm/bret_backend.h"
#include "wm/arcade/wm_arcade_doink.h"
#include "wm/wrestler_sound_labels.h"
#include "wm/wrestler_sound_tables.h"
#include "wm_arcade_roster.h"

static int endless_kills;
static void kill_endless(wm_arcade_actor_t *a, void *user) {
    (void)a; (void)user;
    ++endless_kills;
}

static void pair(wm_arcade_actor_t *me, wm_arcade_actor_t *him) {
    memset(me, 0, sizeof *me);
    memset(him, 0, sizeof *him);
    me->active = him->active = 1;
    me->player_side = 0; him->player_side = 1;
    me->player_mode = WM_PMODE_HEADHELD;
    him->player_mode = WM_PMODE_HEADHOLD;
    me->who_hit_me = him;
}

/* ---- the count, and which counters are in it --------------------- */

static void test_which_buttons_count(void) {
    wm_arcade_actor_t me, him;
    wm_bozo_env_t env;

    memset(&env, 0, sizeof env);

    /* Nothing pressed, nothing doing. */
    pair(&me, &him);
    assert(!wm_arcade_bozo_check(&me, &env));

    /* One short of the threshold. `cmpi 18,a2 / jrlt #no_bozo` is a
       strict less-than, so seventeen is not enough and eighteen is. */
    pair(&me, &him);
    me.spunchb_count = 17;
    assert(!wm_arcade_bozo_check(&me, &env));
    me.spunchb_count = 18;
    assert(wm_arcade_bozo_check(&me, &env));

    /*
     * THE ORDINARY ATTACK COUNTERS ARE NOT IN THE SUM. PLYR.EQU:152-156
     * declares five in one "keep ordered" block, and this routine adds
     * only SPUNCHB_COUNT, SKICKB_COUNT and BLOCKB_COUNT -- "lots of
     * super buttons and blocks have been hit". Mashing punch and kick
     * will never trip it, however hard.
     */
    pair(&me, &him);
    me.punchb_count = 500;
    me.kickb_count = 500;
    assert(!wm_arcade_bozo_check(&me, &env));

    /* And the three that do count are summed, not compared. */
    pair(&me, &him);
    me.spunchb_count = 6;
    me.skickb_count = 6;
    me.blockb_count = 5;
    assert(!wm_arcade_bozo_check(&me, &env));
    me.blockb_count = 6;
    assert(wm_arcade_bozo_check(&me, &env));
}

/* ---- the two refusals are not the same shape --------------------- */

static void test_the_refusals(void) {
    wm_arcade_actor_t me, him;
    wm_bozo_env_t env;

    memset(&env, 0, sizeof env);

    /*
     * "Do reversal unless I have been immobilized!" A frozen wrestler
     * does not get to reverse out of it -- and the count is NOT cleared
     * on the way past, so the moment he thaws it still stands. That is
     * the difference between this refusal and the too-few one, and it
     * is why the order of the two tests matters.
     */
    pair(&me, &him);
    me.spunchb_count = 20;
    me.immobilize_time = 1;
    assert(!wm_arcade_bozo_check(&me, &env));
    assert(me.spunchb_count == 20);
    assert(!(me.status_flags & WM_STATUS_SMART_ATTACK));
    assert(him.immobilize_time == 0);

    me.immobilize_time = 0;
    assert(wm_arcade_bozo_check(&me, &env));

    assert(!wm_arcade_bozo_check(NULL, &env));
}

/* ---- what success does ------------------------------------------- */

static void test_what_it_does(void) {
    wm_arcade_actor_t me, him;
    wm_bozo_env_t env;

    memset(&env, 0, sizeof env);
    env.find_and_kill_endless = kill_endless;
    endless_kills = 0;

    pair(&me, &him);
    me.spunchb_count = 18;
    assert(wm_arcade_bozo_check(&me, &env));

    /*
     * SMRTTGT is a macro with TWO halves (JJXM.H:37) and both of them
     * land: the flag, and the target. "target WHOHITME -- don't hit
     * anyone else."
     */
    assert(me.status_flags & WM_STATUS_SMART_ATTACK);
    assert(me.smart_target == &him);

    /* The man who did it to him is frozen for 32 ticks. */
    assert(him.immobilize_time == WM_BOZO_IMMOBILIZE);

    /* And FIND_AND_KILL_ENDLESS, once. */
    assert(endless_kills == 1);

    /*
     * The one guard that is this port's rather than the source's. The
     * source reads WHOHITME and stores through it without asking
     * whether it is zero; that is survivable on the arcade and a null
     * dereference here. Everything that is his OWN state still happens.
     */
    pair(&me, &him);
    me.who_hit_me = NULL;
    me.spunchb_count = 18;
    endless_kills = 0;
    assert(wm_arcade_bozo_check(&me, &env));
    assert(me.status_flags & WM_STATUS_SMART_ATTACK);
    assert(me.smart_target == NULL);
    assert(endless_kills == 1);

    /* The env is optional; the decision does not depend on it. */
    pair(&me, &him);
    me.spunchb_count = 18;
    assert(wm_arcade_bozo_check(&me, NULL));
    assert(him.immobilize_time == WM_BOZO_IMMOBILIZE);
}

/* ---- the seams, in all three callback structs -------------------- */

static void test_the_seams_are_wired(void) {
    wm_wrestler_backend_actor st;
    wm_bret_backend_actor bva;
    wm_arcade_roster_callbacks_t roster_cb;
    wm_arcade_razor_callbacks_t razor_cb;
    wm_arcade_bret_callbacks_t bret_cb;
    wm_arcade_actor_t me, him;

    memset(&st, 0, sizeof st);
    memset(&bva, 0, sizeof bva);
    roster_cb = wm_wrestler_roster_callbacks(&st);
    razor_cb = wm_wrestler_razor_callbacks(&st);
    bret_cb = wm_bret_backend_callbacks(&bva);

    /* bozo_check had nothing behind it in any of the three, and
       find_and_kill_endless -- which bozo_check itself calls, and which
       fifteen other call sites NULL-check -- had nothing behind it
       either. */
    assert(roster_cb.bozo_check && razor_cb.bozo_check && bret_cb.bozo_check);
    assert(roster_cb.find_and_kill_endless);
    assert(razor_cb.find_and_kill_endless);
    assert(bret_cb.find_and_kill_endless);

    /* And the wired one answers the way the routine does. */
    pair(&me, &him);
    assert(!roster_cb.bozo_check(&me, roster_cb.user));
    me.spunchb_count = 18;
    assert(roster_cb.bozo_check(&me, roster_cb.user));
    assert(him.immobilize_time == WM_BOZO_IMMOBILIZE);
    assert(bret_cb.bozo_check(&me, bret_cb.user));
}

/* ---- and the branch six dispatchers did not have ----------------- */

static const char *last_anim;
static const char *last_sound;
static int reversals, reversal_messages;
static void cap_anim(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; last_anim = l;
}
static void cap_sound(wm_arcade_actor_t *a, const char *l, void *u) {
    (void)a; (void)u; last_sound = l;
}
static int always_bozo(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; return 1; }
static int never_bozo(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; return 0; }
static void cap_reversal(wm_arcade_actor_t *a, void *u) { (void)a; (void)u; ++reversals; }
static void cap_reversal_msg(wm_arcade_actor_t *a, void *u) {
    (void)a; (void)u; ++reversal_messages;
}

static void test_the_dispatcher_branch(void) {
    wm_arcade_actor_t me, him;
    wm_arcade_doink_callbacks_t cb;
    wm_arcade_doink_env_t env;

    memset(&cb, 0, sizeof cb);
    cb.change_anim_label = cap_anim;
    cb.sound_label = cap_sound;
    cb.do_reversal = cap_reversal;
    cb.do_reversal_message = cap_reversal_msg;
    memset(&env, 0, sizeof env);

    /*
     * mode_headhold, "Bozo power move". The animation alternates on
     * PCNT's low bit: `move @PCNT,a14 / btst 0,a14 / jrz #tag` keeps
     * the FIRST label when the bit is clear and falls into the second
     * `movi` when it is set.
     */
    cb.bozo_check = always_bozo;
    pair(&me, &him);
    me.player_mode = WM_PMODE_HEADHOLD;
    him.player_mode = WM_PMODE_HEADHELD;
    last_anim = last_sound = NULL;
    env.pcnt = 0;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(last_anim && strcmp(last_anim, "dnk_3_head_slam_anim") == 0);
    assert(last_sound && strcmp(last_sound, "FLYKICK") == 0);

    last_anim = NULL;
    env.pcnt = 1;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(last_anim && strcmp(last_anim, "dnk_3_pile_driver_anim") == 0);

    /* Refused, and the ordinary head-hold path runs instead. */
    cb.bozo_check = never_bozo;
    last_anim = NULL;
    me.but_val_down = 0;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(last_anim == NULL || strcmp(last_anim, "dnk_3_head_slam_anim") != 0);

    /*
     * mode_headheld, "Bozo reversal" -- the same move, with DO_REVERSAL
     * and DO_REVERSAL_MESS in front of it. Those two are the whole
     * difference between the two call sites.
     */
    cb.bozo_check = always_bozo;
    pair(&me, &him);
    reversals = reversal_messages = 0;
    last_anim = NULL;
    env.pcnt = 0;
    (void)wm_arcade_move_doink(&me, &him, &env, &cb);
    assert(reversals == 1 && reversal_messages == 1);
    assert(last_anim && strcmp(last_anim, "dnk_3_head_slam_anim") == 0);
}

/* ---- and the sound seam behind all of it ------------------------- */

static uint32_t snd_hc(void *u) { uint32_t *t = u; return (*t += 0x139u) & 0x1ff; }
static uint32_t snd_spf(void *u) { uint32_t *t = u; return 0x01000000u + ((*t * 7u) & 0x3fff); }
static uint16_t sound_calls[8];
static int sound_count;
static void cap_call(void *user, uint16_t call) {
    (void)user;
    if (sound_count < 8) sound_calls[sound_count] = call;
    ++sound_count;
}

static void test_the_sound_seam(void) {
    wm_wrestler_backend_actor st;
    wm_arcade_roster_callbacks_t cb;
    wm_arcade_bret_callbacks_t bcb;
    wm_bret_backend_actor bva;
    wm_arcade_actor_t me;
    wm_sndlabel_t s;

    /*
     * sound_label was declared in wm/arcade/wm_arcade_roster.h and
     * assigned by nobody, so every `snd(a,"...",c)` in the six shared
     * dispatchers -- fifty of them -- was dropped on the floor. The
     * tables behind it have been here since the sound work; the
     * mnemonic-to-move-index step and this assignment were what was
     * missing.
     */
    memset(&st, 0, sizeof st);
    cb = wm_wrestler_roster_callbacks(&st);
    assert(cb.sound_label != NULL);

    memset(&bva, 0, sizeof bva);
    bcb = wm_bret_backend_callbacks(&bva);
    assert(bcb.sound != NULL);

    /* The resolver reads SOUND.H's own indexes. */
    s = wm_wrsnd_label("GRABFLING");
    assert(s.kind == WM_SNDLABEL_WRSND && s.move1 == 32 && s.move2 == 33);
    /* A pair need not come from one family: RAZOR.ASM:383 grabs and
       then punches. */
    s = wm_wrsnd_label("GRABFLING_PUNCH");
    assert(s.kind == WM_SNDLABEL_WRSND && s.move1 == 32 && s.move2 == 1);
    /* And BLOCK_WOOSH is not a WRSND at all -- DCSSOUND.ASM:4265 is one
       fixed triple_sound for everybody. */
    s = wm_wrsnd_label("BLOCK_WOOSH");
    assert(s.kind == WM_SNDLABEL_FIXED && s.call == 0x16u);
    /* A name the table does not carry resolves to UNKNOWN rather than
       to silence, which is what lets the source-tools test refuse one. */
    assert(wm_wrsnd_label("SPIRIT").kind == WM_SNDLABEL_UNKNOWN);
    assert(wm_wrsnd_label(NULL).kind == WM_SNDLABEL_UNKNOWN);

    /* End to end: a real label through the real seam reaches the sound
       sink with what MASTER_SOUND_TABLE holds for that wrestler. */
    memset(&me, 0, sizeof me);
    me.active = 1;
    me.wrestler_num = WM_ROSTER_DOINK;
    st.anim_env.sound = cap_call;
    st.anim_env.sound_user = NULL;
    st.anim_env.rng = NULL;

    sound_count = 0;
    cb.sound_label(&me, "BLOCK_WOOSH", cb.user);
    assert(sound_count == 1 && sound_calls[0] == 0x16u);

    /*
     * A WRSND pair goes through wm_wrsndx, which plays what the tables
     * hold -- and with no RNG a random-indexed entry resolves to 0
     * rather than inventing a draw, which is the rest of the port's
     * rule. So this asserts the call was MADE, against the table's own
     * answer, rather than pinning a sound id this test invented.
     */
    {
        WmRng r;
        uint32_t t = 1;
        int direct, through_seam;

        wm_rng_init(&r, 0x2468ACE0u, snd_hc, snd_spf, &t);
        st.anim_env.rng = &r;

        /*
         * Against wm_wrsndx's own answer rather than a sound id this
         * test invented: the seam must play exactly what WRSND plays
         * for that wrestler and that move pair. Almost every
         * per-wrestler entry has table_sound's random bit set, so
         * without an RNG the draw resolves to 0 and nothing is played
         * -- which is the rest of the port's rule, and is why this arm
         * needs one.
         */
        sound_count = 0;
        cb.sound_label(&me, "GRABFLING", cb.user);
        through_seam = sound_count;

        wm_rng_init(&r, 0x2468ACE0u, snd_hc, snd_spf, &t);
        sound_count = 0;
        direct = wm_wrsndx(WM_ROSTER_DOINK, 32, 33, &r, NULL, cap_call);
        assert(through_seam == direct);
        assert(direct >= 1);
    }

    /* An unknown label plays nothing at all rather than something
       arbitrary. */
    sound_count = 0;
    cb.sound_label(&me, "NOT_A_SOUND", cb.user);
    assert(sound_count == 0);
}

int main(void) {
    test_which_buttons_count();
    test_the_refusals();
    test_what_it_does();
    test_the_seams_are_wired();
    test_the_dispatcher_branch();
    test_the_sound_seam();
    printf("bozo ok\n");
    return 0;
}
