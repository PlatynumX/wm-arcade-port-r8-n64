/*
 * ANIM.ASM:1277 ANI_CODE's targets, translated.
 *
 * `_ani_code` is the plainest opcode in the machine -- `move *a4+,a0,L /
 * call a0` -- but it is the single biggest hole in this port's animation
 * VM: 2303 uses across the eight playable wrestlers' sequence files. They
 * collapse to 145 distinct routines, and 66 of those cover 92% of the
 * uses, so the work is bounded by the routines, not by the call sites.
 *
 * What is translated here is the DCSSOUND.ASM leaf sound routines: the ones
 * that pick a real sound index out of a real table -- by RNDRNG0, by
 * WRESTLERNUM, or by RPT_COUNT -- and hand it to triple_sound. Every index
 * and every table below is transcribed from DCSSOUND.ASM, never invented.
 *
 * triple_sound itself (DCSSOUND.ASM:2061) is a four-channel priority mixer
 * over triple_sndtab, and it is NOT translated here: the sound's own index
 * is passed through to wm_anim_env's `sound` callback unchanged, and
 * channel arbitration belongs to a DCSSOUND port rather than to this
 * bridge. The port's audio layer is already a command queue, so an index
 * is exactly what it wants.
 *
 * Deliberately NOT translated yet, and left as named gaps: the announcer
 * speech group (CALL_MISSES, CALL_SPECIAL_MOVE, CALL_SETUP,
 * CALL_ANI_AVERAGE_MOVE, CALL_NASTY_MOVE, CALL_THROWN_OUT,
 * CALL_OTHER_AVERAGE). Those do not make a sound -- they CREATE a process
 * that sleeps and then calls ADD_IF_SILENT to arbitrate a phrase against
 * whatever the announcer is already saying. That needs the speech queue,
 * which is its own system.
 */
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wm_arcade_roster.h"
#include "wm/arcade/wmania_ring_geometry.h"
#include "wm/arcade/wm_arcade_butcount.h"

#include <string.h>

static void play(const wm_anim_env *env, uint16_t call) {
    /* `move a0,a0 / jrz` guards in the source: a zero table entry is "no
       sound", not sound zero. The spare roster slot below relies on it. */
    if (env && env->sound && call != 0u)
        env->sound(env->sound_user, call);
}

/* UTIL.ASM RNDRNG0, 0..max inclusive. With no RNG the source's own "first
   entry" is used rather than a made-up draw. */
static uint32_t rnd0(const wm_anim_env *env, uint32_t max_inclusive) {
    if (!env || !env->rng) return 0u;
    return wm_rng_rndrng0(env->rng, max_inclusive);
}

/*
 * UTIL.ASM:1734 RNDPER: stir RAND, take mul_high(1000, RAND) for a 0..999
 * draw, and `jrhi` -- the event happens when the probability exceeds the
 * draw. RNDRNG0(999) performs the identical stir and the identical
 * mul_high with the identical multiplier (999+1), so it reproduces RNDPER
 * exactly rather than approximating it.
 */
static bool rndper(const wm_anim_env *env, uint32_t per_mille) {
    if (!env || !env->rng) return false;
    return wm_rng_rndrng0(env->rng, 999u) < per_mille;
}

/* A wrestler-indexed table: WRESTLERNUM picks the row. The source indexes
   without a bounds test because WRESTLERNUM is always in range. */
static uint16_t by_wrestler(const wm_arcade_actor_t *actor,
                            const uint16_t *table, size_t n) {
    size_t i;
    if (!actor) return 0u;
    i = (size_t)actor->wrestler_num;
    return (i < n) ? table[i] : 0u;
}

/* ---------------------------------------------------------------- *
 * DCSSOUND.ASM:3999 HIT_THE_MAT -- by far the most-called ANI_CODE
 * routine in the game (339 uses). Two calls: a fixed one, then one of
 * three low-priority mat hits. The source's own commented-out head notes
 * an "outside the ring, use concrete instead" variant that was dropped
 * ("It doesn't sound right"), so there is deliberately no INRING test.
 * ---------------------------------------------------------------- */
static const uint16_t MAT_HITS[3] = { 0x76u, 0x77u, 0x78u };

static void hit_the_mat(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)actor;
    play(env, 0x0C1u);
    play(env, MAT_HITS[rnd0(env, 2u)]);
}

/* DCSSOUND.ASM:4020/4029. The source's own comment marks the third bounce
   entry "this one is way too loud"; it is kept as written. */
static const uint16_t SMALL_BOUNCE_SOUNDS[3] = { 0x0C0u, 0x0C2u, 0x00Du };
static const uint16_t SMALL_RUN_SOUNDS[3]    = { 0x0C0u, 0x0C2u, 0x0C0u };

static void small_bounce(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)actor;
    play(env, SMALL_BOUNCE_SOUNDS[rnd0(env, 2u)]);
}

static void small_run(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)actor;
    play(env, SMALL_RUN_SOUNDS[rnd0(env, 2u)]);
}

/* DCSSOUND.ASM:4045/4057 -- Doink's flame. */
static const uint16_t FLAME_SOUNDS[2]     = { 0x99u, 0x9Au };
static const uint16_t FLAME_HIT_SOUNDS[5] = { 0x9Du, 0x9Eu, 0x9Fu,
                                              0x0A0u, 0x0A1u };

static void do_flame_snd(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)actor;
    play(env, FLAME_SOUNDS[rnd0(env, 1u)]);
}

static void do_flame_hit_snd(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)actor;
    play(env, FLAME_HIT_SOUNDS[rnd0(env, 4u)]);
}

/* DCSSOUND.ASM:4134 DO_WAIL -- index 7 is the spare roster slot and is a
   real zero in the source, i.e. no sound. */
static const uint16_t WHICH_WAIL[9] = {
    0x25Fu, 0x270u, 0x20Du, 0x20Du, 0x20Du, 0x20Du, 0x20Du, 0u, 0x20Du
};

static void do_wail(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    play(env, by_wrestler(actor, WHICH_WAIL, 9u));
}

/*
 * DCSSOUND.ASM:4223 FIND_AND_KILL_ENDLESS and the two routines that start
 * an endless sound. ENDLESS_SOUND is a single global in the source -- one
 * looping sound at a time for the whole machine -- so it is one here too.
 * The kill path walks WHICH_CHANNEL to clear that channel's priority and
 * duration and re-issue the sound to stop it; this port's audio layer has
 * no channel state to clear, so what is modelled is the part that is
 * observable through the same seam: the sound stops being the endless one.
 */
static uint16_t endless_sound;

static void find_and_kill_endless(wm_arcade_actor_t *actor,
                                  const wm_anim_env *env) {
    (void)actor;
    (void)env;
    endless_sound = 0u;
}

static const uint16_t WHICH_CHOKE[9] = {
    0x21Au, 0x21Au, 0x21Au, 0x21Au, 0x21Au, 0x21Au, 0x21Au, 0u, 0x21Au
};
static const uint16_t WHICH_NONO[9] = {
    0x23Cu, 0x281u, 0x23Cu, 0x23Cu, 0x23Cu, 0x23Cu, 0x219u, 0u, 0x23Cu
};

static void do_choke(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    uint16_t call = by_wrestler(actor, WHICH_CHOKE, 9u);
    find_and_kill_endless(actor, env);
    play(env, call);
    endless_sound = call;
}

/* DO_OTHERNONO reads its OWN wrestler number; DO_NONO reads the number of
   the wrestler attached to it (*a13(ATTACH_PROC)), which is the one being
   held -- the two differ only in whose voice objects. */
static void do_othernono(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    uint16_t call = by_wrestler(actor, WHICH_NONO, 9u);
    find_and_kill_endless(actor, env);
    play(env, call);
    endless_sound = call;
}

static void do_nono(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    const wm_arcade_actor_t *held = actor ? actor->attach_proc : NULL;
    uint16_t call = by_wrestler(held, WHICH_NONO, 9u);
    find_and_kill_endless(actor, env);
    play(env, call);
    endless_sound = call;
}

/*
 * DCSSOUND.ASM:4153 DO_DOINK_SLAM: which taunt he uses depends on how far
 * through the repeat span he is. RPT_COUNT 0 or 1 is the plain slam;
 * otherwise DOINK_WHICH_SLAM[RPT_COUNT-1], and RPT_COUNT-1 >= 4 is silent.
 *
 * The table has three entries but that index can reach 3, one past the end
 * -- in the ROM that reads the first assembled word of the routine after
 * it. Reproducing a value this port cannot know would be inventing data, so
 * that one case plays nothing and says so here.
 */
static const uint16_t DOINK_WHICH_SLAM[3] = { 0x215u, 0x216u, 0x217u };

static void do_doink_slam(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    int32_t rpt = actor ? actor->rpt_count : 0;
    if (rpt == 0 || rpt == 1) { play(env, 0x218u); return; }
    if (rpt - 1 >= 4) return;
    if ((size_t)(rpt - 1) < 3u) play(env, DOINK_WHICH_SLAM[rpt - 1]);
}

/*
 * DCSSOUND.ASM:4069-4090, the shove taunts. Each entry point loads a
 * different table and falls into the shared DO_PUSH_SPEECH: a 50%
 * RNDPER(500) gate, then a check that no shove taunt is already running
 * (the source spawns a 60-tick DUMMY_WAIT process and tests a flag), then
 * one line out of that wrestler's table. The word BEFORE each table is its
 * own RNDRNG0 maximum -- `move *A10(-10H),A0` -- so Razor picks between
 * three and everyone else has exactly one.
 */
static const uint16_t RAZOR_WHICH[3] = { 0x27Eu, 0x27Fu, 0x280u };
static const uint16_t DOINK_WHICH[1] = { 0x282u };
static const uint16_t SHAWN_WHICH[1] = { 0x283u };
static const uint16_t BRET_WHICH[1]  = { 0x285u };
static const uint16_t LEX_WHICH[1]   = { 0x286u };

/* The source's razor_swear_exists flag, held for 60 ticks by DUMMY_WAIT.
   Counted down by wm_anim_code_tick so the lockout is real rather than
   permanently open. */
static uint16_t push_speech_lockout;

static void push_speech(const wm_anim_env *env, const uint16_t *table,
                        uint32_t max_inclusive) {
    if (!rndper(env, 500u)) return;
    if (push_speech_lockout) return;
    push_speech_lockout = 60u;
    play(env, table[rnd0(env, max_inclusive)]);
}

static void do_razor_push(wm_arcade_actor_t *a, const wm_anim_env *e)
    { (void)a; push_speech(e, RAZOR_WHICH, 2u); }
static void do_doink_push(wm_arcade_actor_t *a, const wm_anim_env *e)
    { (void)a; push_speech(e, DOINK_WHICH, 0u); }
static void do_shawn_push(wm_arcade_actor_t *a, const wm_anim_env *e)
    { (void)a; push_speech(e, SHAWN_WHICH, 0u); }
static void do_bret_push(wm_arcade_actor_t *a, const wm_anim_env *e)
    { (void)a; push_speech(e, BRET_WHICH, 0u); }
static void do_lex_push(wm_arcade_actor_t *a, const wm_anim_env *e)
    { (void)a; push_speech(e, LEX_WHICH, 0u); }

void wm_anim_code_tick(void) {
    if (push_speech_lockout) --push_speech_lockout;
}

void wm_anim_code_reset(void) {
    endless_sound = 0u;
    push_speech_lockout = 0u;
}

uint16_t wm_anim_code_endless_sound(void) { return endless_sound; }

/*
 * DCSSOUND.ASM:4295 DO_BLOCKED -- a 5% chance (RNDPER's argument is per
 * mille) of the blocker saying something, from a per-wrestler table whose
 * spare slot is a real zero.
 */
static const uint16_t WHICH_BLOCK_SPEECH[9] = {
    0x236u, 0x280u, 0x23Du, 0x23Eu, 0x284u, 0x06Au, 0x212u, 0u, 0x287u
};

static void do_blocked(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    if (!rndper(env, 50u)) return;
    play(env, by_wrestler(actor, WHICH_BLOCK_SPEECH, 9u));
}

/*
 * DCSSOUND.ASM:4319 MAKE_HIM_SCREAM / :4324 DO_SCREAM. Four screams per
 * wrestler, one picked at random. The two entry points differ only in
 * whose voice it is: MAKE_HIM_SCREAM screams as the wrestler he just hit
 * (WHOIHIT), DO_SCREAM as himself.
 *
 * The table's own comments mark which rows are borrowed -- Undertaker,
 * Shawn and Bam Bam all use Bret's screams, Lex uses Razor's -- and the
 * spare slot is four real zeros.
 */
static const uint16_t WHICH_SCREAM[9][4] = {
    { 0x265u, 0x266u, 0x262u, 0x263u },   /* Bret */
    { 0x268u, 0x269u, 0x26Fu, 0x26Cu },   /* Razor */
    { 0x265u, 0x266u, 0x262u, 0x263u },   /* Undertaker -- Bret's */
    { 0x268u, 0x269u, 0x26Fu, 0x26Cu },   /* Yokozuna -- Razor's */
    { 0x265u, 0x266u, 0x262u, 0x263u },   /* Shawn -- Bret's */
    { 0x265u, 0x266u, 0x262u, 0x263u },   /* Bam Bam -- Bret's */
    { 0x071u, 0x072u, 0x20Au, 0x20Cu },   /* Doink */
    { 0u,     0u,     0u,     0u     },   /* spare slot */
    { 0x268u, 0x269u, 0x26Fu, 0x26Cu }    /* Lex -- Razor's */
};

static void scream_as(const wm_arcade_actor_t *who, const wm_anim_env *env) {
    size_t i = who ? (size_t)who->wrestler_num : 0u;
    if (i >= 9u) return;
    play(env, WHICH_SCREAM[i][rnd0(env, 3u)]);
}

static void make_him_scream(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    scream_as(actor ? actor->who_i_hit : NULL, env);
}

static void do_scream(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    scream_as(actor, env);
}

/* DCSSOUND.ASM:4350 GOUGE_SOUND -- one fixed call, nothing else. */
static void gouge_sound(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)actor;
    play(env, 0x0A9u);
}

/*
 * DCSSOUND.ASM:4280 DO_RAZOR_RUG_SPEECH -- Razor's four lines as the rug
 * shake goes on, stepped by RPT_COUNT and silent once it has run past
 * them. RPT_COUNT 0 would index before the table in the ROM, so that case
 * plays nothing here rather than inventing what sat there.
 */
static const uint16_t RAZOR_RUG_TABLE[4] = { 0x27Du, 0x27Cu, 0x27Bu, 0x27Au };

static void do_razor_rug_speech(wm_arcade_actor_t *actor,
                                const wm_anim_env *env) {
    int32_t i = (actor ? actor->rpt_count : 0) - 1;
    if (i < 0 || i >= 4) return;
    play(env, RAZOR_RUG_TABLE[i]);
}

/*
 * DCSSOUND.ASM:4377 CALL_BONE_BREAK -- one of three crunches. The source
 * then tests triple_sound's own carry (did the call actually get a
 * channel?) and, only if it did, spawns a process that sleeps 50 ticks and
 * has the announcer say "did you hear that". That follow-up needs both the
 * channel arbitration this port does not model and the speech queue, so
 * what is translated here is the crunch itself.
 */
static const uint16_t BONE_BREAK_SOUNDS[3] = { 0x01Du, 0x09Bu, 0x098u };

static void call_bone_break(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)actor;
    play(env, BONE_BREAK_SOUNDS[rnd0(env, 2u)]);
}

/* ================================================================== *
 * The self-contained actor-state routines.
 *
 * These live in whichever wrestler's sequence file happened to define them
 * first -- ckzpos in DNKSEQ2, no_bk_xvel and tbukl_flip in SHNSEQ2/3, the
 * palette pair in DNKSEQ3 -- but they are plain global labels, so every
 * wrestler's animations call them. Each reads and writes only the actor it
 * is given, which is why they translate one-for-one.
 * ================================================================== */

/*
 * DNKSEQ2.ASM:1886 ckzpos, with the source's own comment: "If falling near
 * the front or rear ropes, slide toward middle of the ring to allow
 * opponent to walk around him." Above 510h he slides up the screen, below
 * 442h he slides down, and in between he is already clear.
 */
static void ckzpos(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    if (actor->z_int > 0x510) { actor->z_vel = -0x24000; return; }
    if (actor->z_int > 0x442) return;
    actor->z_vel = 0x24000;
}

/*
 * SHNSEQ3.ASM:3260 no_bk_xvel: kill any x velocity that is carrying him
 * BACKWARD. The source negates the velocity when FACING_DIR's right bit is
 * clear -- turning it into a forward-relative figure -- and zeroes it if
 * that comes out negative.
 */
static void no_bk_xvel(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    int32_t v;
    (void)env;
    if (!actor) return;
    v = actor->x_vel;
    if (!(actor->facing_dir & WM_MOVE_RIGHT)) v = -v;
    if (v < 0) actor->x_vel = 0;
}

/*
 * HRTSEQ4.ASM:1081 choose_2or4: report which facing bank the animation
 * should continue in, through MODE_STATUS -- clear for the 2-bank when
 * NEW_FACING_DIR points up, set for the 4-bank otherwise. The animation
 * then forks on it with an ordinary ANI_IFSTATUS.
 */
static void choose_2or4(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    if (actor->new_facing_dir & WM_MOVE_UP)
        actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
    else
        actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
}

/*
 * DNKSEQ3.ASM:428 am_I_dead: report through MODE_STATUS whether this
 * wrestler is finished. On zero health it also puts him in MODE_DEAD
 * itself. The live path is subtler than "clear the bit": it clears
 * MODE_STATUS but sets it straight back if he is ALREADY in MODE_DEAD, so
 * a wrestler who died earlier keeps answering yes even once the lifebar
 * has been reset.
 */
static void am_i_dead(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    if (actor->life == 0) {
        actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
        actor->player_mode = WM_PMODE_DEAD;
        return;
    }
    actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
    if (actor->player_mode == WM_PMODE_DEAD)
        actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
}

/*
 * DNKSEQ3.ASM's palette and constant-colour routines -- Doink's buzzer
 * flash. make_white and make_black differ only in the OBJ_CONST value they
 * load; both turn on DISPLAY.EQU:104's M_CONNON, "replace non-zero data
 * with constant". make_norm puts the ordinary SYS.EQU DMAWNZ write-mode
 * back. All three clear the low four control bits first.
 */
#define WM_DMA_MODE_MASK  0x000Fu
#define WM_M_CONNON       0x0008u   /* DISPLAY.EQU:104 */
#define WM_DMAWNZ         0x8002u   /* SYS.EQU:215 */
#define WM_M_TEMP_PAL     0x0004u   /* PLYR.EQU:399/422, 1<<B_TEMP_PAL */

static void set_dma_mode(wm_arcade_actor_t *actor, uint16_t mode) {
    actor->obj_control =
        (uint16_t)((actor->obj_control & (uint16_t)~WM_DMA_MODE_MASK) | mode);
}

static void make_white(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    actor->obj_const = 0x0101u;
    set_dma_mode(actor, WM_M_CONNON);
}

/*
 * #make_black is written once per sequence file, and the source says why
 * at every copy: "This is a black color within the wrestler's pal. It is
 * different for each wrestler." Seven distinct values across the eight
 * files -- Bam Bam and Doink happen to share 0B0Bh -- so the value comes
 * from the registry row for the file the call was made from, and getting
 * that wrong paints six wrestlers in Doink's black.
 */
static void make_black(wm_arcade_actor_t *actor, const wm_anim_env *env,
                       int32_t param) {
    (void)env;
    if (!actor) return;
    actor->obj_const = (uint16_t)param;
    set_dma_mode(actor, WM_M_CONNON);
}

static void make_norm(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    /* `andni 01111b / ori DMAWNZ` -- DMAWNZ is 8002h, so this ORs a whole
       write-mode word back in, not just a low bit. */
    set_dma_mode(actor, WM_DMAWNZ);
}

static void set_skeleton_pal(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    actor->obj_pal = actor->skeleton_pal;
    actor->status_flags |= WM_M_TEMP_PAL;
}

static void set_my_pal(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    actor->obj_pal = actor->my_pal;
    actor->status_flags &= ~(uint32_t)WM_M_TEMP_PAL;
}

/*
 * SHNSEQ2.ASM:1556 tbukl_flip and its sibling entry point face_inside.
 *
 * On a turnbuckle, which way he faces depends on which corner he is in and
 * whether the opponent is in the ring at all. tbukl_flip reads the
 * opponent's INRING (0 in, 1 out); face_inside is the same code entered
 * with that answer forced to "in", i.e. always turn to face the ring.
 *
 * When the opponent IS outside, neither corner decides it -- the source
 * falls through to #out and goes by NEW_FACING_DIR's left bit instead.
 *
 * The last twist is the source's own: "doink is the opposite... so is
 * yoko." Both of those two have their artwork drawn facing the other way,
 * so the final flip is inverted for them.
 */
static void turnbuckle_face(wm_arcade_actor_t *actor, bool opponent_outside) {
    /* The source's #yes (mirror) against #no (don't). Both corners send an
       outside opponent to the same #out test, so which corner he is in
       only decides it while the opponent is still in the ring -- and there
       the right corner mirrors and the left does not. */
    bool mirror = opponent_outside
        ? (actor->new_facing_dir & WM_MOVE_LEFT) != 0
        : (actor->x_int >= WM_RING_X_CENTER);

    /* "doink is the opposite... so is yoko" -- the source's own comment.
       Their artwork is drawn facing the other way. */
    if (actor->wrestler_num == WM_ROSTER_DOINK ||
        actor->wrestler_num == WM_ROSTER_YOKO)
        mirror = !mirror;

    if (mirror) actor->obj_control |= (uint16_t)WM_OBJ_FLIPH;
    else        actor->obj_control &= (uint16_t)~WM_OBJ_FLIPH;
}

static void tbukl_flip(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    bool outside = false;
    if (!actor) return;
    /* `calla get_opp_process / move *a0(INRING),a0`. PLYR.EQU's INRING is
       0 for "in the ring"; this port's in_ring is the ordinary boolean, so
       the test flips. With no opponent reachable the source would read a
       null process; treating that as in the ring keeps him facing inward
       rather than guessing. */
    if (env && env->opponent) outside = (env->opponent->in_ring == 0);
    turnbuckle_face(actor, outside);
}

static void face_inside(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    turnbuckle_face(actor, false);
}

/*
 * WRESTLE2.ASM:4967 free_toss_check -- report through MODE_STATUS whether
 * a free hiptoss is on. Two ways to earn it: the two wrestlers are within
 * 15 units of each other in z, or he is holding block. The block test is an
 * equality against PLAYER_BLOCK_VAL, not a bit test, so block AND anything
 * else does not count.
 *
 * The source's own commented-out lines show an earlier version that
 * required the stick pulled away instead; it was replaced by the block
 * test, and the dead lines are left in place there.
 */
static void free_toss_check(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    int32_t dz;
    if (!actor) return;
    actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
    if (env && env->opponent) {
        dz = env->opponent->z_fixed - actor->z_fixed;
        if (dz < 0) dz = -dz;
        if ((dz >> 16) < 15) return;
    }
    if (actor->but_val_cur == (uint16_t)WM_BTN_BLOCK) return;
    actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
}

/*
 * WRESTLE2.ASM:5004 setup_freetoss, with the source's own summary: "We're
 * gonna do a free hiptoss. Do all the neccesary setup here. Set our
 * PLYRMODE to normal, IMMOBILIZE the bad guy, clear velocities, etc."
 *
 * The SMRTTGT smart-target call at the end is a display service (it points
 * the camera's target at the victim) and is not modelled here.
 */
static void setup_freetoss(wm_arcade_actor_t *actor, const wm_anim_env *env,
int32_t param) {
    (void)param;
    (void)env;
    if (!actor) return;
    actor->player_mode = WM_PMODE_NORMAL;
    if (actor->who_i_hit) actor->who_i_hit->immobilize_time = 20;
}

/*
 * DNKSEQ2.ASM:5486 SET_DIR_FACE -- point NEW_FACING_DIR at the ring while
 * climbing in or out. The source's own note: "this is now used for both
 * climbins and climbouts. If we're outside, use an identical version, but
 * with the a0's switched." So the corner test is the same and only the two
 * answers trade places. 10 is DOWN|RIGHT, 6 is DOWN|LEFT.
 */
static void set_dir_face(wm_arcade_actor_t *actor, const wm_anim_env *env,
                         int32_t param) {
    bool left_of_centre;
    (void)env; (void)param;
    if (!actor) return;
    left_of_centre = actor->x_int < WM_RING_X_CENTER;
    /* `move *a13(INRING),a14 / jrz #method2` -- the source branches to its
       second version when INRING is 0, which is its own encoding for IN
       the ring. This port's in_ring is the ordinary boolean, so the test
       flips: in the ring takes #method2. */
    if (actor->in_ring != 0)
        actor->new_facing_dir = left_of_centre ? 6 : 10;    /* #method2 */
    else
        actor->new_facing_dir = left_of_centre ? 10 : 6;
}

/*
 * WRESTLE2.ASM:4951 set_tbukl_airmode -- leaping off a turnbuckle puts him
 * in MODE_INAIR2, the mode that says "this one is a turnbuckle leap", which
 * is what an opponent's own do_kick tests to answer it. Against a dead
 * opponent there is nothing to answer, so it is the plain MODE_INAIR.
 */
static void set_tbukl_airmode(wm_arcade_actor_t *actor, const wm_anim_env *env,
                              int32_t param) {
    (void)param;
    if (!actor) return;
    actor->player_mode =
        (env && env->opponent && env->opponent->player_mode == WM_PMODE_DEAD)
            ? WM_PMODE_INAIR : WM_PMODE_INAIR2;
}

/*
 * WRESTLE2.ASM:4527 check_raisearm_bit -- report through MODE_STATUS
 * whether he has NOT already done his raise-arm celebration. The sense is
 * inverted from what the name suggests: the bit being SET clears
 * MODE_STATUS, so the animation forks into the celebration only the first
 * time.
 */
#define WM_M_DID_RAISEARM (1u << 16)   /* PLYR.EQU:414 B_DID_RAISEARM */

static void check_raisearm_bit(wm_arcade_actor_t *actor,
                               const wm_anim_env *env, int32_t param) {
    (void)env; (void)param;
    if (!actor) return;
    if (actor->status_flags & WM_M_DID_RAISEARM)
        actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
    else
        actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
}

/*
 * DNKSEQ3.ASM:1516 clear_opp_counts, with the source's own comment: "Zero
 * opponents buttons for later counting." It clears all five of the wrestler
 * he is HOLDING, which is what makes the mash gates in a grapple measure
 * struggling from the moment the hold starts rather than from the round.
 */
static void clear_opp_counts(wm_arcade_actor_t *actor, const wm_anim_env *env,
                             int32_t param) {
    (void)env; (void)param;
    if (!actor || !actor->attach_proc) return;
    wm_arcade_clear_button_presses(actor->attach_proc);
}

/*
 * DNKSEQ3.ASM:1509 head_grab_time -- stamp when this head hold started,
 * then fall straight through into clear_opp_counts. (The CALL_SETUP
 * announcer line between them belongs to the speech queue and is not
 * translated.) The stamp is what the head-hold monitor reads to decide
 * whether a repeat grab within 120 ticks becomes a fake hold instead.
 */
static void head_grab_time(wm_arcade_actor_t *actor, const wm_anim_env *env,
                           int32_t param) {
    if (!actor) return;
    actor->last_headhold = (env ? env->pcnt : 0u);
    clear_opp_counts(actor, env, param);
}

/*
 * WRESTLE2.ASM:1622 halve_bk_xvel -- the gentler twin of no_bk_xvel: where
 * that one zeroes a velocity carrying him backward, this one halves it.
 */
static void halve_bk_xvel(wm_arcade_actor_t *actor, const wm_anim_env *env,
                          int32_t param) {
    int32_t v;
    (void)env; (void)param;
    if (!actor) return;
    v = actor->x_vel;
    if (!(actor->facing_dir & WM_MOVE_RIGHT)) v = -v;
    if (v < 0) actor->x_vel >>= 1;      /* the source's own `sra 1` */
}


/*
 * ANI_CODE:  #set_zvel2 -- one constant Z velocity, written once per
 * sequence file with a DIFFERENT value. Six wrestlers get -5.0, Doink
 * -6.75 and the Undertaker -5.75, which is exactly why the registry is
 * keyed on (name, file) and the value rides in `param`.
 */
static void set_zvel2(wm_arcade_actor_t *actor, const wm_anim_env *env,
                      int32_t param) {
    (void)env;
    if (actor) actor->z_vel = param;
}

/* HRTSEQ3.ASM #half_vels: halve the X velocity (the source's own `sra 1`)
   and set a fixed 2.0 upward. */
static void half_vels(wm_arcade_actor_t *actor, const wm_anim_env *env,
                      int32_t param) {
    (void)env; (void)param;
    if (!actor) return;
    actor->x_vel >>= 1;
    actor->y_vel = 0x20000;
}

/* SHNSEQ3.ASM #reverse_xvel: turn him round at a quarter of the speed. */
static void reverse_xvel(wm_arcade_actor_t *actor, const wm_anim_env *env,
                         int32_t param) {
    (void)env; (void)param;
    if (actor) actor->x_vel = (-actor->x_vel) >> 2;
}

/* DNKSEQ2.ASM #zero_x -- the source's own comment: "Don't float if
   dropping straight down". Only kills the drift when the opponent is
   underneath rather than off to the side. */
static void zero_x(wm_arcade_actor_t *actor, const wm_anim_env *env,
                   int32_t param) {
    (void)env; (void)param;
    if (actor && actor->closest_xdist <= 0x40) actor->x_vel = 0;
}

/*
 * SHNSEQ3.ASM / YOKSEQ3.ASM #store_opp_xvel and #merge_xvels, a pair
 * sharing the file's own `#opp_xvel` word: remember what the opponent was
 * doing, then average it into your own -- (mine + his) >> 2, which is a
 * quarter rather than a half, as written.
 */
static int32_t stored_opp_xvel;

static void store_opp_xvel(wm_arcade_actor_t *actor, const wm_anim_env *env,
                           int32_t param) {
    (void)param;
    stored_opp_xvel = (env && env->opponent) ? env->opponent->x_vel : 0;
    (void)actor;
}

static void merge_xvels(wm_arcade_actor_t *actor, const wm_anim_env *env,
                        int32_t param) {
    (void)env; (void)param;
    if (actor) actor->x_vel = (stored_opp_xvel + actor->x_vel) >> 2;
}

/*
 * The "throw the man you just hit" routines. All of them read WHOIHIT and
 * push him AWAY from the way he is facing: `movi -[n,0]` then negate when
 * NEW_FACING_DIR is left, so facing right sends him left.
 *
 * `param` packs the three velocities the file's own copy uses, because
 * each file writes different ones: y in the low byte, z in the next, x in
 * the next, all in whole units.
 */
static void throw_opp(wm_arcade_actor_t *actor, const wm_anim_env *env,
                      int32_t param) {
    wm_arcade_actor_t *v;
    int32_t x;
    (void)env;
    if (!actor || !actor->who_i_hit) return;
    v = actor->who_i_hit;
    v->y_vel = ((param >> 0) & 0xff) << 16;
    if ((param >> 8) & 0xff) v->z_vel = (int32_t)((param >> 8) & 0xff) << 16;
    x = (int32_t)((param >> 16) & 0xff) << 16;
    v->x_vel = (v->new_facing_dir & WM_MOVE_RIGHT) ? -x : x;
}

/* DNKSEQ3.ASM's own #set_opp_y is just the lift, with no push at all. */
static void lift_opp(wm_arcade_actor_t *actor, const wm_anim_env *env,
                     int32_t param) {
    (void)env;
    if (actor && actor->who_i_hit)
        actor->who_i_hit->y_vel = param;
}


/*
 * #set_zvel1 and #ckspin, written separately in every sequence file with
 * the same body: MODE_STATUS says which way up he is. Bit 0 of FACING_DIR
 * is MOVE_UP, and the status is set when it is CLEAR -- facing down.
 * Whatever reads it afterwards branches on that.
 */
static void status_from_facing(wm_arcade_actor_t *actor,
                               const wm_anim_env *env, int32_t param) {
    (void)env; (void)param;
    if (!actor) return;
    if (actor->facing_dir & WM_MOVE_UP)
        actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
    else
        actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
}

/*
 * DNKSEQ2.ASM/LEXSEQ2/RZRSEQ2/UNDSEQ2 #holdup: how long a leap may be
 * held up. BUT_COUNT counts the ticks; past the source's own limit of 25
 * ("Max time to hold up in air (*2 ticks)"), or the moment super kick is
 * released, MODE_STATUS clears and the animation moves on.
 */
static void holdup(wm_arcade_actor_t *actor, const wm_anim_env *env,
                   int32_t param) {
    int holding;
    (void)env; (void)param;
    if (!actor) return;
    ++actor->but_count;
    holding = actor->but_count <= 25 &&
              (actor->but_val_cur & (uint16_t)WM_BTN_SKICK);
    if (holding) actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
    else actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
}

/* HRTSEQ2.ASM NOT_IN_RING -- a global, and one of the places PLYR.EQU's
   INRING polarity bites: the source writes 1 for OUTSIDE, this port's
   field is the ordinary boolean. */
static void not_in_ring(wm_arcade_actor_t *actor, const wm_anim_env *env,
                        int32_t param) {
    (void)env; (void)param;
    if (actor) actor->in_ring = 0;
}

/* DNKSEQ3/YOKSEQ2 #set_zvel: give the man he hit a little Z drift, but
   only if he has none of his own. */
static void set_zvel_if_still(wm_arcade_actor_t *actor,
                              const wm_anim_env *env, int32_t param) {
    (void)env;
    if (actor && actor->who_i_hit && actor->who_i_hit->z_vel == 0)
        actor->who_i_hit->z_vel = param;
}


/*
 * hiptoss_delay and fling_delay -- the same routine over two different
 * PCNT stamps. Each records when it last ran and sets MODE_STATUS if the
 * gap is under three seconds, which the source's own comment explains:
 * "This blocked fling attempt is too close (in terms of time) to most
 * recent grab". Whatever follows branches on the status to refuse.
 *
 * `param` picks the stamp: 0 = LAST_HIPTOSS, 1 = LAST_FLING.
 */
static void grab_rate_limit(wm_arcade_actor_t *actor,
                            const wm_anim_env *env, int32_t param) {
    uint32_t *stamp, now, since;
    if (!actor) return;
    stamp = param ? &actor->last_fling : &actor->last_hiptoss;
    now = env ? env->pcnt : 0u;
    since = now - *stamp;
    *stamp = now;
    if ((int32_t)since >= 3 * 60)
        actor->anim_mode &= (uint16_t)~WM_MODE_STATUS;
    else
        actor->anim_mode |= (uint16_t)WM_MODE_STATUS;
}

/*
 * The registry. `file` NULL means a global label, which resolves the same
 * from anywhere; a non-NULL file scopes the row to that sequence file,
 * which is what a leading '#' in the source means.
 */
static const struct {
    const char *name;
    const char *file;      /* NULL = global */
    wm_anim_code_fn fn;
    int32_t param;
} code_table[] = {
    /*
     * #set_zvel2, one row per file because the constant differs:
     * BAM/HRT/LEX/RZR/SHN/YOK -5.0, DNK -6.75, UND -5.75.
     */
    { "#set_zvel2", "BAMSEQ2.ASM", set_zvel2, -0x50000 },
    { "#set_zvel2", "HRTSEQ2.ASM", set_zvel2, -0x50000 },
    { "#set_zvel2", "LEXSEQ2.ASM", set_zvel2, -0x50000 },
    { "#set_zvel2", "RZRSEQ2.ASM", set_zvel2, -0x50000 },
    { "#set_zvel2", "SHNSEQ2.ASM", set_zvel2, -0x50000 },
    { "#set_zvel2", "YOKSEQ2.ASM", set_zvel2, -0x50000 },
    { "#set_zvel2", "DNKSEQ2.ASM", set_zvel2, -0x6c000 },
    { "#set_zvel2", "UNDSEQ2.ASM", set_zvel2, -0x5c000 },

    { "#half_vels", "HRTSEQ3.ASM", half_vels, 0 },
    { "#reverse_xvel", "SHNSEQ3.ASM", reverse_xvel, 0 },
    { "#zero_x", "DNKSEQ2.ASM", zero_x, 0 },
    { "#store_opp_xvel", "SHNSEQ3.ASM", store_opp_xvel, 0 },
    { "#store_opp_xvel", "YOKSEQ3.ASM", store_opp_xvel, 0 },
    { "#merge_xvels", "SHNSEQ3.ASM", merge_xvels, 0 },
    { "#merge_xvels", "YOKSEQ3.ASM", merge_xvels, 0 },

    /* x/z/y in whole units, per the file's own copy. BAMSEQ3/LEXSEQ3's
       #set_opp_y is 5 up, 2 along Z and 3 away; BAMSEQ2's #set_opp_xy is
       2 up and 2 away with no Z; DNKSEQ3's #set_opp_y only lifts. */
    { "#set_opp_y", "BAMSEQ3.ASM", throw_opp, (3 << 16) | (2 << 8) | 5 },
    { "#set_opp_y", "LEXSEQ3.ASM", throw_opp, (3 << 16) | (2 << 8) | 5 },
    { "#set_opp_xy", "BAMSEQ2.ASM", throw_opp, (2 << 16) | (0 << 8) | 2 },
    { "#set_opp_y", "DNKSEQ3.ASM", lift_opp, 0x40000 },

    /* #set_zvel1 and #ckspin: same body, written once per file. */
    { "#set_zvel1", "BAMSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "BAMSEQ2.ASM", status_from_facing, 0 },
    { "#set_zvel1", "DNKSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "DNKSEQ2.ASM", status_from_facing, 0 },
    { "#set_zvel1", "HRTSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "HRTSEQ2.ASM", status_from_facing, 0 },
    { "#set_zvel1", "LEXSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "LEXSEQ2.ASM", status_from_facing, 0 },
    { "#set_zvel1", "RZRSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "RZRSEQ2.ASM", status_from_facing, 0 },
    { "#set_zvel1", "SHNSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "SHNSEQ2.ASM", status_from_facing, 0 },
    { "#set_zvel1", "UNDSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "UNDSEQ2.ASM", status_from_facing, 0 },
    { "#set_zvel1", "YOKSEQ2.ASM", status_from_facing, 0 },
    { "#ckspin", "YOKSEQ2.ASM", status_from_facing, 0 },

    { "#set_zvel3", "DNKSEQ2.ASM", set_zvel2, -0x7c000 },
    { "#set_zvel", "DNKSEQ3.ASM", set_zvel_if_still, 0x10000 },
    { "#set_zvel", "YOKSEQ2.ASM", set_zvel_if_still, 0x10000 },
    { "NOT_IN_RING", NULL, not_in_ring, 0 },
    { "#holdup", "DNKSEQ2.ASM", holdup, 0 },
    { "#holdup", "LEXSEQ2.ASM", holdup, 0 },
    { "#holdup", "RZRSEQ2.ASM", holdup, 0 },
    { "#holdup", "UNDSEQ2.ASM", holdup, 0 },

    /* Both are SUBRs, so global: the same limiter over two stamps. */
    { "hiptoss_delay", NULL, grab_rate_limit, 0 },
    { "fling_delay", NULL, grab_rate_limit, 1 },
    { "ckzpos", NULL, ckzpos, 0 },
    { "SET_DIR_FACE", NULL, set_dir_face, 0 },
    { "set_tbukl_airmode", NULL, set_tbukl_airmode, 0 },
    { "check_raisearm_bit", NULL, check_raisearm_bit, 0 },
    { "clear_opp_counts", NULL, clear_opp_counts, 0 },
    { "head_grab_time", NULL, head_grab_time, 0 },
    { "halve_bk_xvel", NULL, halve_bk_xvel, 0 },
    { "free_toss_check", NULL, free_toss_check, 0 },
    { "setup_freetoss", NULL, setup_freetoss, 0 },
    { "no_bk_xvel", NULL, no_bk_xvel, 0 },
    { "choose_2or4", NULL, choose_2or4, 0 },
    { "am_I_dead", NULL, am_i_dead, 0 },
    { "make_white", NULL, make_white, 0 },
    { "make_norm", NULL, make_norm, 0 },
    { "set_skeleton_pal", NULL, set_skeleton_pal, 0 },
    { "set_my_pal", NULL, set_my_pal, 0 },
    { "tbukl_flip", NULL, tbukl_flip, 0 },
    { "face_inside", NULL, face_inside, 0 },
    { "HIT_THE_MAT", NULL, hit_the_mat, 0 },
    { "DO_BLOCKED", NULL, do_blocked, 0 },
    { "MAKE_HIM_SCREAM", NULL, make_him_scream, 0 },
    { "DO_SCREAM", NULL, do_scream, 0 },
    { "GOUGE_SOUND", NULL, gouge_sound, 0 },
    { "DO_RAZOR_RUG_SPEECH", NULL, do_razor_rug_speech, 0 },
    { "CALL_BONE_BREAK", NULL, call_bone_break, 0 },
    { "SMALL_BOUNCE", NULL, small_bounce, 0 },
    { "SMALL_RUN", NULL, small_run, 0 },
    { "DO_FLAME_SND", NULL, do_flame_snd, 0 },
    { "DO_FLAME_HIT_SND", NULL, do_flame_hit_snd, 0 },
    { "DO_WAIL", NULL, do_wail, 0 },
    { "DO_CHOKE", NULL, do_choke, 0 },
    { "DO_NONO", NULL, do_nono, 0 },
    { "DO_OTHERNONO", NULL, do_othernono, 0 },
    { "DO_DOINK_SLAM", NULL, do_doink_slam, 0 },
    { "FIND_AND_KILL_ENDLESS", NULL, find_and_kill_endless, 0 },
    { "DO_RAZOR_PUSH", NULL, do_razor_push, 0 },
    { "DO_DOINK_PUSH", NULL, do_doink_push, 0 },
    { "DO_SHAWN_PUSH", NULL, do_shawn_push, 0 },
    { "DO_BRET_PUSH", NULL, do_bret_push, 0 },
    { "DO_LEX_PUSH", NULL, do_lex_push, 0 },
    /* #make_black, once per file, each with its own palette black. */
    { "#make_black", "HRTSEQ4.ASM", make_black, 0x2F2F },
    { "#make_black", "RZRSEQ3.ASM", make_black, 0x0D0D },
    { "#make_black", "UNDSEQ3.ASM", make_black, 0x3F3F },
    { "#make_black", "YOKSEQ3.ASM", make_black, 0x0F0F },
    { "#make_black", "SHNSEQ4.ASM", make_black, 0x2121 },
    { "#make_black", "BAMSEQ3.ASM", make_black, 0x0B0B },
    { "#make_black", "DNKSEQ3.ASM", make_black, 0x0B0B },
    { "#make_black", "LEXSEQ3.ASM", make_black, 0x1A1A },
};

bool wm_anim_code_run(wm_arcade_actor_t *actor, const wm_anim_env *env,
                      const char *name, const char *source_file) {
    size_t i;
    const size_t n = sizeof(code_table) / sizeof(code_table[0]);
    if (!name) return false;
    /* A row scoped to this exact file wins over a global of the same name;
       nothing in the source relies on that today, but a local label really
       does shadow, so resolving in that order is the assembler's own. */
    for (i = 0; i < n; ++i)
        if (code_table[i].file && source_file &&
            strcmp(code_table[i].name, name) == 0 &&
            strcmp(code_table[i].file, source_file) == 0) {
            code_table[i].fn(actor, env, code_table[i].param);
            return true;
        }
    for (i = 0; i < n; ++i)
        if (!code_table[i].file && strcmp(code_table[i].name, name) == 0) {
            code_table[i].fn(actor, env, code_table[i].param);
            return true;
        }
    return false;
}

size_t wm_anim_code_count(void) {
    return sizeof(code_table) / sizeof(code_table[0]);
}
