#include "wm/match.h"
#include "wm/bret_backend.h"
#include "wm/wrestler_backend.h"
#include "wm/arcade/wm_arcade_auto_pin.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm/arcade/wm_arcade_react_anims.h"
#include "wm/arcade/wm_arcade_react5_core.h"
#include "wm/arcade/wm_arcade_buddies.h"
#include "wm/announce_tables.h"
#include "wm/award.h"
#include "wm/anim_program.h"
#include "wm/arcade/wm_arcade_butcount.h"
#include "wm/arcade/wm_arcade_veladd.h"
#include "wm/arcade/wm_arcade_roll.h"
#include "wm/arcade/wmania_rope_source_data.h"
#include <string.h>

/* PLYR.EQU: PSIDE_PLYR1 equ 0, PSIDE_PLYR2 equ 1. */
#define WM_MATCH_PSIDE_PLYR1 0
#define WM_MATCH_PSIDE_PLYR2 1

/*
 * WRESTLE.ASM:2691-2755 (#set0, wrestler-placement init): a wrestler's real
 * starting OBJ_XPOS/OBJ_ZPOS and NEW_FACING_DIR/FACING_DIR all come from the
 * SAME #teamX_starts table row (WRESTLE.ASM:2782-2792), X,Z,face_dir,unused
 * quads. #set0 picks the row by COUNTING how many wrestlers are already on
 * your side (the `#loop0` walk above it), so the first on a team gets row 0
 * and buddy mode's partner, arriving second, gets row 1.
 * RING_X_CENTER (RING.EQU:22) = 0400h+50 = 1074.
 */
#define WM_MATCH_RING_X_CENTER 1074
#define WM_MATCH_P1_START_X (WM_MATCH_RING_X_CENTER - 85)
#define WM_MATCH_P1_START_Z (1127 + 93)
#define WM_MATCH_P1_START_FACING WM_MOVE_UP_RIGHT
#define WM_MATCH_P2_START_X (WM_MATCH_RING_X_CENTER + 85)
#define WM_MATCH_P2_START_Z (1103 + 93)
#define WM_MATCH_P2_START_FACING WM_MOVE_DOWN_LEFT

/* `#team1_starts` / `#team2_starts` row 1, "second" -- where buddy
   mode's partner stands. Facing 9 and 5, as written. */
#define WM_MATCH_T1_SECOND_X (WM_MATCH_RING_X_CENTER - 150)
#define WM_MATCH_T1_SECOND_Z (1127 + 170)
#define WM_MATCH_T1_SECOND_FACING WM_MOVE_UP_RIGHT
#define WM_MATCH_T2_SECOND_X (WM_MATCH_RING_X_CENTER + 150)
#define WM_MATCH_T2_SECOND_Z (1103 + 170)
#define WM_MATCH_T2_SECOND_FACING WM_MOVE_UP_LEFT


/*
 * WRESTLE.ASM:1598 `callr init_scroller`, early in start_match --
 * after pal_clean and the display init, before the ring is built and
 * before any wrestler process is created. That placement is the
 * source's and is why this is called where it is rather than beside
 * the other per-match setup below.
 *
 * The routine itself is wm_round_init_scroller
 * (wm/arcade/wm_arcade_round_reset.h), which reads both of its Y
 * values off the source and picks between them on NUM_OPPS; it is
 * shared with LIFEBAR.ASM:3128, where the round reset calls the same
 * routine again. NUM_OPPS is 1 here for the reason wm/match.h gives:
 * no ladder table, so no team is ever drawn.
 */
static void wm_match_init_scroller(wm_match_state *m) {
    if (!m) return;
    wm_round_init_scroller(&m->scroll.worldtlx, &m->scroll.worldtly, 1);
}

/*
 * ANIM.ASM's rope opcodes reaching ROPES.ASM. The animation says which
 * bank and what to do to it; the rope processes belong to the match, so
 * the VM calls out through wm_anim_env and this is what it lands on.
 */
/*
 * DO_END_STUFF's health sweep: `MOVI NUM_WRES,A1 / get_health / CMPI 40`.
 * The match knows both actors, so this is the whole sweep here.
 */
/* AWARD.ASM round_award for the hit path, whose callback carries the
   attacker rather than a player number. */
static void wm_match_first_hit_award(wm_arcade_actor_t *attacker, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || !attacker || !m->anim_round_award) return;
    m->anim_round_award(m->anim_award_user, (int)attacker->player_num,
                        WM_AWARD_FIRST_HIT);
}

/* AWARD.ASM's round_award, for a backend that needs RND_AWARD. */
static void match_backend_round_award(void *user, int player_num,
                                      int award_index) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || !m->anim_round_award) return;
    m->anim_round_award(m->anim_award_user, player_num, award_index);
}

static bool match_anyone_near_death(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned i;
    if (!m) return false;
    for (i = 0; i < m->actor_count; ++i)
        if (m->actors[i].life < WM_ANNOUNCE_END_GAME_HEALTH) return true;
    return false;
}

/*
 * WRESTLE.ASM process_ptrs, the array of wrestler processes indexed by
 * PLYRNUM. A slot is a pointer, so an empty one is NULL; here an actor
 * that is not `active` is the same thing.
 */
static wm_arcade_actor_t *match_actor_by_plyrnum(void *user, int32_t plyrnum) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned i;
    if (!m || plyrnum < 0) return NULL;
    for (i = 0; i < m->actor_count; ++i)
        if (m->actors[i].active && m->actors[i].player_num == plyrnum)
            return &m->actors[i];
    return NULL;
}

/* WRESTLE2.ASM:3896 kill_smove_procs, reached through the env. */
static void match_kill_smoves(void *user, wm_arcade_actor_t *a) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned i;
    if (!m || !a) return;
    for (i = 0; i < m->actor_count; ++i) {
        if (&m->actors[i] != a) continue;
        wm_smove_kill(m->smoves[i], m->smove_count[i]);
        return;
    }
}

static void match_rope_command(void *user, int bank, int action,
                               int selector, int32_t wrestler_z_fp16) {
    wm_match_state *m = (wm_match_state *)user;
    WmRopeCommand cmd;
    if (!m || bank < 0 || bank >= WM_MATCH_ROPE_BANKS) return;
    if (!wm_rope_resolve_command((WmRopeBank)bank, (WmRopeAction)action,
                                 (uint8_t)selector, wrestler_z_fp16, &cmd))
        return;   /* the same table-invalid cases rope_command rejects */
    (void)wm_rope_runtime_apply_resolved_command(
        &m->ropes[bank], &cmd, wm_rope_source_program_resolver, NULL);
}

/*
 * WRESTLE.ASM:6069 shake_all_ropes, reached the OTHER way.
 *
 * There are two callers in the source and this port had only one of
 * them. DNKSEQ2's four `WL ANI_CODE,shake_all_ropes` rows go through the
 * animation opcode of the same name (src/core/anim_code.c), which has
 * been right since the rope work landed. REACT4.ASM:196 calls it as a
 * plain routine from the bounce helper -- and that seam
 * (wm_arcade_react1_callbacks_t::shake_all_ropes) was declared,
 * NULL-checked and never filled, so a bounce off a stomped wrestler
 * shook nothing.
 *
 * The body is the source's, unchanged: ROPE_BOUNCEUD with selector 2 on
 * all four banks. Its `NUM_OPPS >= 2` suppression is commented out in
 * the source and stays that way.
 */
static void match_react_shake_all_ropes(void *user) {
    static const int banks[4] = { WM_ROPE_FRONT, WM_ROPE_BACK,
                                  WM_ROPE_LEFT, WM_ROPE_RIGHT };
    wm_match_state *m = (wm_match_state *)user;
    size_t i;
    if (!m) return;
    for (i = 0; i < 4; ++i)
        match_rope_command(m, banks[i], (int)WM_ROPE_BOUNCE_UD, 2, 0);
}

/*
 * REACT4.ASM:199 `movi 8,a10 / calla SHAKER2` -- the screen shake that
 * goes with it, and the second seam that was never filled.
 *
 * NOT named after the seam, deliberately. UTIL.ASM's SHAKER2 is ledgered
 * as renamed onto wm_shake_tick precisely because it "resolved only
 * through a callback field called shaker2 until seam names stopped
 * counting" -- and an adapter called match_react_shaker2 puts that
 * accident straight back, this time as a definition the carve-out
 * cannot reach. The ledger's own anti-shadow guard caught it, which is
 * what that guard is for.
 */
static void match_react_screen_shake(int amount, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    wm_shake_start(&m->shake, amount);
}

/*
 * REACT5's `calla ditch_getup_meter` -- REACT1.ASM:798's own call, on a
 * wrestler who was BOUNCING or RUNNING when he was hit.
 *
 * The third unfilled seam, and the one that had nothing behind it until
 * now: wm_arcade_ditch_getup_meter is this commit's neighbour
 * (wm/arcade/wm_arcade_getup_meter.h). REACT5 has already checked
 * GETUP_TIME and METER_PROC before it calls, which are two of the
 * routine's own three guards; PLYR_DIZZY is the third and is checked
 * inside, where the source checks it.
 */
static void match_react_slide_getup_meter(wm_arcade_actor_t *attacker,
                                          void *user) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned i;
    if (!m || !attacker) return;
    for (i = 0; i < m->actor_count; ++i)
        if (&m->actors[i] == attacker) {
            (void)wm_arcade_ditch_getup_meter(attacker, &m->getup_meter[i]);
            return;
        }
}

/*
 * REACT1.ASM:1453 and REACT4.ASM:137, the two places REACT compares the
 * ATTACKER's ANIBASE against named animation labels.
 *
 * Both seams were declared, NULL-checked at their call sites and filled
 * by nobody, so every label comparison in REACT answered "not that one".
 * What that cost is specific and is written out at each one below.
 *
 * The label an attacker is playing lives in whichever backend drives
 * him: the shared one keeps the source label itself in current_label,
 * and Bret's typed one keeps an id whose sequence carries the label. So
 * this is a lookup, not a guess -- and it is the reason the seams are
 * here in the match rather than inside REACT, which has no backend.
 */
static const char *match_attacker_source_label(wm_match_state *m,
                                               const wm_arcade_actor_t *a) {
    unsigned i;
    if (!m || !a) return NULL;
    for (i = 0; i < m->actor_count; ++i) {
        if (&m->actors[i] != a) continue;
        if (a->wrestler_num == WM_ROSTER_BRET) {
            const wm_visual_sequence *seq =
                wm_bret_anim_sequence(m->bret_visual[i].current_id);
            return seq ? seq->source_label : NULL;
        }
        return m->wrestler_visual[i].current_label;
    }
    return NULL;
}

/*
 * REACT1.ASM:1453, in the flying-kick handler, under the source's own
 * comment: "HACK! - Lex's flying kicks don't knock you down. Use
 * CALL_MID_HIT."
 *
 *      move  *a10(ANIBASE),a14,L
 *      cmpi  lex_flying_kick_anim,a14
 *      jreq  #midhit
 *      cmpi  lex_super_kick_anim,a14
 *      jreq  #midhit
 *      calla CALL_DROP_KICK
 *
 * Two labels, and only Lex's. With this seam empty every flying kick in
 * the game took CALL_DROP_KICK, so Lex's two kicks knocked opponents
 * down when the source says they must not -- a rule about one wrestler
 * that was silently applied to nobody.
 */
static int match_react_attacker_uses_lex_flykick(
        const wm_arcade_actor_t *attacker, void *user) {
    const char *label = match_attacker_source_label((wm_match_state *)user,
                                                   attacker);
    if (!label) return 0;
    return strcmp(label, "lex_flying_kick_anim") == 0 ||
           strcmp(label, "lex_super_kick_anim") == 0;
}

/*
 * REACT4.ASM:137 hit_stomp, under "more nasty hacks...". Five labels,
 * two separate comparisons, three different outcomes:
 *
 *   shn_combo_run_stomp_anim, shn_run_stomp_anim -> DO_SCREAM instead of
 *       the LBOWDROP sound pair and `triple_sound 43h`.
 *   dnk_belly_anim, und_flying_butt_drop_anim -> #bounce, which throws
 *       the ATTACKER back up off the downed man's chest and shakes the
 *       ropes and the screen with him.
 *   lex_flying_ground_punch_anim -> #bounce3, the same bounce and then a
 *       Y velocity overwrite, under the source's own warning "REALLY
 *       NASTY HACK! Watch out if you modify this in any way."
 *
 * Empty, this seam returned WM_R_ANIMTAG_OTHER for everything: Shawn's
 * two run stomps played the ordinary elbow-drop sound instead of a
 * scream, and Doink's belly flop, the Undertaker's butt drop and Lex's
 * flying ground punch all landed on the mat and stayed there.
 */
static wm_arcade_react_anim_tag_t match_react_attacker_anim_tag(
        const wm_arcade_actor_t *attacker, void *user) {
    static const struct { const char *label; wm_arcade_react_anim_tag_t tag; }
    tags[] = {
        { "shn_combo_run_stomp_anim",     WM_R_ANIMTAG_SHN_COMBO_RUN_STOMP },
        { "shn_run_stomp_anim",           WM_R_ANIMTAG_SHN_RUN_STOMP },
        { "dnk_belly_anim",               WM_R_ANIMTAG_DNK_BELLY },
        { "und_flying_butt_drop_anim",    WM_R_ANIMTAG_UND_FLYING_BUTT_DROP },
        { "lex_flying_ground_punch_anim", WM_R_ANIMTAG_LEX_FLYING_GROUND_PUNCH }
    };
    const char *label = match_attacker_source_label((wm_match_state *)user,
                                                    attacker);
    size_t i;
    if (!label) return WM_R_ANIMTAG_OTHER;
    for (i = 0; i < sizeof tags / sizeof tags[0]; ++i)
        if (strcmp(label, tags[i].label) == 0) return tags[i].tag;
    return WM_R_ANIMTAG_OTHER;
}

/*
 * ANIM.ASM:41 _ani_rope_z / set_rope_z. Only the second half's Z is
 * decided by the action -- RZ_HIGH is a fixed value, RZ_NORM copies the
 * first half -- and that is what wm_rope_second_half_z returns. The strand
 * selects which of the bank's three ropes; this port's runtime keeps its
 * channels rather than per-strand Z, so the value is computed and applied
 * to the bank's shared state rather than to one rope's object.
 */
/*
 * CROWD.ASM:1130 crowd_cheer and DCSSOUND.ASM:3046's SNDSND, held for a
 * CROWD.ASM port to read. Nothing here decides anything: it records what
 * the source's arguments were, and runs CROWD_DUMMY's clock.
 */
static void match_crowd_cheer(void *user, int flags, int percent) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    m->crowd.flags = flags;
    m->crowd.percent = percent;
    ++m->crowd.cheers;
}

static void match_crowd_sound(void *user, int sound, int ticks) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    m->crowd.sound = sound;
    m->crowd.sound_ticks = (uint16_t)(ticks < 0 ? 0 : ticks);
    ++m->crowd.sounds;
}

/* `move @crowd_dummy_exists,a0 / JRNZ NO_CROWD_ALREADY_GOING` */
static bool match_crowd_busy(void *user) {
    const wm_match_state *m = (const wm_match_state *)user;
    return m && m->crowd.sound_ticks != 0u;
}

/*
 * SPECIAL.ASM's DEBRIS_PID creations, held for a SPECIAL.ASM port. The
 * routine has already decided whether it may run, on whom, and how many;
 * what a debris object then does is sprite work this port cannot do.
 */
static void match_create_debris(void *user, const char *effect,
                                wm_arcade_actor_t *at, int count,
                                int32_t yoff) {
    wm_match_state *m = (wm_match_state *)user;
    (void)at;
    if (!m || count <= 0) return;
    m->debris.last_effect = effect;
    m->debris.last_count = count;
    m->debris.last_yoff = yoff;
    m->debris.created += (uint32_t)count;
}

/*
 * LIFEBAR.ASM:3444 MOVE_NAME_ANNC. The three gates are translated
 * (wm_move_name_should_draw); what is left is the drawing, which needs a
 * renderer, so it is recorded for one.
 */
static void match_draw_move_name(void *user, int side, int index) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    if (!wm_move_name_should_draw(&m->move_names, side, index,
                                  m->debris.reduce_bog > 0))
        return;
    m->move_name.side = side;
    m->move_name.index = index;
    m->move_name.image = (index >= 0 && index < WM_MOVE_NAME_COUNT)
                             ? wm_move_name_images[index] : 0;
    ++m->move_name.drawn;
}

/* UTIL.ASM:2406 SHAKER2. The oscillator is real; what reads WORLDTLY is
   a camera this port has not got. */
static void match_screen_shake(void *user, int32_t ticks) {
    wm_match_state *m = (wm_match_state *)user;
    if (m) wm_shake_start(&m->shake, ticks);
}

/* SPECIAL.ASM:87 create_dizzy_proc. The gate and the per-wrestler offset
   are in the VM; the star itself is a DEBRIS_PID object. */
static void match_create_dizzy(void *user, wm_arcade_actor_t *at,
                               int32_t xoff, int32_t yoff) {
    wm_match_state *m = (wm_match_state *)user;
    (void)at;
    if (!m) return;
    m->dizzy.xoff = xoff;
    m->dizzy.yoff = yoff;
    ++m->dizzy.created;
}

/* ANIM.ASM:4403 ANI_SET_IDIOT -- WRESTLE.ASM:257 allow_offscrn. */
static void match_set_allow_offscrn(void *user, int32_t ticks) {
    wm_match_state *m = (wm_match_state *)user;
    if (m) m->allow_offscrn = ticks;
}

/*
 * ANI_LOOP's own reason for existing beyond parking: if announce_rnd_winner
 * is asleep at arw_bwait waiting to see whether anybody bucks off, a pinned
 * wrestler reaching the loop wakes it NOW rather than letting it sleep out
 * its 90 ticks.
 */
static void match_wake_round_announce(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    if (m->round_announce.phase != (uint8_t)WM_ARW_BUCKOFF_WAIT) return;
    m->round_announce.sleep_left = 0;      /* `movk 1,a14 / move a14,*a0(PTIME)` */
}

/*
 * ANIM.ASM change_anim1a, the UNGUARDED form: it starts the program
 * whether or not the same one is already running. The dispatchers
 * call the guarded change_anim1 every tick they stay in a mode; this
 * is what everything that reaches ACROSS to another wrestler uses --
 * ANI_SLAVEANIM, grnd_hit, and the coffin sequence's push_to_coffin
 * and make_wres_disappear.
 *
 * Each wrestler's animation lives in his own backend's exec, and
 * which backend that is depends on who he is (wm/match.h), so this
 * finds the actor's slot rather than being handed one.
 */
static void match_change_anim(wm_arcade_actor_t *actor, const char *label,
                              void *user) {
    wm_match_state *m = (wm_match_state *)user;
    const wm_anim_program *prog;
    unsigned i;

    if (!m || !actor || !label) return;
    prog = wm_anim_program_find(label);
    if (!prog) return;

    for (i = 0; i < m->actor_count; ++i) {
        if (&m->actors[i] != actor) continue;
        if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
            /* Bret's own backend selects by a typed id, and a label
               reaching him from another wrestler's sequence has no id
               -- his current_id is left alone deliberately, so his
               dispatcher's next selection restarts him normally. */
            wm_anim_exec_start(&m->bret_visual[i].prog, prog, actor,
                               (uint16_t)m->tick_count,
                               &m->bret_visual[i].anim_env);
        } else {
            m->wrestler_visual[i].current_label = label;
            wm_anim_exec_start(&m->wrestler_visual[i].prog, prog, actor,
                               (uint16_t)m->tick_count,
                               &m->wrestler_visual[i].anim_env);
        }
        return;
    }
}

/*
 * ANIM.ASM:3634 ANI_CREATEPROC. Six processes are ever named, and the
 * two that belong to the coffin finish are the two this port can run:
 *
 *   und_coffin_up  the mat, the coffin and the hover, as timing and a
 *                  handshake (wm/arcade/wm_arcade_coffin.h). Its
 *                  objects are a renderer's; its clock is the game's,
 *                  and everything else waits on it.
 *   raise_dead     `SLEEP TSEC/2`, change_anim1a(raise_dead_anim) on
 *                  @dead_wrestler, DIE. That is the whole routine.
 *
 * The other four are CREATE_SWEAT, SPIN_SWEAT and the Undertaker's
 * tomb bits, all of which only make sprites.
 */
static void match_create_proc(void *user, wm_arcade_actor_t *at,
                              const char *proc, int proc_id,
                              int32_t a, int32_t b, int32_t c) {
    wm_match_state *m = (wm_match_state *)user;
    (void)proc_id; (void)a; (void)b; (void)c;
    if (!m || !proc) return;
    if (strcmp(proc, "und_coffin_up") == 0) {
        wm_coffin_driver_start(&m->coffin_driver, &m->coffin);
        return;
    }
    if (strcmp(proc, "raise_dead") == 0) {
        m->raise_dead_delay = WM_COFFIN_RAISE_DEAD_SLEEP;
        /* @dead_wrestler, which TAKER.ASM latched from the
           Undertaker's WHOIHIT before the sequence started. */
        m->raise_dead_target = wm_coffin_dead_wrestler(&m->coffin, at);
        return;
    }
}

/* @in_finish_move (TAKER.ASM:676 and :703). Two things read it: the
   scroller stops while it is set, and announce_rnd_winner holds off
   calling the round. */
static void match_set_in_finish_move(int on, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    m->in_finish_move = (on != 0);
}

/* TAKER.ASM:715 adjust_view's WORLDTLX/WORLDTLY writes, each followed
   in the source by BGND_UD1 -- a background refresh this port has no
   renderer to need. */
static void match_set_world_origin(int32_t tlx, int32_t tly, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    m->scroll.worldtlx = tlx;
    m->scroll.worldtly = tly;
}


/*
 * REACT's per-move grading: `calla CALL_NASTY_MOVE` and
 * `calla CALL_AVERAGE_MOVE`, at REACT4.ASM:351, :363, :390 and
 * REACT5.ASM:625.
 *
 * THE LEDGER NOTE ON THIS WAS WRONG about the one thing it turned on. It
 * said "this seam only reports them, so nothing downstream currently
 * hears. Wireable once there is a consumer that is not display." The
 * consumer is not display and it already exists: DCSSOUND.ASM:3484 and
 * :3689 are both two instructions and a CREATE --
 *
 *     CALL_NASTY_MOVE
 *         MOVE  *A13(WRESTLERNUM),A9
 *         CREATE SOUND_PID,PROC_NASTY_MOVE
 *     PROC_NASTY_MOVE
 *         SLEEP 10
 *         MOVE  A9,A5
 *         MOVI  NASTY_MOVE,A2
 *         MOVI  500,A0
 *         CALLR ADD_IF_SILENT
 *
 * -- so a grade is a line of ANNOUNCER COMMENTARY, drawn from a speech
 * table exactly as the turnbuckle pair is. Both tables and both caller
 * rows have been in src/generated/announce_tables.c all along, with
 * their own sleep of 10 and percentage of 500.
 *
 * AND THE TWO READ DIFFERENT REGISTERS. CALL_AVERAGE_MOVE takes
 * WRESTLERNUM from a10 and CALL_NASTY_MOVE from a13, which in REACT are
 * the attacker and the victim. That is not cosmetic: both tables are
 * `personal`, so A5 picks which wrestler's own voice lines can come up.
 * A nasty move is remarked on in the VICTIM's voice and an average one
 * in the ATTACKER's.
 *
 * The existence of a third entry point, CALL_ANI_AVERAGE_MOVE, reading
 * a13 for the same table is what makes this readable rather than a
 * guess: the family has one form per register convention, the same way
 * ck_ignore and ck_ignore_a8 do, and the REACT callers pick by which
 * one holds the subject they mean.
 */
static void match_react_move_grade(wm_arcade_actor_t *attacker,
                                   wm_arcade_actor_t *victim,
                                   wm_arcade_move_grade_t grade,
                                   void *user) {
    wm_match_state *m = (wm_match_state *)user;
    const char *name = (grade == WM_R_MOVE_NASTY) ? "CALL_NASTY_MOVE"
                                                  : "CALL_AVERAGE_MOVE";
    const wm_announce_call *call;
    const wm_announce_table *t;
    const wm_arcade_actor_t *speaker;
    wm_announce_ctx ctx;

    if (!m) return;
    call = wm_announce_call_find(name);
    if (!call) return;
    t = wm_announce_table_find(call->table);
    if (!t) return;

    /* a13 for NASTY, a10 for AVERAGE. */
    speaker = (grade == WM_R_MOVE_NASTY) ? victim : attacker;
    if (!speaker) return;

    memset(&ctx, 0, sizeof(ctx));
    ctx.rng = m->anim_rng;
    ctx.wrestler_num = (int)speaker->wrestler_num;
    ctx.anyone_near_death = match_anyone_near_death;
    ctx.user = m;
    ctx.crowd_user = m;
    ctx.crowd_cheer = match_crowd_cheer;
    ctx.crowd_sound = match_crowd_sound;
    ctx.crowd_busy = match_crowd_busy(m);
    /*
     * PROC_x's own `SLEEP 10` is a process delay before the queue, and
     * ADD_IF_SILENT is what it then calls. This port has no process to
     * hold the ten ticks, so the line is offered now; the queue's own
     * silence test is what decides whether it is taken, which is the
     * part that changes what you hear.
     */
    (void)wm_announce_from_table(&m->announcer, t, call->percent,
                                 true, &ctx);
}

/*
 * TAKER.ASM:602 `CREATE FIREWRK_PID,shake_world` and :711's KIL1C.
 *
 * Both seams were declared and neither was filled; kill_shake was not
 * even CALLED, which the seam audit misses because it only reports
 * seams that are called and empty. So the coffin finish did not shake
 * the world, and had it, nothing would have stopped it.
 *
 * The process captures @WORLDTLX/@WORLDTLY once into a8/a9 and every
 * jitter offsets from THOSE, so the shake is about a fixed point. The
 * body is wm_arcade_und_shake_world; what is here is the process:
 * start, three-tick cadence, stop.
 */
static void match_start_shake(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || m->shake_world_on) return;   /* CREATE is idempotent here:
                                              one FIREWRK_PID process */
    m->shake_world_on = true;
    m->shake_world_base_tlx = m->scroll.worldtlx;
    m->shake_world_base_tly = m->scroll.worldtly;
    /* The loop jitters BEFORE its first SLEEPK, so the tick that starts
       it is a jitter tick. */
    m->shake_world_delay = 0;
}

static void match_kill_shake(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || !m->shake_world_on) return;
    m->shake_world_on = false;
    m->shake_world_delay = 0;
    /*
     * KIL1C kills the process and nothing puts the origin back -- unlike
     * SHAKER2, which takes its own outstanding Y_ADJ off WORLDTLY when
     * aborted. The world is simply left wherever the last jitter put it,
     * which is at most two pixels off on each axis, and the scroller
     * that restarts the moment in_finish_move clears takes it from
     * there. Restoring it here would be a correction the source does
     * not make.
     */
}

/*
 * SPECIAL.ASM:3415 react_debris. The decision half is real -- the RNDPER
 * gate, the DEBRIS_MAX cap, the per-wrestler impact sound, the
 * Undertaker's pin bat -- and the pieces are objects a renderer would
 * make. The burst is resolved here and released immediately, because
 * nothing in this port holds the pieces for their lifespan yet.
 */
static void match_react_debris(void *user, wm_arcade_actor_t *victim,
                               int percent, int shape,
                               int32_t xoff, int32_t yoff, int32_t zoff) {
    wm_match_state *m = (wm_match_state *)user;
    wm_debris_burst burst;
    bool taker_pin;
    (void)xoff; (void)yoff; (void)zoff;
    if (!m || !victim) return;
    /* `cmpi und_4_pin2_anim,a1` -- the source tests the victim's own
       ANIBASE, which this port carries as the running program's label. */
    taker_pin = victim->anipc_program &&
        strcmp(victim->anipc_program, "und_4_pin2_anim") == 0;
    if (!wm_debris_react(&m->debris_runtime, m->anim_rng, victim,
                         percent, shape, taker_pin, &burst))
        return;
    m->last_burst = burst;
    if (burst.sound && m->anim_sound)
        m->anim_sound(m->anim_sound_user, (uint16_t)burst.sound);
    m->debris.last_count = burst.pieces;
    m->debris.created += (uint32_t)burst.pieces;
    /* No object system holds the pieces, so the slots come straight back
       rather than being leaked for the whole match. */
    wm_debris_finish(&m->debris_runtime);
}

/* @no_debris, written by create_impact4/flykick and by LEXSEQ2's
   #stop_debris / #restore_debris pair. */
static void match_set_no_debris(void *user, bool off) {
    wm_match_state *m = (wm_match_state *)user;
    if (m) m->debris.no_debris = off;
}

/* DNKSEQ2.ASM:5202 win_announce: start LIFEBAR.ASM's round-ending
   process. Ticked below, beside round_state's own KO countdown. */
static void match_win_announce(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    /* DNKSEQ2.ASM:5212 `CALLA KILL_PIN_HIM` -- the pin stuck, so the
       crowd stops asking for one. */
    if (m->kill_pin_him) m->kill_pin_him(m->sound_proc_user);
    (void)wm_arcade_win_announce(&m->round_announce);
}

/* announce_rnd_winner's own SNDSND calls. The crowd sink already holds
   what the crowd is told; the victory loop is one of its sounds. */
static void match_arw_sound(void *user, int sound, int ticks) {
    match_crowd_sound(user, sound, ticks);
}

/* `move *a10(PLYR_SIDE),a0 / CALLA CALL_MATCH_OVER` */
static void match_arw_match_over(void *user, int winner_side) {
    wm_match_state *m = (wm_match_state *)user;
    wm_announce_ctx ctx;
    const wm_arcade_actor_t *winner;
    if (!m || winner_side < 0) return;
    winner = m->round_announce.winner;
    memset(&ctx, 0, sizeof(ctx));
    ctx.rng = m->anim_rng;
    ctx.wrestler_num = winner ? (int)winner->wrestler_num : -1;
    ctx.anyone_near_death = match_anyone_near_death;
    ctx.user = m;
    ctx.crowd_user = m;
    ctx.crowd_cheer = match_crowd_cheer;
    ctx.crowd_sound = match_crowd_sound;
    ctx.crowd_busy = match_crowd_busy(m);
    /* PROC_MATCH_OVER's `xor a8,a9 / jrz #drn_l`: the losing side was a
       drone when it carries no PSTATUS bit. This port's PSTATUS is one
       bit for the single human, so the loser is a drone unless he is
       that human. */
    /* `win_streak` is WRESTLE.ASM:233's p1winstreak, a per-credit counter
       this port does not keep -- 0 is "no streak", which is what the
       over-four-wins line tests against, not a stand-in for one. */
    (void)wm_announce_match_over_run(
        &m->announcer,
        !(m->has_human && winner_side != 0),
        ctx.wrestler_num,
        0,
        &ctx);
}

static void match_rope_set_z(void *user, int bank, int strand, int action) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || bank < 0 || bank >= WM_MATCH_ROPE_BANKS) return;
    (void)strand;
    m->rope_second_half_z[bank] =
        wm_rope_second_half_z(m->rope_second_half_z[bank],
                              (WmRopeZAction)action);
}

static void place_wrestler(wm_arcade_actor_t *a, int32_t x, int32_t z, int32_t facing) {
    a->x_int = x;
    a->x_fixed = x << 16;
    a->z_int = z;
    a->z_fixed = z << 16;
    a->facing_dir = a->new_facing_dir = facing;
    /*
     * WRESTLE.ASM:2724-2728 creates the wrestler with OBJ_YPOS = 0 and
     * GROUND_Y = MAT_Y, then runs the veladd clamp inline right there
     * (:2730, the source's own ";From veladd" comment): 0 is below the mat,
     * so he is lifted to it with OBJ_YVEL cleared. That settled position is
     * what this sets directly. OBJ_GRAVITY starts at GAME.EQU's GRAVITY,
     * which every change_anim resets it to anyway (ANIM.ASM:4520/:4553).
     */
    a->ground_y = WM_MAT_Y;
    a->y_int = WM_MAT_Y;
    a->y_fixed = (int32_t)WM_MAT_Y << 16;
    a->y_vel = 0;
    a->gravity = WM_GRAVITY;
}

/*
 * WRESTLE.ASM reset_start/#set0 at wrestler_main startup. The row is
 * the number of already-created wrestlers on THIS PLYR_SIDE before
 * this process. Reuse the exact table/rule already translated for
 * between-round reset instead of assigning a row from controller slot.
 */
static void place_created_wrestler(wm_match_state *m, unsigned actor_index) {
    wm_arcade_actor_t *order[WM_MATCH_MAX_ACTORS];
    wm_arcade_actor_t *a;
    const wm_round_start_t *row;
    int start_index;
    int side;
    unsigned i;

    if (!m || actor_index >= WM_MATCH_MAX_ACTORS) return;
    for (i = 0; i <= actor_index; ++i) order[i] = &m->actors[i];
    a = &m->actors[actor_index];
    start_index = wm_round_start_index(a, order, actor_index + 1u);
    if (start_index < 0 || start_index >= WM_ROUND_STARTS_PER_TEAM) return;
    side = a->player_side == WM_MATCH_PSIDE_PLYR1 ? 0 : 1;
    row = &wm_round_team_starts[side][start_index];
    place_wrestler(a, row->x, row->z, row->facing);
}

void wm_match_init(wm_match_state *m) {
    size_t i;
    if (!m) return;
    memset(m, 0, sizeof(*m));
    /* SPECIAL.ASM:1278 init_special_objlist, and the cold init each
       pooled process gets once. A zeroed list is already empty, but the
       source has a routine for it and so does this. */
    wm_arcade_special_lists_init(&m->specials);
    for (i = 0; i < WM_MATCH_MAX_SPECIALS; ++i)
        wm_arcade_special_obj_init(&m->special_pool[i]);
}

unsigned wm_match_draw_wrestler_index(WmRng *rng) {
    uint32_t v = wm_rng_rndrng0(rng, 7);
    if (v == 7) v = 8;
    return v;
}

static void init_actor_life(wm_arcade_actor_t *a) {
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->in_ring = 1;
    a->life = WM_ARCADE_LIFE_MAX;
}

/*
 * PLYR.EQU:263 PLYR_TYPE. WRESTLE2.ASM's #ko_if_drone reads it off the
 * process at the head of xxx_dead_anim -- a dead drone is knocked out
 * for good and a dead human is not -- so every actor needs one. The
 * source sets it at wrestler creation; so does this.
 */
/*
 * WRESTLE.ASM:2385 `calla init_smoves`, which start_match runs once
 * per wrestler per MATCH -- and WRESTLE.ASM:1578's `clr a14 / move
 * a14,@p1pins / move a14,@p2pins / move a14,@finish_completed`, which
 * happens in the same place and is what the finishing move's "second
 * pin attempt" test counts from.
 */
/*
 * LIFEBAR.ASM:3098-3099 and :3126: reset_for_round, reset_for_round2
 * and init_scroller, the block that starts the next round.
 *
 * What the source also does around them and this port does not is
 * presentation and audio: CLEAR_SPEECH_REPEAT, the round-2/round-3
 * music (SNDSND 16 or 17 on current_round), CLOSE_VERT_SCREEN_LINE and
 * the screen wipe, deleting the text plates, the clock's palette, the
 * meters, and INIT_SKIRTS.
 */
/*
 * The five award routines LIFEBAR.ASM's DO_WAIT path calls in a row,
 * in its order. Every one of them is already translated -- this is
 * the call site they never had, which is why a finished match awarded
 * nothing.
 *
 * One of them still needs more than the match has: arm_comeback_award
 * wants every live enemy's health at the moment of arming (AWARD.ASM:
 * 1059 arms it, not this path, and nothing arms it here yet), so it
 * is called with what the match really knows rather than skipped and
 * declines for a reason that is visible.
 *
 * is_it_a_really_quick_win used to be the other one. It reads
 * @match_time, which had no round clock behind it, so every win
 * scored zero and the award could never fire; the clock is real now
 * and it is handed the same number AWARD.ASM:996 computes.
 */
static void match_end_awards(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    bool human[WM_AWARD_PLAYER_COUNT];
    unsigned p;
    int32_t pstatus;

    if (!m) return;
    pstatus = m->pstatus;

    for (p = 0; p < WM_AWARD_PLAYER_COUNT; ++p) {
        /* AWARD.ASM:996's own reading of @match_time -- see
           wm_match_clock_award_score. A win with more than 69 left on
           the clock is "really quick". */
        (void)wm_award_quick_win(&m->awards, p, p,
                                 (uint32_t)wm_match_clock_award_score(
                                     &m->clock));
        wm_award_defeat_human(&m->awards, p, p, (uint32_t)pstatus);
        wm_award_check_comeback(&m->awards, p, p);
        human[p] = (pstatus & (1 << p)) != 0;
    }
    wm_award_accumulate_round(&m->awards, false, human);
}

/* `#end`'s check_for_award_for_winstreak, for each player. */
static void match_end_winstreak_award(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned p;
    if (!m) return;
    for (p = 0; p < WM_AWARD_PLAYER_COUNT; ++p)
        wm_award_check_winstreak(&m->awards, p);
}

/*
 * AWARD.ASM:1121 arm_winstreak_award, plus the copy the port's own split
 * makes necessary.
 *
 * In the source there is ONE win streak per player -- @p1winstreak --
 * and both the match code and the award code read it. This port holds
 * the match's copy in wm_match_streaks_t and the award subsystem's in
 * wm_award_state::win_streak, and nothing ever wrote the second. That
 * mattered: show_bonus_icons resets a player's accumulated icon total
 * whenever his streak is zero, so with the award copy stuck at zero the
 * between-match bonus screen wiped the total every time and showed
 * nothing. The two are written together here, where the source writes
 * its one.
 */
static void match_end_arm_winstreak(void *user, unsigned player,
                                    int32_t streak) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    wm_award_set_win_streak(&m->awards, player, (uint16_t)streak);
    wm_award_arm_winstreak(&m->awards, player, (uint16_t)streak);
}

/* create_end_rnd_awards' `callr adjust_perfects`. */
static void match_end_adjust_perfects(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    wm_award_adjust_perfects_and_blocks(&m->awards);
}

/* ...and its `clr a8 / callr total_icons / movk 1,a8 / callr total_icons`. */
static void match_end_total_icons(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned p;
    if (!m) return;
    for (p = 0; p < WM_AWARD_PLAYER_COUNT; ++p)
        wm_award_total_icons(&m->awards, p);
}

/* increment_wincount's `rst_winstreak_awards` for a streak that just
   went to zero. */
static void match_end_reset_winstreak_rows(void *user, unsigned mask) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned p;
    if (!m) return;
    for (p = 0; p < WM_AWARD_PLAYER_COUNT; ++p)
        if (mask & (1u << p)) wm_award_reset_winstreak(&m->awards, p);
}

/* LIFEBAR.ASM:3302 BONUS_MESS, defined below beside its react wrapper.
   Declared here because the monitors reach it before it is written. */
static void match_bonus_mess(wm_match_state *m, wm_arcade_actor_t *attacker,
                             int bonus);

/*
 * `move a0,*a8(SPECIAL_MOVE_ADDR),L` -- queue an animation on a
 * wrestler for his OWN process to pick up next frame, which
 * WRESTLE.ASM:3843 move_wrestler then dispatches. The source writes it
 * from two places this port reaches: a fired special-move monitor, and
 * #dobuck sending the man who pinned you to his buckoff.
 *
 * What belongs in the field is not the same thing for everyone. Seven
 * wrestlers are label-driven, so the label pointer IS the address the
 * source stores. Bret is not: his backend selects by a typed id and
 * reads this field back as one, so handing him a pointer makes him
 * decode garbage -- and a garbage id looked like a secret move, which
 * latched MODE_UNINT|MODE_NOAUTOFLIP on him permanently and had every
 * later special refused by its own guard. That was found once, in the
 * monitor path. Writing the field in a second place would have been a
 * second chance to get it wrong, so both go through here.
 */
static void match_queue_special_move(wm_arcade_actor_t *a, const char *label) {
    if (!a || !label) return;
    if (a->wrestler_num == WM_ROSTER_BRET) {
        int id = wm_bret_anim_id_for_label(label);
        if (id >= 0) a->special_move_addr = (uintptr_t)id;
        return;
    }
    a->special_move_addr = (uintptr_t)label;
}

/*
 * init_smoves' watchdogs for ONE wrestler, one tick each.
 *
 * The arcade runs every one as its own SMOVE_PID process, and WHERE
 * that process sits is load-bearing. init_smoves creates them with
 * `GETPRC_INSERT`, whose own comment at MPROC.ASM:358 says what that
 * means: "Identical to GETPRC, except that the created process is
 * placed in the process list immediately BEFORE the parent process,
 * not after." The parent is the wrestler, so every one of his monitors
 * runs before he does, every frame.
 *
 * That ordering is the whole point. A free move's last input is an
 * attack button, and the wrestler's own dispatcher would take that
 * same press as an ordinary punch or kick -- which starts an animation
 * whose header sets MODE_UNINT, which is exactly what the monitor's own
 * `WM_SMOVE_G_UNINT` guard then refuses on. This port used to tick the
 * monitors at the END of the frame, after the dispatchers, so the
 * ordinary attack won that race every time and no free move could ever
 * fire: DOWN-AWAY-KICK completed the sequence, reached the guard, and
 * was turned down by the kick it had just triggered. The Undertaker's
 * two spirit moves and Yokozuna's salt -- the three projectiles -- were
 * unreachable for that reason and not for any of their own.
 *
 * So it runs here instead, per wrestler, immediately after his switches
 * are read and before anything of his own moves.
 */
static void match_tick_smoves_for(wm_match_state *m, unsigned ai) {
    size_t si;
    wm_arcade_actor_t *a = &m->actors[ai];
    wm_smove_env_t senv;
    wm_arcade_und_finish_callbacks_t ucb;

    if (!a->active) return;

    memset(&ucb, 0, sizeof(ucb));
    ucb.set_in_finish_move = match_set_in_finish_move;
    ucb.set_world_origin = match_set_world_origin;
    ucb.start_shake = match_start_shake;
    ucb.kill_shake = match_kill_shake;
    ucb.rng = m->anim_rng;
    ucb.user = m;

    memset(&senv, 0, sizeof(senv));
    senv.my_pins = wm_arcade_pins_for(&m->pins, (int)a->player_side);
    senv.victim = a->who_i_hit ? a->who_i_hit
                               : (ai == 0 ? &m->actors[1]
                                          : &m->actors[0]);
    /* PLYR.EQU RING_TIME. Nothing in this port counts a
       wrestler out of the ring yet, so it is derived from the
       INRING flag that IS maintained rather than left at a
       value that would pass the guard by accident. */
    senv.ring_time = a->in_ring ? 1 : -1;
    /*
     * *a8(CLOSEST_NUM) through process_ptrs, which
     * WRESTLE.ASM:4489 get_opp_plyrmode reads. In a two-man
     * match that is the other wrestler; the charge,
     * grab_toss_air and free-move monitors all test his mode
     * and refuse when he is down.
     */
    senv.closest = (ai == 0) ? &m->actors[1] : &m->actors[0];
    senv.pcnt = m->tick_count;
    senv.world_tlx = m->scroll.worldtlx;
    senv.world_tly = m->scroll.worldtly;
    senv.und_cb = &ucb;

    for (si = 0; si < m->smove_count[ai]; ++si) {
        wm_smove_fire_t fire;
        if (!wm_smove_tick(&m->smoves[ai][si], a, &senv, &fire))
            continue;
        if (fire.walk_fast) a->walk_fast = fire.walk_fast;
        if (fire.risk) a->risk = fire.risk;
        /*
         * `movi <n>,a10` and then the process: twenty-three live ones
         * across the eight wrestler files, twenty-two spelled `CREATE
         * MESSAGE_PID,BONUS_MESS` and YOKO.ASM:803 spelled `CREATE0
         * BONUS_MESS`. (A raw grep says twenty-six; three of those are
         * commented out in the source.) This loop computed every one
         * of those numbers and threw it away.
         *
         * BONUS_MESS is not display. LIFEBAR.ASM:3302 sets DAM_MULT to
         * 2 and scores HIGH_RISK_AWD on every path past its first test,
         * so a head-hold special is meant to hit for x1.5 and to score;
         * only the words are gated on the first-time flag. The port
         * carries all twenty-three -- twenty-two generated head-hold
         * rows plus SHAWN.ASM:1606 flipslam -- and all twenty-three
         * landed here and stopped. The seam was wired on the REACT and
         * ANIM.ASM paths, both of which pass -1, so what was reachable
         * was the taunt's high risk and never a secret move's.
         *
         * -1 means the monitor does NOT create the process at all; it
         * is not A10 = -1, which is the taunt path and would hand out
         * an award the source never gives here. So the sign is a gate,
         * not an argument.
         *
         * It runs before the immobilise and the animation because the
         * source's CREATE sits there; what the created process then
         * reads of RISK is the value written just above, which is the
         * order a separate process would have seen anyway.
         */
        if (fire.bonus >= 0) match_bonus_mess(m, a, fire.bonus);
        /* `movk 15,a14 / move a14,*a0(IMMOBILIZE_TIME)` -- the
           move pins the man it is done to. The amount is the
           row's own; eighteen of the forty pin nobody. */
        if (fire.victim && fire.victim_immobilize > 0)
            fire.victim->immobilize_time = fire.victim_immobilize;
        if (fire.anim) {
            /*
             * `move a14,*a8(SPECIAL_MOVE_ADDR),L` -- the monitor queues
             * the animation and the wrestler's own process picks it up,
             * WRESTLE.ASM:3843 move_wrestler.
             *
             * What goes in the field is not the same thing for
             * everyone, and getting that wrong was silent; the reasoning
             * now lives on match_queue_special_move, because #dobuck
             * writes the same field and would have been a second chance
             * to get it wrong. His three monitors were the only ones
             * that could reach this, and they could not reach anything
             * at all until the process-order fix above let them fire.
             */
            match_queue_special_move(a, fire.anim);
            match_change_anim(a, fire.anim, m);
        }
    }
}

/* ------------------------------------------------------------------ *
 * SPECIAL.ASM's projectiles.
 *
 * Three of the five constructors are reachable in the shipped game and
 * two are not: every call site for Doink's pie (DNKSEQ2.ASM:116, :159
 * `ANI_SNOT,doink_pie`) and Bam Bam's fireball (BAMSEQ2.ASM:693
 * `CREATE0 bam_fireball`) is commented out. Their constructors stay
 * translated and tested; nothing here calls them, because nothing in
 * the arcade did either.
 * ------------------------------------------------------------------ */

/*
 * `CREATE0 <routine>`: the source makes a whole process. Here a pool
 * slot is claimed instead, cold-initialised once, and handed to the
 * same constructor. A full pool drops the throw rather than recycling a
 * live projectile -- the source cannot run out, so there is no source
 * behaviour to copy, and silently stealing one in flight would be worse
 * than one that never appears.
 */
static void match_spawn_special(void *user, wm_arcade_actor_t *owner,
                                int kind) {
    wm_match_state *m = (wm_match_state *)user;
    size_t i;

    if (!m || !owner) return;
    for (i = 0; i < WM_MATCH_MAX_SPECIALS; ++i) {
        wm_arcade_special_obj_t *obj = &m->special_pool[i];
        if (obj->in_list) continue;
        wm_arcade_special_obj_init(obj);
        m->specials_spawned++;
        m->specials_born_this_tick |= 1u << i;
        switch ((wm_arcade_special_kind_t)kind) {
        case WM_SP_KIND_YOKO_SALT:
            wm_arcade_spawn_yoko_salt(&m->specials, obj, owner);
            break;
        case WM_SP_KIND_TAKER_SPIRIT:
            wm_arcade_spawn_taker_spirit(&m->specials, obj, owner);
            break;
        case WM_SP_KIND_TAKER_REAPER:
            wm_arcade_spawn_taker_reaper(&m->specials, obj, owner);
            break;
        /* Cut from the shipped game -- see above. Reached only if
           something starts calling a commented-out spawn. */
        case WM_SP_KIND_DOINK_PIE:
        case WM_SP_KIND_BAM_FIREBALL:
        default:
            break;
        }
        return;
    }
}

/*
 * One turn of each projectile's own process loop (SPECIAL.ASM:1745 for
 * the salt, :1610 for the spirit and reaper -- the same shape both
 * times):
 *
 *     #lp  callr sp_velocity_add
 *          ...write the object's draw position...
 *          SLEEPK 1
 *          callr sp_animate
 *          move *a13(SP_DIE),a0 / jrnz #die
 *          move @WORLDTLX+16,a0 / addi 200,a0
 *          move *a13(SP_OBJ_XPOSINT),a1 / sub a1,a0 / abs a0
 *          cmpi 256,a0 / jrlt #lp          ;"off screen by 56 pixels"
 *     #die delete_special_objlist / DELOBJ / DIE
 *
 * One difference between the two is real and is kept: the salt calls
 * delete_special_objlist inside #die, so both exits remove it from the
 * collision list, while the spirit and reaper call it just ABOVE the
 * #die label -- so the SP_DIE exit jumps past it. That is safe rather
 * than a leak, because SP_DIE is only ever set by special_hit and
 * wrestler_hit_special, which have already removed the object
 * themselves. Reproduced as written.
 */
/*
 * COLLIS.ASM:733 object_collisions, called from `#loop calla
 * check_collisions` -- the FIRST thing WRESTLE.ASM's main loop does
 * (:2062). The main-loop process sits ahead of every wrestler in the
 * process list, because GETPRC links a child AFTER its parent, so this
 * sweep sees the projectiles as they were at the end of last frame. A
 * projectile a wrestler's animation CREATE0s later this frame is not
 * swept until the next one, and that is why the two halves are separate
 * functions here rather than one.
 */
static void match_sweep_specials(wm_match_state *m,
                                 wm_arcade_actor_t **actor_ptrs) {
    wm_arcade_special_callbacks_t cb;

    size_t i;
    wm_arcade_special_obj_t *held[WM_MATCH_MAX_SPECIALS];
    size_t held_n = 0;

    if (!m) return;

    /* Anything built by this tick's animations is lifted off the lists
       for the sweep and put straight back -- the arcade's own one-frame
       boundary, expressed against a pool instead of a process list. */
    for (i = 0; i < WM_MATCH_MAX_SPECIALS; ++i) {
        if (!(m->specials_born_this_tick & (1u << i))) continue;
        if (!m->special_pool[i].in_list) continue;
        held[held_n++] = &m->special_pool[i];
        wm_arcade_special_delete(&m->specials, &m->special_pool[i]);
    }

    wm_arcade_special_set_all_boxes(&m->specials);
    memset(&cb, 0, sizeof cb);
    cb.react1 = &m->react1_ctx;
    (void)wm_arcade_object_collisions(&m->specials, actor_ptrs,
                                      m->actor_count,
                                      &m->combat_runtime, &cb);

    for (i = 0; i < held_n; ++i)
        wm_arcade_special_insert(&m->specials, held[i]);
    m->specials_born_this_tick = 0u;
}

static void match_step_specials(wm_match_state *m) {
    size_t i;

    if (!m) return;

    for (i = 0; i < WM_MATCH_MAX_SPECIALS; ++i) {
        wm_arcade_special_obj_t *obj = &m->special_pool[i];
        int32_t centre, dx;

        if (!obj->in_list) continue;

        wm_arcade_special_velocity_add(obj);
        wm_arcade_special_tick_source_state(obj);   /* sp_animate */

        if (obj->die != 0) {
            /* The spirit/reaper exit that skips the delete; doing it
               here is a no-op for an object already off the list. */
            wm_arcade_special_delete(&m->specials, obj);
            continue;
        }

        /* `@WORLDTLX+16` is the world X's integer half. */
        centre = (m->scroll.worldtlx >> 16) + 200;
        dx = centre - (obj->x_fixed >> 16);
        if (dx < 0) dx = -dx;
        if (dx >= 256)
            wm_arcade_special_delete(&m->specials, obj);
    }
}

/*
 * LIFEBAR.ASM:2810-2825, the two MATCH awards -- the block between
 * set_winner and CALL_MATCH_OVER, which is upstream of DO_WAIT and had
 * no translation at all. Nothing in this port ever called match_award,
 * so no match-level award existed: TWO_RND_AWD, PERFECT_AWD and (for
 * want of arming, above) FIVE_WINS_AWD were all unreachable, and with
 * them the whole match[] half of the award state stayed whatever the
 * per-round accumulation had left in it.
 *
 * `a10` is what set_winner found: "PLYRNUM of a wrestler on the winning
 * team. The pinner if there is one." match_award then reads his own
 * PLYRNUM and refuses anybody from slot 2 up, so only a human scores
 * one; PLYR_SIDE picks the row. This takes the winning SIDE from the
 * score, which is the same answer for the case the port can reach --
 * one human a side -- and says so rather than pretending to have the
 * pinner's process.
 */
static void match_grant_match_awards(wm_match_state *m,
                                     wm_arcade_actor_t *const *actors) {
    int32_t side;
    unsigned p;

    if (!m || m->score.match_winner <= 0) return;
    side = m->score.match_winner - 1;     /* 1-or-2 -> PLYR_SIDE */

    /* match_award's own `cmpi 2,a9 / jrge #ma_out`: a drone cannot be
       awarded anything, so the winning side only scores if a human is
       playing on it. PSTATUS is one bit per human player and the port's
       player index doubles as PLYR_SIDE for the humans. */
    p = (unsigned)side;
    if (p >= WM_AWARD_PLAYER_COUNT) return;
    if ((m->pstatus & (1 << p)) == 0) return;

    if (wm_match_two_round_victory(&m->score, m->eight_on_one))
        wm_award_match_award(&m->awards, p, WM_AWARD_TWO_ROUND);

    /*
     * `callr is_perfect / jrnc #not_perfect`. The CREATE_PERFECT
     * graphic and its SLEEP 55+50 are presentation and are not here.
     *
     * This is wm/arcade/wm_arcade_lifebar.h's own translation of
     * LIFEBAR.ASM:3650, not a second one. A duplicate did briefly live
     * in wm_arcade_match_end.c: the routine was already translated and
     * tested and had never been called, so writing the award path found
     * a gap that was really a missing caller. One routine, one
     * translation.
     */
    {
        const wm_arcade_actor_t *winner = NULL;
        size_t wi;
        for (wi = 0; wi < m->actor_count; ++wi) {
            if (actors[wi] && actors[wi]->active &&
                actors[wi]->player_side == side) {
                winner = actors[wi];
                break;
            }
        }
        if (winner &&
            wm_arcade_is_perfect(winner,
                                 (const wm_arcade_actor_t *const *)actors,
                                 m->actor_count, m->eight_on_one))
            wm_award_match_award(&m->awards, p, WM_AWARD_PERFECT);
    }
}

/* The two sounds DO_WAIT makes itself, through triple_sound. */
static void match_end_sound(void *user, int sound) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || !m->anim_sound) return;
    m->anim_sound(m->anim_sound_user, (uint16_t)sound);
}


/*
 * Who counts as "the opponent" for one wrestler.
 *
 * WRESTLE.ASM's own answer is CLOSEST_NUM, which calc_closest fills
 * by searching every other wrestler process on the OTHER PLYR_SIDE
 * and keeping the nearest in X. Until buddy mode there were only
 * ever two actors, so this port substituted `the other slot` and
 * said so; with four in the ring that substitution stops being
 * true -- slot 2 is player one's partner and must not be treated as
 * his opponent.
 *
 * This is the search, restricted to what the port's actor records
 * carry: nearest ACTIVE actor whose player_side differs, by absolute
 * X. For a two-actor match it returns exactly the slot `1u - i` did,
 * so nothing about the existing paths changes.
 *
 * Returns NULL if the side is empty, which a caller must handle --
 * the source cannot reach that state and neither should this, but a
 * null is better than slot zero pretending to be an enemy.
 */
static wm_arcade_actor_t *match_opponent_of(wm_match_state *m, unsigned i) {
    wm_arcade_actor_t *best = NULL;
    int32_t best_dx = 0;
    unsigned k;

    if (!m || i >= m->actor_count) return NULL;
    for (k = 0; k < m->actor_count; ++k) {
        int32_t dx;
        if (k == i) continue;
        if (!m->actors[k].active) continue;
        if (m->actors[k].player_side == m->actors[i].player_side) continue;
        dx = m->actors[k].x_int - m->actors[i].x_int;
        if (dx < 0) dx = -dx;
        if (!best || dx < best_dx) {
            best = &m->actors[k];
            best_dx = dx;
        }
    }
    return best;
}

static void match_reset_for_round(wm_match_state *m) {
    wm_arcade_actor_t *actors[WM_MATCH_MAX_ACTORS];
    unsigned i;

    if (!m) return;
    /* LIFEBAR.ASM:3136 `CREATE SOUND_PID,ring_bell`, in reset_for_round
       itself -- every round after the first opens with the bell too. */
    if (m->start_bell) m->start_bell(m->sound_proc_user);
    for (i = 0; i < WM_MATCH_MAX_ACTORS; ++i) actors[i] = &m->actors[i];

    /* reset_for_round's own loop over every wrestler. */
    for (i = 0; i < m->actor_count; ++i) {
        if (!m->actors[i].active) continue;
        wm_round_reset_wrestler(&m->actors[i], actors, m->actor_count,
                                m->tick_count);
        /* `calla init_rnd_life_data` -- LIFEBAR.ASM:238, everyone back
           to full for round two and later. */
        m->actors[i].life = WM_ARCADE_LIFE_MAX;
    }

    m->current_round += 1;
    /* WRESTLE.ASM:2657 `;reset match_time` -- the same three stores
       match_timer opens with, so round two starts on a full clock. */
    wm_match_clock_reset(&m->clock);
    m->clock_warning = false;
    /* `callr reset_smoves` -- every watchdog back to its own entry. */
    for (i = 0; i < m->actor_count; ++i)
        wm_smove_reset(m->smoves[i], m->smove_count[i]);
    /* `clr a0 / move a0,@any_hits`. */
    m->combat_runtime.any_hits = 0;

    /* reset_for_round2: `clr a14 / move a14,@annc_rnd_winner_done`,
       then the per-wrestler pass. */
    m->round_announce.done = false;
    for (i = 0; i < m->actor_count; ++i) {
        if (!m->actors[i].active) continue;
        wm_round_reset_wrestler2(&m->actors[i]);
    }

    /* `calla init_scroller` -- put the camera back too. */
    wm_round_init_scroller(&m->scroll.worldtlx, &m->scroll.worldtly, 1);

    /* The round state itself, so the next KO can decide again. */
    wm_arcade_round_state_init(&m->round_state);
}

/*
 * WRESTLE2.ASM:3772 change_wrestler's other half -- the parts that need
 * the match's own tables, run right after wm_final_change_wrestler has
 * done the wrestler's own fields.
 *
 * `callr init_smoves` here is per-wrestler in the source (it is the
 * SUBRP with a13 set, not the whole-roster sweep start_match runs), so
 * only this slot's watchdogs are rebuilt. `calla init_wres_life_data`
 * is LIFEBAR.ASM:199, "one wrestler, on a swap": his CLIFE snaps to
 * full, which is what makes the next man in the queue arrive healthy.
 *
 * NOT done here and not pretended: choose_pal/pal_getf and the sweep
 * that writes the new palette over every OBJ_BASE piece, and
 * change_anim1a/change_anim2a onto the new wrestler's stand4/torso4
 * pair. Both are the display's, and this port's per-wrestler animation
 * coverage is what decides whether the second one can mean anything.
 */
static void match_finish_zombie_transform(wm_match_state *m, unsigned i) {
    if (!m || i >= m->actor_count) return;

    wm_final_change_wrestler(&m->actors[i], m->scroll.worldtlx);

    m->smove_count[i] = wm_smove_init(
        m->actors[i].wrestler_num,
        m->actors[i].plyr_type == WM_PTYPE_DRONE,
        m->smoves[i], WM_SMOVE_MAX_PER_WRESTLER,
        &m->smove_unported[i]);

    /* `calla init_wres_life_data`, LIFEBAR.ASM:199 -- CLIFE snaps to
       full for this one wrestler, which is how the next man in the
       queue arrives healthy. This port keeps life on the actor, the
       same place init_rnd_life_data's sweep writes it. */
    m->actors[i].life = WM_ARCADE_LIFE_MAX;
}

/*
 * WRESTLE.ASM:3886 auto_pin_check, at its own place in the loop.
 *
 * "if all opponents are dead, wait four seconds, then wait for the unint
 * bit to clear, then turn into a drone." Its one caller is move_wrestler
 * (WRESTLE.ASM:3858), one line before the per-character dispatch, every
 * tick, for every wrestler -- which is where this sits.
 *
 * WHY IT IS CALLED FROM HERE AND NOT FROM THE TRANSLATED move_wrestler.
 * There are two translations of that routine. wm_arcade_move_wrestler
 * (wm/arcade/wm_arcade_move_dispatch.h) is the faithful one -- the @HALT
 * check, the SPECIAL_MOVE_ADDR drain, auto_pin_check, then the
 * per-character dispatch -- and it is called only from a test.
 * wm_arcade_move_ported_wrestler is that last step alone, and it is what
 * this loop calls.
 *
 * Routing this loop through the faithful one was tried and backed out,
 * because the two cannot simply be collapsed. The drain is not missing:
 * BRET.ASM's half of it lives INSIDE his own dispatcher
 * (src/core/arcade/wm_arcade_bret.c:375), keyed on a typed
 * wm_arcade_bret_anim_id_t, and his own monitors are what write it. A
 * shared drain running first would take his queued id, cast it as a
 * label pointer, and skip his dispatcher on the tick that was supposed
 * to consume it. Reconciling that means deciding whether
 * SPECIAL_MOVE_ADDR carries an id or an address for everybody, which is
 * its own change; until then, the honest thing is to put auto_pin_check
 * where the source calls it and say why the rest is still split.
 *
 * (For the other seven the field is vestigial rather than latched: the
 * monitor writes it to mirror the source's queue and no dispatcher
 * reads it, because match_tick_smoves_for starts the animation itself.)
 */
static void match_auto_pin_check(wm_match_state *m, unsigned i,
                                 wm_arcade_actor_t *opp,
                                 wm_arcade_actor_t *const *actor_ptrs) {
    wm_auto_pin_env_t ape;

    memset(&ape, 0, sizeof ape);
    /* The same two globals the mode_dead env reads, read the same way. */
    ape.in_finish_move = m->in_finish_move;
    ape.finish_completed = m->coffin.finish_completed != 0;
    ape.royal_rumble = m->royal_rumble;
    ape.opponent = opp;
    ape.actors = actor_ptrs;
    ape.actor_count = m->actor_count;

    /* Becoming a drone is the whole effect and it lands on the actor's
       own PLYR_TYPE; the drone AI picks him up from there. */
    (void)wm_arcade_auto_pin_check(&m->actors[i], &ape);
}

/*
 * confine_wrestler with everything it needs, plus the three things it
 * decides but cannot carry out.
 *
 * The climb checks live INSIDE confine_wrestler in the source
 * (WRESTLE.ASM:3103, :3121, :3235 for the way out and :3560, :3583,
 * :3604 for the way back in), and climbing out is the only thing in the
 * game that clears INRING. They were translated a long time ago in
 * wm/arcade/wmania_ring_climb.h and had no caller at all, so no wrestler
 * ever left the ring and the whole out-of-ring half of confine_wrestler
 * was dead by construction.
 */
static void match_confine_actor(wm_match_state *m, unsigned i,
                                wm_arcade_actor_t *const *actor_ptrs) {
    wm_confine_result_t res;
    wm_arcade_actor_t *a = &m->actors[i];

    wm_arcade_confine_wrestler_ex(a, actor_ptrs, m->actor_count,
                                  m->tick_count, &res);

    /* `calla change_anim1a` on whichever climbthru/climbin animation the
       check chose. NEW_FACING_DIR came with it. */
    if (res.climb_anim) {
        a->new_facing_dir = (int32_t)res.climb_facing;
        match_change_anim(a, res.climb_anim, m);
    }

    /*
     * The other half of the same decision, and until now the half that
     * went nowhere: he reached the rope facing the wrong way, so the
     * source turns him first and defers the climb.
     *
     *      move    a2,*a13(NEW_FACING_DIR)
     *      calla   set_rotate_anim
     *      calla   change_anim1a
     *      movi    #climb,a0
     *      move    a0,*a13(CODE_ADDR),L
     *      SETMODE WAITANIM
     *
     * (WRESTLE2.ASM:196-201, :574-579 and :698-703.) All four lines
     * matter. set_rotate_anim picks the turn animation AND copies
     * NEW_FACING_DIR into FACING_DIR, so he is already facing the right
     * way while the turn plays; change_anim1a starts it; CODE_ADDR
     * remembers where to resume; and WM_PMODE_WAITANIM -- which
     * wm_arcade_confine_wrestler_ex has already written -- is what makes
     * each wrestler's own mode_waitanim pick the continuation up when
     * that animation ends.
     *
     * Dropping the continuation was not a missing flourish. Two of the
     * three checks also set CLIMBING_THRU on this path, and only the
     * climbthru animation's own ANI_INRING/ANI_NOTINRING clears it
     * (ANIM.ASM:405, :4471), so a wrestler who walked into the side
     * ropes facing away came out of the check flagged as climbing
     * through with nothing running to ever unflag him -- and
     * ck_climb_in_side's leading `if (climbing_thru) return` then
     * refused him the ropes for the rest of the match.
     */
    if (res.climb_rotate_then != WM_RING_CLIMB_CONT_NONE) {
        const char *turn;
        a->new_facing_dir = (int32_t)res.climb_facing;
        turn = wm_wrestler_set_rotate_anim(a, (int)a->wrestler_num,
                                           a->facing_dir);
        if (turn) match_change_anim(a, turn, m);
        a->code_addr = (uintptr_t)res.climb_rotate_then;
    }

    /*
     * WRESTLE.ASM:3729 `movk 1,a0 / calla change_wrestler` -- a zombie
     * who reaches the arena edge transforms there rather than waiting
     * out mode_dead's ten seconds. (The `movk 1,a0` is dead:
     * change_wrestler never reads a0.)
     */
    if (res.zombie_transform) match_finish_zombie_transform(m, i);

    /*
     * The gate crash's own animation, WRESTLE.ASM:3695. The routine has
     * already applied the velocities, MODE_NORMAL, RUN_TIME and the
     * damage; only the animation choice comes back here, because it is a
     * per-wrestler table dispatch and the routine has no roster.
     *
     * It was a choice that came back and stopped. Everything else about
     * a gate crash happened and the wrestler slid into the fence still
     * playing his run -- the third instance of this shape found in one
     * sweep, with the buckoff's two above.
     *
     * The two tables are not the same shape, which is the whole reason
     * the source uses two macros. `FACETBL fall_back_tbl` (REACT1.ASM:1801)
     * is one long per wrestler. `FACE24TBL bncoff_gate` (REACT5.ASM:353)
     * is two, and FACE24TBL takes column 0 when MOVE_UP_BIT is SET --
     * wm_roster_anim_facing applies that rule and ignores it for the
     * one-column table, so both go through the same call.
     *
     * Which one is the three-second rule, and its sense is worth
     * keeping straight: `cmpi TSEC*3,a14 / jrge #bnc` means THREE
     * SECONDS OR MORE since the last hit bounces him off, and anything
     * sooner falls him back. So gate_fall_back true is the recent-hit
     * case.
     */
    if (res.gate_crash) {
        const wm_roster_anim_table *tbl = wm_roster_anim_find(
                res.gate_fall_back ? "fall_back_tbl" : "bncoff_gate");
        const char *label = tbl ? wm_roster_anim_facing(
                tbl, (int)a->wrestler_num,
                (a->facing_dir & (int32_t)WM_MOVE_UP) != 0) : NULL;
        /* `calla change_anim1a` -- from the top, because he is already
           playing his run animation and that is what is being replaced. */
        if (label) match_change_anim(a, label, m);
        /* `movi 0c5h,a0 / calla triple_sound`. */
        if (m->anim_sound) m->anim_sound(m->anim_sound_user, 0xc5u);
    }
}

/*
 * DOINK.ASM mode_dead's five globals and its process_ptrs sweep, handed
 * to whichever backend is about to run the wrestler's MODE_DEAD case.
 */
static wm_mode_dead_env_t match_mode_dead_env(wm_match_state *m,
                                              wm_arcade_actor_t *const *ptrs) {
    wm_mode_dead_env_t e;
    memset(&e, 0, sizeof e);
    if (!m) return e;
    /* @p1rounds / @p2rounds. */
    e.rounds[0] = m->score.p1rounds;
    e.rounds[1] = m->score.p2rounds;
    e.eight_on_one = m->eight_on_one;
    e.royal_rumble = m->royal_rumble;
    e.instant_combos_on = m->instant_combos_on;
    e.in_finish_move = m->in_finish_move;
    e.finish_completed = m->coffin.finish_completed != 0;
    e.actors = ptrs;
    e.actor_count = m->actor_count;
    return e;
}

/*
 * What #dobuck decided but could not do: two FACETBL animation
 * dispatches and a display message.
 */
static void match_apply_mode_dead(wm_match_state *m, unsigned i,
                                  const wm_mode_dead_result_t *r) {
    if (!m || !r || !r->bucked_off) return;
    if (i >= m->actor_count) return;

    /*
     * `FACETBL hitonground_tbl / calla change_anim1a` -- the convulse he
     * comes back to life on.
     *
     * BOTH OF THESE WERE REFUSED, AND BOTH REFUSALS HAD GONE STALE. This
     * one read "this port has no hitonground table for every wrestler",
     * and the other "only some of those animations are extracted". The
     * registry now carries hitonground_tbl (REACT1.ASM:1856, all ten
     * rows including the two `.long 0`) and #buckoff_tbl (DOINK.ASM:3235,
     * nine), and every one of the eight `*_hitonground_anim` and eight
     * `*_buckoff_anim` programs is generated. Nothing has to be invented
     * to play either. So a wrestler who mashed his way out of a pin
     * convulsed in the arcade and stood still here, and the man who had
     * pinned him was left in his pinning animation.
     *
     * FACETBL is the one-long-per-wrestler form (MACROS.H:102), so the
     * facing plays no part despite the macro's name -- that is
     * FACE24TBL. A NULL is slot 7 or 9, the cut Adam Bomb and the
     * Referee, and the source's own `.long 0` for them: the caller
     * copes rather than substituting somebody else's animation.
     *
     * `change_anim1a`, not `change_anim1`: he is very likely already
     * playing his death animation, and the convulse has to restart from
     * frame 0 rather than being refused as already running.
     * match_change_anim is the unguarded form.
     */
    if (r->convulse) {
        const wm_roster_anim_table *tbl = wm_roster_anim_find("hitonground_tbl");
        const char *label = tbl ? wm_roster_anim_for(
                tbl, (int)m->actors[i].wrestler_num) : NULL;
        if (label) match_change_anim(&m->actors[i], label, m);
    }

    /*
     * `move a0,a8 / FACETBL #buckoff_tbl,a8 / move a0,*a8(SPECIAL_MOVE_ADDR),L`
     * onto the man who pinned him -- queued on his process rather than
     * started here, which is what the source does and what the field is
     * for. The source's own comment is unsure about it ("stick it into
     * special_move_addr?"); the instruction is not.
     *
     * wm_arcade_mode_dead already applies the guard above it: a pinner
     * carrying DID_RAISEARM is not sent here at all, because he is no
     * longer on top. So pinner_to_buck being NULL is a decision, not a
     * gap.
     */
    if (r->pinner_to_buck) {
        const wm_roster_anim_table *tbl = wm_roster_anim_find("#buckoff_tbl");
        const char *label = tbl ? wm_roster_anim_for(
                tbl, (int)r->pinner_to_buck->wrestler_num) : NULL;
        if (label) match_queue_special_move(r->pinner_to_buck, label);
    }

    /*
     * `calla init_reduce_bog`, "because match_timer clears it when it
     * sees one team dead" -- and a bucked-off wrestler means it was
     * wrong about that.
     */
    m->debris.no_debris = false;
    m->debris.reduce_bog = (int32_t)m->actor_count - 2;
}

static void init_smoves(wm_match_state *m) {
    unsigned i;
    if (!m) return;
    wm_arcade_pins_clear(&m->pins);
    wm_coffin_reset(&m->coffin);
    m->in_finish_move = false;
    /* actor_count, not the cap: buddy mode raised the cap to four and
       a two-wrestler match must not arm watchdogs for slots it never
       created. */
    for (i = 0; i < m->actor_count; ++i) {
        m->smove_count[i] = wm_smove_init(
            m->actors[i].wrestler_num,
            m->actors[i].plyr_type == WM_PTYPE_DRONE,
            m->smoves[i], WM_SMOVE_MAX_PER_WRESTLER,
            &m->smove_unported[i]);
    }
}

/*
 * `movi PTYPE_PLAYER,a8 / btst n,a0 / jrnz #ok / movi PTYPE_DRONE,a8`
 * -- each created wrestler is a player or a drone by its own PSTATUS
 * bit, which is what actor_is_human records. The buddies #2plyr adds
 * are PTYPE_DRONE unconditionally and are simply not flagged human.
 */
static void init_plyr_types(wm_match_state *m) {
    unsigned i;
    for (i = 0; i < m->actor_count; ++i) {
        m->actors[i].plyr_type =
            m->actor_is_human[i] ? WM_PTYPE_PLAYER : WM_PTYPE_DRONE;
    }
}

static void init_bret_backends(wm_match_state *m) {
    unsigned i;
    for (i = 0; i < m->actor_count; ++i) {
        wm_bret_backend_init(&m->bret_visual[i]);
        /* LIFEBAR.ASM adjust_health's "attract mode never dies" rule
           (PSTATUS==0) -- see wm_arcade_adjust_health. Fixed for the whole
           match: neither start path changes has_human afterward. */
        m->bret_visual[i].attract_mode = !m->has_human;
        m->bret_visual[i].instant_combos_on = m->instant_combos_on;
        if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
            wm_arcade_bret_callbacks_t cb = wm_bret_backend_callbacks(&m->bret_visual[i]);
            wm_arcade_bret_ani_init(&m->actors[i], &cb);
        }
    }
}

/*
 * Everything start_match does once the wrestlers exist, shared by
 * all three creation paths (#0plyr, #1plyr and #2plyr) because the
 * source shares it too -- the three paths converge on
 * #wrestlers_created and run the same setup from there.
 */
static void match_start_common_tail(wm_match_state *m) {
    wm_arcade_combat_runtime_init(&m->combat_runtime);
    {
        /* ROPES.ASM creates one process per bank. reduce_bog kills only the
           front/back pair after object creation, which is the source's own
           argument to this call. */
        unsigned b;
        for (b = 0; b < WM_MATCH_ROPE_BANKS; ++b)
            wm_rope_runtime_init_bank(&m->ropes[b], (WmRopeBank)b, false);
    }
    wm_announcer_init(&m->announcer);       /* RESET_VOICE_QUEUE */
    /* WRESTLE.ASM:1621, in start_match: `clr a0 / move
       a0,@message_flag,L` -- and a LONG is half the field, which
       wm_bonus_mess_reset_match preserves. */
    wm_bonus_mess_reset_match(&m->bonus_mess);
    wm_anim_code_reset();
    wm_arcade_round_state_init(&m->round_state);
    /*
     * WRESTLE.ASM:1631 `CREATE TIMER_PID,match_timer` -- the round
     * clock starts with the match, not with the round. Its rate is
     * chosen once here and survives every reset.
     *
     * All three of match_timer's slowdowns are false, for the same
     * reason royal_rumble is hard-coded false everywhere else in this
     * file: this port runs a fixed two-man bout with no ladder state
     * to ask. wm_match_clock_rate takes them so a caller that has a
     * wm_pregame_state can pass the real answers without the clock
     * itself having to learn about ladders.
     */
    wm_match_clock_start(&m->clock,
                         wm_match_clock_rate(WM_MATCH_CLOCK_ADJSPEED_DEFAULT,
                                             m->royal_rumble, false, false));
    m->clock_warning = false;
    wm_arcade_round_announce_init(&m->round_announce);
    wm_match_end_init(&m->match_end);
    m->match_over = 0;
    wm_award_init(&m->awards);
    /* WRESTLE.ASM:4552 init_reduce_bog, with this match's two actors. */
    m->debris.no_debris = false;
    m->debris.reduce_bog = (int32_t)m->actor_count - 2;
    wm_shake_init(&m->shake);
    wm_debris_init(&m->debris_runtime);
    memset(&m->last_burst, 0, sizeof(m->last_burst));
    m->allow_offscrn = 0;
    memset(&m->dizzy, 0, sizeof(m->dizzy));
    wm_move_name_init(&m->move_names);
    memset(&m->move_name, 0, sizeof(m->move_name));
    wm_arcade_match_score_init(&m->score);
    /*
     * LIFEBAR.ASM:5131 -- a round in an eight-on-one or a rumble counts
     * DOUBLE, so one of them decides the match. It has to be applied
     * here, after the score reset and after the creation path has
     * settled royal_rumble, rather than in wm_match_bind_final_battle:
     * the bind runs at start_match's own position, which is before all
     * three creation branches.
     */
    m->score.double_rounds = m->eight_on_one || m->royal_rumble;

    /*
     * `CREATE GETUP_PID,getup_meter` -- one per wrestler, right after
     * his wrestler_main, at every one of WRESTLE.ASM's six match-start
     * branches. Started here rather than in each of this port's three
     * creation paths for the same reason everything else in this
     * function is: the paths differ in who they make, not in what is
     * made for them.
     *
     * It comes AFTER the round setup above, which is the source's order
     * too -- reset_for_round stamps DELAY_METER with its fourteen
     * seconds and then slide_offscr's eighteen replace them. That
     * eighteen is the point of wiring this at all: nothing was writing
     * it, and DELAY_METER set is what makes a knocked-down wrestler get
     * straight back up instead of having to mash (wm_arcade_roll.c's
     * `jrz #reg` arm wipes GETUP_TIME outright).
     */
    {
        unsigned gi;
        for (gi = 0; gi < m->actor_count; ++gi) {
            wm_arcade_actor_t *ptrs[WM_MATCH_MAX_ACTORS];
            unsigned k;
            for (k = 0; k < m->actor_count; ++k) ptrs[k] = &m->actors[k];
            (void)wm_arcade_getup_meter_start(
                &m->getup_meter[gi], &m->actors[gi], ptrs, m->actor_count,
                m->drone_meters_on != 0, m->num_opps,
                m->royal_rumble);
        }
    }
}


void wm_match_set_input(wm_match_state *m, unsigned player,
                        const wm_input_state *in) {
    if (!m || player >= 2u) return;
    if (!in) {
        m->player_input_set[player] = false;
        return;
    }
    m->player_input[player] = *in;
    m->player_input_set[player] = true;
}

void wm_match_bind_final_battle(wm_match_state *m,
                                wm_final_battle_state_t *fb,
                                bool is_final_match,
                                bool eight_on_one) {
    if (!m) return;
    m->final_battle = fb;
    m->eight_on_one = eight_on_one;
    /* `jrnc #do_zf` -- nothing happens at all when this is not the
       final match, not even a clear. */
    if (fb && is_final_match)
        wm_final_reset_ptr(fb);
}

/*
 * LIFEBAR.ASM:2511 `CREATE SOUND_PID,ring_bell` -- the match opens with
 * the bell, and reset_for_round rings it again at :3136 for rounds two
 * and three. Both had no caller: the routine is a SOUND_PID process
 * with its own channel bookkeeping, so it lives on whoever owns the
 * mixer and the match only says when.
 */
static void match_ring_the_bell(wm_match_state *m) {
    if (m && m->start_bell) m->start_bell(m->sound_proc_user);
}

void wm_match_start_attract(wm_match_state *m, WmRng *rng) {
    wm_arcade_actor_t *p1, *opp;
    if (!m) return;

    wm_match_init_scroller(m);

    /* `#0plyr` is reached on `move @PSTATUS,a0 / jrz #0plyr`: nobody
       is playing, so no actor is a human. */
    m->pstatus = 0;
    m->royal_rumble = false;
    memset(m->actor_is_human, 0, sizeof m->actor_is_human);
    m->has_human = false;

    m->index1 = wm_match_draw_wrestler_index(rng);
    /* Placeholder opponent draw -- see wm/match.h. Not @index2. */
    m->opponent_wrestler = wm_match_draw_wrestler_index(rng);

    p1 = &m->actors[0];
    opp = &m->actors[1];
    init_actor_life(p1);
    init_actor_life(opp);

    /* WRESTLE.ASM:1775-1782 #0plyr: P1 drone, PLYRNUM=2, PSIDE_PLYR1. */
    p1->player_num = 2;
    p1->player_side = WM_MATCH_PSIDE_PLYR1;
    p1->wrestler_num = (int32_t)m->index1;

    /* WRESTLE.ASM:1786-1794: opponent drone(s), PLYRNUM starts at 3,
       PSIDE_PLYR2. The source loop repeats this NUM_OPPS times from the
       ladder table; only one opponent is created here (see wm/match.h). */
    opp->player_num = 3;
    opp->player_side = WM_MATCH_PSIDE_PLYR2;
    opp->wrestler_num = (int32_t)m->opponent_wrestler;

    p1->smart_target = opp;
    opp->smart_target = p1;

    place_created_wrestler(m, 0);
    place_created_wrestler(m, 1);

    wm_arcade_drone_init(&m->drones[0], 0);
    wm_arcade_drone_init(&m->drones[1], 0);

    /* #0plyr and #1plyr each create two. Set BEFORE the init
       helpers below: every one of them loops over actor_count now
       that buddy mode has raised the cap past it. */
    m->actor_count = 2;
    m->num_opps = 1;

    init_plyr_types(m);
    /* After init_plyr_types: std_taunt's first instruction is
       `move *a8(PLYR_TYPE),a14 / janz SUCIDE`, so who is a drone has
       to be settled before the watchdogs are made. */
    init_smoves(m);
    init_bret_backends(m);
    m->active = true;
    m->tick_count = 0;
    match_start_common_tail(m);
    match_ring_the_bell(m);
}


/* =================================================================
 * WRESTLE.ASM:1801 #2plyr -- the two-player creation path.
 *
 * The third of start_match's three, and the one this port did not
 * have. It differs from #1plyr in four ways worth stating:
 *
 *   Both wrestlers are created from their OWN PSTATUS bit, so each
 *   is a player or a drone independently. On the shipped dispatch
 *   that is dead branching -- `CMPI 3,A0 / JREQ #2plyr` means both
 *   bits are set and both `btst` tests pass -- but the code is
 *   written to be reached with either bit clear and is translated
 *   that way rather than assumed away.
 *
 *   Player two's side is `PSIDE_PLYR2 - royal_rumble`. In a rumble
 *   that is 1 - 1 = 0, putting both humans on side ZERO: the rumble
 *   is how two players end up team-mates rather than opponents, and
 *   it is one subtract that does it.
 *
 *   Buddy mode needs BOTH players to have asked. `move
 *   @p1powerup_request,a8,L / move @p2powerup_request,a9,L / and
 *   a9,a8 / andi BUDDY_MODE,a8` -- an AND, not an OR, so one
 *   player's code is not enough.
 *
 *   With buddy mode on, two more wrestlers are created as
 *   PTYPE_DRONE at slots 2 and 3, one on each side. Which buddy goes
 *   where is not the order they were drawn -- see
 *   wm_buddy_for_player1.
 *
 * Not translated here, and not faked: the `#no_buddies` tail's
 * royal-rumble lineup rewrite (get_royal_lineup into CURRENT_LADDER
 * with NUM_OPPS=2 and FINAL_PTR), because the ladder table it edits
 * is not what this port's match reads.
 * ================================================================= */

/* GAME.EQU:565 `BUDDY_MODE .equ 128`. */
#define WM_MATCH_BUDDY_MODE 128

bool wm_match_buddy_mode_on(int32_t p1powerup_request,
                            int32_t p2powerup_request) {
    /* `and a9,a8 / andi BUDDY_MODE,a8 / jrz #no_buddies`. */
    return ((p1powerup_request & p2powerup_request) &
            WM_MATCH_BUDDY_MODE) != 0;
}

void wm_match_start_two_player(wm_match_state *m, WmRng *rng,
                               int32_t pstatus,
                               uint8_t index1, uint8_t index2,
                               bool royal_rumble,
                               int32_t p1powerup_request,
                               int32_t p2powerup_request) {
    wm_arcade_actor_t *p1, *p2;
    bool buddies_on;
    unsigned i;

    if (!m) return;

    wm_match_init_scroller(m);

    p1 = &m->actors[0];
    p2 = &m->actors[1];
    init_actor_life(p1);
    init_actor_life(p2);

    /* First SCREATE: PSIDE_PLYR1, PLYRNUM 0, @index1. */
    p1->player_num = 0;
    p1->player_side = WM_MATCH_PSIDE_PLYR1;
    p1->wrestler_num = (int32_t)index1;

    /* Second SCREATE: `movi PSIDE_PLYR2,a9 / move @royal_rumble,a14
       / sub a14,a9`, PLYRNUM 1, @index2. */
    p2->player_num = 1;
    p2->player_side = WM_MATCH_PSIDE_PLYR2 - (royal_rumble ? 1 : 0);
    p2->wrestler_num = (int32_t)index2;

    m->index1 = index1;
    m->opponent_wrestler = index2;

    p1->smart_target = p2;
    p2->smart_target = p1;

    place_created_wrestler(m, 0);
    place_created_wrestler(m, 1);

    m->pstatus = pstatus;
    m->royal_rumble = royal_rumble;
    memset(m->actor_is_human, 0, sizeof m->actor_is_human);
    /* `movi PTYPE_PLAYER,a8 / btst 0,a0 / jrnz #ok / movi
       PTYPE_DRONE,a8`, and the same on bit 1 for the second. */
    m->actor_is_human[0] = (pstatus & 1) != 0;
    m->actor_is_human[1] = (pstatus & 2) != 0;
    m->actor_count = 2;
    /* #2plyr never writes @NUM_OPPS -- a two-player game does not climb
       the ladder, so it plays on whatever the last pregame left. One is
       what the two-player match is. */
    m->num_opps = 1;

    /* `movk 1,a14 / move a14,@buddy_mode_checked`. */
    m->buddy_mode_checked = true;
    buddies_on = wm_match_buddy_mode_on(p1powerup_request,
                                        p2powerup_request);
    m->buddy_mode_on = buddies_on;

    if (buddies_on) {
        wm_buddies_t b = wm_choose_buddies(index1, index2, rng);
        wm_arcade_actor_t *b1 = &m->actors[2];
        wm_arcade_actor_t *b2 = &m->actors[3];

        init_actor_life(b1);
        init_actor_life(b2);

        /* `movk PTYPE_DRONE,a8 / movi PSIDE_PLYR1,a9 / movk 2,a10 /
           PULL a11` -- player one's partner, on his side, slot 2. */
        b1->player_num = 2;
        b1->player_side = WM_MATCH_PSIDE_PLYR1;
        b1->wrestler_num = (int32_t)wm_buddy_for_player1(&b);

        /* `movk PTYPE_DRONE,a8 / movk PSIDE_PLYR2,a9 / movk 3,a10 /
           PULL a11`. Note this one is a bare PSIDE_PLYR2: the
           royal-rumble subtract is NOT applied to the buddies, so in
           a rumble the humans share side 0 while their partners stay
           on opposite sides. That is what the source does. */
        b2->player_num = 3;
        b2->player_side = WM_MATCH_PSIDE_PLYR2;
        b2->wrestler_num = (int32_t)wm_buddy_for_player2(&b);

        /*
         * #set0 again, and the row is the count of wrestlers
         * already on that side -- so each partner takes row 1,
         * "second", of his own team's table rather than an offset
         * from his player.
         */
        place_created_wrestler(m, 2);
        place_created_wrestler(m, 3);

        m->actor_count = 4;
    }

    for (i = 0; i < m->actor_count; ++i) {
        wm_arcade_drone_init(&m->drones[i], 0);
        if (m->actor_is_human[i]) wm_human_input_init(&m->human_input[i]);
    }

    /* The first human, for everything that asks "is anybody
       playing" rather than "which actor". */
    m->has_human = m->actor_is_human[0] || m->actor_is_human[1];
    m->human_actor_index = m->actor_is_human[0] ? 0u : 1u;
    wm_human_input_init(&m->human_input_state);

    init_plyr_types(m);
    init_smoves(m);
    init_bret_backends(m);
    m->active = true;
    m->tick_count = 0;
    match_start_common_tail(m);
    match_ring_the_bell(m);
}

/*
 * WRESTLE.ASM:1726 #ndrone -- the drone TEAM.
 *
 * #1plyr creates the human and falls straight into this loop:
 *
 *     MOVE @CURRENT_LADDER,A4,L / MOVE *A4,A4,L
 *     MOVK 2,A10 / move @NUM_OPPS,a3
 *   #nxtdrn
 *     CALLA SORT_OUT_WRESTLER_NUM ... SCREATE ...
 *     SRL 8,A4 / INC A10 / dsj a3,#nxtdrn
 *
 * -- the opponents are the successive bytes of the packed ladder
 * entry, PLYRNUM counts up from 2, and every drone goes on the side
 * OPPOSITE the human.
 *
 * This port drew ONE randomly-chosen opponent for every rung, and
 * the ladder it needed was ported all along (wm/pregame.h's
 * opponents[]/opponent_count come from the real scramble_table) --
 * it was simply never handed over. The rungs are not all singles:
 * 0-3 are one-on-one, 4 and 5 are two-on-one and 6, the final
 * battle, is three-on-one.
 *
 * `opponents`/`count` may be NULL/0, which keeps the old behaviour
 * exactly: one opponent, drawn.
 */
void wm_match_start_one_player_team(wm_match_state *m, WmRng *rng,
                                    int32_t pstatus,
                                    uint8_t index1, uint8_t index2,
                                    const uint8_t *opponents,
                                    unsigned count) {
    wm_arcade_actor_t *human, *opp;
    uint8_t selected;
    unsigned i;

    if (!m || (pstatus != 1 && pstatus != 2)) return;

    wm_match_init_scroller(m);

    human = &m->actors[0];
    opp = &m->actors[1];
    init_actor_life(human);
    init_actor_life(opp);

    /* WRESTLE.ASM #1plyr: bit 0 means P1/index1/side0/PLYRNUM0;
       otherwise the one human is P2/index2/side1/PLYRNUM1. */
    if (pstatus & 1) {
        human->player_num = 0;
        human->player_side = WM_MATCH_PSIDE_PLYR1;
        selected = index1;
    } else {
        human->player_num = 1;
        human->player_side = WM_MATCH_PSIDE_PLYR2;
        selected = index2;
    }
    human->wrestler_num = (int32_t)selected;

    m->index1 = index1;

    /* `move @NUM_OPPS,a3`, clamped to what the ring can hold. */
    if (count < 1u) count = 1u;
    if (count > WM_MATCH_MAX_ACTORS - 1u) count = WM_MATCH_MAX_ACTORS - 1u;

    for (i = 0; i < count; ++i) {
        wm_arcade_actor_t *d = &m->actors[1 + i];
        uint8_t w;

        init_actor_life(d);
        /* `MOVK 2,A10` then `INC A10` each time round. */
        d->player_num = (int32_t)(2 + i);
        /* `btst 0,a14 / jrnz #pside_set` -- always the other side. */
        d->player_side = human->player_side == WM_MATCH_PSIDE_PLYR1
                             ? WM_MATCH_PSIDE_PLYR2
                             : WM_MATCH_PSIDE_PLYR1;
        /* SORT_OUT_WRESTLER_NUM on the entry's next byte; the caller
           has applied it (wm_pregame_opponent_at). With no ladder to
           read, the old placeholder draw stands in. */
        w = opponents ? opponents[i] : 0xffu;
        if (w == 0xffu) w = (uint8_t)wm_match_draw_wrestler_index(rng);
        d->wrestler_num = (int32_t)w;
        if (i == 0) m->opponent_wrestler = w;
        d->smart_target = human;
    }

    /* smart_target is a fixed pair here; match_opponent_of re-picks
       the real nearest on the other side every tick. */
    human->smart_target = opp;

    m->actor_count = 1u + count;
    /* `move @NUM_OPPS,a3` -- the rung's own opponent count. */
    m->num_opps = (int32_t)(count ? count : 1u);
    for (i = 0; i < m->actor_count; ++i) place_created_wrestler(m, i);
    for (i = 0; i < m->actor_count; ++i) wm_arcade_drone_init(&m->drones[i], 0);

    m->pstatus = pstatus;
    m->royal_rumble = false;
    memset(m->actor_is_human, 0, sizeof m->actor_is_human);
    m->actor_is_human[0] = true;
    wm_human_input_init(&m->human_input[0]);
    m->has_human = true;
    m->human_actor_index = 0;
    wm_human_input_init(&m->human_input_state);

    init_plyr_types(m);
    init_smoves(m);
    init_bret_backends(m);
    m->active = true;
    m->tick_count = 0;
    match_start_common_tail(m);
}

/* #1plyr with no ladder team: one drawn opponent, as before. */
void wm_match_start_one_player(wm_match_state *m, WmRng *rng,
                               int32_t pstatus,
                               uint8_t index1, uint8_t index2) {
    wm_match_start_one_player_team(m, rng, pstatus, index1, index2, NULL, 0);
}

void wm_match_start_selected(wm_match_state *m, WmRng *rng,
                             uint8_t p1_source_wrestler) {
    wm_match_start_one_player(m, rng, 1, p1_source_wrestler, 0);
    match_ring_the_bell(m);
}

/*
 * wm_arcade_adjust_health's death_anim bridge. LIFEBAR.ASM's death
 * dispatch names two per-wrestler tables and this resolves both:
 * fallbacks_t (:1849) for a man knocked off his feet, convulse_t
 * (:1863) for one already on the ground.
 *
 * It used to be Bret-only, "since only the one carrying WM_ROSTER_BRET
 * has a real backend to dispatch through". Every wrestler has had a
 * program-driven backend since the generic dispatcher landed, and both
 * tables carry all nine, so the roster table decides the label and
 * match_change_anim starts it -- the same route the climb animations
 * take out of confine_wrestler.
 */
static void wm_match_death_change_anim(wm_arcade_actor_t *victim,
                                       wm_arcade_react1_anim_group_t anim,
                                       void *user) {
    wm_match_state *m = (wm_match_state *)user;
    const wm_roster_anim_table *table;
    const char *table_name;
    const char *label;

    if (!m || !victim) return;
    if (anim == WM_R1_ANIM_FALL_BACK)            table_name = "fallbacks_t";
    else if (anim == WM_R1_ANIM_HIT_ON_GROUND)   table_name = "convulse_t";
    else return;

    /*
     * Bret has a richer backend than the label path can drive: it
     * selects by typed id and keeps current_id, which his dispatcher
     * reads back. Where a typed id exists for what the table names, use
     * it -- fall_back is one. hitonground is not (his typed enum carries
     * only the FACEDOWN variant, a different animation from the one
     * convulse_t names), so that falls through to the label.
     */
    if (anim == WM_R1_ANIM_FALL_BACK) {
        unsigned i;
        for (i = 0; i < m->actor_count; ++i) {
            if (&m->actors[i] != victim) continue;
            if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
                wm_bret_backend_change_anim(victim, WM_BRET_ANIM_FALL_BACK,
                                            &m->bret_visual[i]);
                return;
            }
            break;
        }
    }

    table = wm_roster_anim_find(table_name);
    if (!table) return;
    label = wm_roster_anim_for(table, (int)victim->wrestler_num);
    if (!label) return;                 /* slot 7, the cut Adam Bomb */
    match_change_anim(victim, label, m);
}

/*
 * REACT1.ASM's change_anim hook: the victim's own reaction animation.
 *
 * This is what makes a hit look like a hit. Until it was wired, the
 * reaction was COMPUTED on every blow and then discarded -- damage
 * applied and nothing else, so nobody ever staggered, flinched or went
 * down. See wm/arcade/wm_arcade_react_anims.h for how a typed group
 * becomes one wrestler's label, and which ten groups have no table yet.
 */
static void wm_match_react_change_anim(wm_arcade_actor_t *victim,
                                       wm_arcade_react1_anim_group_t group,
                                       void *user) {
    wm_match_state *m = (wm_match_state *)user;
    const char *label;
    if (!m || !victim) return;
    label = wm_react_anim_label(group, (int)victim->wrestler_num,
                               victim->facing_dir);
    if (!label) return;
    match_change_anim(victim, label, m);
}

/* REACT's own sound hook, onto the same queue every other cue uses. */
static void wm_match_react_sound(wm_arcade_actor_t *victim,
                                 wm_arcade_react1_sound_t sound,
                                 void *user) {
    wm_match_state *m = (wm_match_state *)user;
    (void)victim;
    if (m && m->anim_sound) m->anim_sound(m->anim_sound_user, (uint16_t)sound);
}

/* REACT2's `calla triple_sound` with an explicit id. */
static void wm_match_react_triple_sound(wm_arcade_actor_t *victim,
                                        uint16_t sound_id, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    (void)victim;
    if (m && m->anim_sound) m->anim_sound(m->anim_sound_user, sound_id);
}

/*
 * LIFEBAR.ASM:3290 FIRSTATT_MESS, the FIRST HIT banner.
 *
 * The ledger said: "Its AWARD half is already wired separately through
 * react_cb.round_first_hit_award ... so what is left behind this seam is
 * the drawing." Almost. FIRSTATT_MESS sets up its own message and then
 * `jruc #common` -- INTO BONUS_MESS's tail -- and #common's second
 * instruction after the message is `MOVI 0BBH,A0 / CALLA triple_sound`.
 * The guitar is behind this seam too, and it is not drawing.
 *
 * The rest genuinely is: the FIRSTATT banner itself, and the xDAMAGE
 * "2x" banner #inhere shows after it. The multiplier those words
 * describe is set by the CALLER (ANIM.ASM:2261 `movk 2,a14 / move
 * a14,@DAM_MULT`), which this port already does.
 */
static void match_first_hit_message(wm_arcade_actor_t *attacker, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || !attacker) return;
    wm_match_react_triple_sound(attacker, 0xBBu, m);
}

/*
 * REACT1.ASM:746 #goto_stand_anim and :753 #abort_att_anim -- what
 * hit_stuff does to a THIRD wrestler when the man he was holding gets
 * hit by somebody else.
 *
 * The seam was ledgered as untraced: "A teammate breaking up a hold in
 * buddy mode. The port runs a fixed pair by default; the call site is
 * live only with four actors, and the routine behind it has not been
 * traced." It is two instructions and a label apiece:
 *
 *     #goto_stand_anim                #abort_att_anim
 *         PUSH  a13                       PUSH  a13
 *         move  a1,a13                    move  a1,a13
 *         movi  xxx_goto_stand_anim,a0    movi  xxx_aborted_attach_anim,a0
 *         calla change_anim1a             calla change_anim1a
 *         PULL  a13                       PULL  a13
 *
 * The `move a1,a13` is the partner being swapped in so change_anim1a
 * acts on HIM rather than on the victim -- the same register-convention
 * plumbing ck_ignore's caller does, and the reason this reads as a
 * routine of its own when it is a two-line arm.
 *
 * Both animations are generic rather than per wrestler: `xxx_` in this
 * source means shared, and REACT1.ASM:1873 and :1927 define them once
 * for the whole roster. They have been in the generated programs since
 * they were extracted; nothing selected them.
 *
 * Both are change_anim1a, unguarded -- a partner already standing is
 * sent back to the top of the stand animation rather than left alone.
 *
 * The note was right about one thing: this needs a third party. The
 * caller only fires when the partner's own ATTACH_PROC points back at
 * the victim AND the partner is not the attacker, which two wrestlers
 * cannot satisfy. Buddy mode's four actors can.
 */
static void match_partner_breakout(wm_arcade_actor_t *partner,
                                   wm_arcade_partner_breakout_t kind,
                                   void *user) {
    const char *label;
    if (!partner) return;
    switch (kind) {
    case WM_PARTNER_GOTO_STAND:        label = "xxx_goto_stand_anim"; break;
    case WM_PARTNER_ABORT_ATTACH_ANIM: label = "xxx_aborted_attach_anim"; break;
    default:                           return;
    }
    match_change_anim(partner, label, user);
}

/*
 * LIFEBAR.ASM:3302 BONUS_MESS, the non-display half.
 *
 * The seam was ledgered as display, with the question of whether an
 * award rides along left to "be read off the callers". It is not in the
 * callers: BONUS_MESS itself sets DAM_MULT and scores HIGH_RISK_AWD on
 * every path, so every secret move in the game is meant to hit harder
 * and to score, and with this seam empty it did neither.
 *
 * DAM_MULT already had a working consumer -- adjust_health scales by
 * (delta*(1+mult))>>1 and clears it -- so this is the writer that
 * consumer was short of on the secret-move path.
 */
static void match_bonus_mess(wm_match_state *m, wm_arcade_actor_t *attacker,
                             int bonus) {
    wm_bonus_mess_result r;
    if (!m || !attacker) return;
    r = wm_bonus_mess(&m->bonus_mess, bonus, attacker->risk);
    if (!r.ran) return;
    if (r.high_risk_award)
        match_backend_round_award(m, (int)attacker->player_num,
                                  (int)WM_AWARD_HIGH_RISK);
    /* `MOVI 0BBH,A0 / CALLA triple_sound` -- the guitar. The source
       reaches it only past is_there_one_already, which asks whether a
       message is already on this side's screen; nothing in this port
       puts one there, so the gate never fires and the sound always
       plays. That is what the code does with no messages, not a
       simplification of it. */
    if (r.guitar) wm_match_react_triple_sound(attacker, 0xBBu, m);
    /*
     * dam_mult 0 means "leave the caller's value alone" -- ANIM.ASM:2234
     * has already put 4 there for the taunt-style path, and #tag's own
     * write is commented out at :3320.
     */
    if (r.dam_mult != 0) m->combat_runtime.dam_mult = r.dam_mult;
    /* r.show_text is the message itself, which this port does not draw. */
}

/* REACT1.ASM:550 and ANIM.ASM:2237, both `CREATE0 BONUS_MESS` with A10
   already -1 -- the taunt-style high risk rather than a move number. */
static void match_react_bonus_message(wm_arcade_actor_t *attacker,
                                      int bonus, void *user) {
    match_bonus_mess((wm_match_state *)user, attacker, bonus);
}

/* REACT2's `calla get_health`. */
static int32_t wm_match_react_get_health(const wm_arcade_actor_t *victim,
                                         void *user) {
    (void)user;
    return victim ? victim->life : 0;
}

/* REACT2's ck_live_teammates, against this match's own roster. */
static int wm_match_react_live_teammates(const wm_arcade_actor_t *victim,
                                         void *user) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned i;
    if (!m || !victim) return 0;
    for (i = 0; i < m->actor_count; ++i) {
        const wm_arcade_actor_t *p = &m->actors[i];
        if (p == victim || !p->active) continue;
        if (p->player_side != victim->player_side) continue;
        if (p->player_mode == WM_PMODE_DEAD) continue;
        return 1;
    }
    return 0;
}

/*
 * REACT3 hit_bigboot's `RNDPER 100` -- "return nonzero iff the target
 * implementation's RNDPER leaves HI true". The same shared RAND every
 * other draw in the game stirs.
 */
static int wm_match_react_rndper_hi(uint16_t argument, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m || !m->anim_rng) return 0;
    /*
     * UTIL.ASM:1734 RNDPER, written out the same way src/core/anim_code.c
     * and wrestler_taunt.c already read it: RNDRNG0(999) is the identical
     * stir and mul_high, and `jrhi` means the event happens when the
     * probability EXCEEDS the draw. The argument is per mille, so
     * REACT3's own `RNDPER 100` is a one-in-ten chance and not one in
     * four.
     */
    return wm_rng_rndrng0(m->anim_rng, 999u) < (uint32_t)argument ? 1 : 0;
}

/* REACT1's wres_collis_off on the victim. */
static void wm_match_react_collisions_off(wm_arcade_actor_t *victim,
                                          void *user) {
    (void)user;
    wm_arcade_wrestler_collisions_off(victim);
}

/*
 * wm_arcade_react_callbacks_t.reaction -- the seam that was declared,
 * built for, and never filled in.
 *
 * It exists as an adapter rather than as the bridge itself because
 * wm_arcade_react_callbacks_t carries ONE `user` for all its hooks: the
 * match needs it for adjust_health, and the REACT dispatcher needs a
 * wm_arcade_react1_context_t. So the match owns the context and this
 * hands it across.
 */
static void wm_match_reaction(wm_arcade_actor_t *attacker,
                              wm_arcade_actor_t *victim,
                              wm_arcade_reaction_id_t reaction,
                              int16_t *hit_damage_pending,
                              int16_t *new_victim_movedir,
                              void *user) {
    wm_match_state *m = (wm_match_state *)user;
    if (!m) return;
    wm_arcade_react123456789_reaction_callback(attacker, victim, reaction,
                                               hit_damage_pending,
                                               new_victim_movedir,
                                               &m->react1_ctx);
}

/* wm_arcade_react_callbacks_t.adjust_health adapter: the real logic lives
   in wm_arcade_adjust_health (wm/arcade/wm_arcade_lifebar.h), shared with
   BRET.ASM's own self-death path (wm_bret_backend_callbacks). */
static void wm_match_adjust_health(wm_arcade_actor_t *victim, int16_t signed_delta,
                                   wm_arcade_actor_t *damage_source, void *user) {
    wm_match_state *m = (wm_match_state *)user;
    wm_arcade_death_anim_callback_t death_anim;
    death_anim.change_anim = wm_match_death_change_anim;
    death_anim.user = m;
    wm_arcade_adjust_health(victim, signed_delta, damage_source,
                            m ? !m->has_human : false,
                            m ? m->tick_count : 0,
                            m ? &m->combat_runtime.dam_mult : NULL,
                            &death_anim);
}

void wm_match_tick(wm_match_state *m, const wm_arcade_drone_callbacks_t *cb,
                   const wm_input_state *human_input) {
    wm_arcade_drone_world_t world;
    wm_arcade_actor_t *actor_ptrs[WM_MATCH_MAX_ACTORS];
    unsigned i;

    if (!m || !m->active) return;

    for (i = 0; i < m->actor_count; ++i)
        actor_ptrs[i] = &m->actors[i];

    /*
     * The reset a decided round is owed, run at the TOP of a later
     * tick rather than the one that decided it -- so the deciding
     * tick's own state is still there to be read, the way it is while
     * announce_rnd_winner is still running in the source.
     *
     * LIFEBAR.ASM:2649 `#fini_wait` is the gate: announce_rnd_winner,
     * and therefore everything downstream of it including
     * WRESTLERS_RESET, sits polling while @in_finish_move is set, so a
     * finishing move plays out in full before the next round is set
     * up. Without it the Undertaker's coffin sequence is wiped
     * half-way through by the reset for the round it just won.
     */
    if (m->round_reset_pending && !m->in_finish_move) {
        m->round_reset_pending = false;
        match_reset_for_round(m);
    }

    /*
     * The end-of-match sequence, once one has started. Like the reset
     * above it waits on a finishing move: DO_WAIT is downstream of
     * `#fini_wait` in the same routine, so the coffin has to finish
     * before the match does.
     */
    if (!m->in_finish_move) {
        wm_match_end_ctx_t mec;
        memset(&mec, 0, sizeof(mec));
        mec.score = &m->score;
        mec.streaks = &m->streaks;
        mec.pstatus = m->pstatus;
        mec.match_cnt = m->match_cnt;
        /*
         * DO_RIGHT_MUSIC reads `*A10(WRESTLERNUM)` -- the winner's
         * process, which set_winner put in a10 inside
         * announce_rnd_winner. On the source's KO path that routine
         * really does run (WRESTLE2.ASM:4249 CREATEs it), so a10 is
         * always valid there. This port's KO countdown awards the
         * round itself and never starts the announcer, so when it has
         * no winner the side the countdown decided is used instead --
         * with one wrestler a side, that is the same man set_winner
         * would have found.
         */
        mec.winner_wrestler_num = -1;
        if (m->round_announce.winner) {
            mec.winner_wrestler_num = m->round_announce.winner->wrestler_num;
        } else if (m->score.match_winner > 0) {
            int side = (int)m->score.match_winner - 1;
            unsigned wi;
            for (wi = 0; wi < m->actor_count; ++wi)
                if (m->actors[wi].active &&
                    (int)m->actors[wi].player_side == side) {
                    mec.winner_wrestler_num = m->actors[wi].wrestler_num;
                    break;
                }
        }
        /* @award_ok_to_die reaching 3. Nothing here draws the award
           bar, so there is nothing to wait for. */
        mec.awards_done = true;
        mec.user = m;
        mec.royal_rumble = m->royal_rumble;
        mec.run_awards = match_end_awards;
        mec.run_winstreak_award = match_end_winstreak_award;
        mec.reset_winstreak_rows = match_end_reset_winstreak_rows;
        mec.sound = match_end_sound;
        mec.arm_winstreak = match_end_arm_winstreak;
        mec.adjust_perfects = match_end_adjust_perfects;
        mec.total_icons = match_end_total_icons;
        /*
         * LIFEBAR.ASM:2985, the last thing DO_WAIT does before it
         * dies: `MOVK 2,A0 / move a0,@match_over`. Until this the
         * match ended and nothing above it could tell -- the app sat
         * in its match mode forever, which was the same dead end a
         * decided round and a finished match each used to be one
         * level down.
         */
        if (wm_match_end_tick(&m->match_end, &mec)) m->match_over = 2;
    }

    /*
     * DOINK.ASM:2999 mode_dead's `#zmb` tail, one tick per zombie.
     *
     * The source runs it from the wrestler's own mode table, where
     * wm_arcade_mode_dead sits in this port. It is driven from here
     * instead because finishing the job needs things only the match has
     * -- init_smoves' watchdog tables and the life data -- and splitting
     * the transform across a backend callback would put half of
     * change_wrestler somewhere it cannot see either. What the wrestler
     * needs done to himself is still in one place, in
     * wm/arcade/wm_arcade_final_battle.h.
     */
    {
        unsigned zi;
        for (zi = 0; zi < m->actor_count; ++zi) {
            wm_arcade_actor_t *z = &m->actors[zi];
            if (!z->active) continue;
            if (!(z->status_flags & WM_STATUS_ZOMBIE)) continue;
            if (z->player_mode != WM_PMODE_DEAD) continue;
            if (wm_final_zombie_tick(z, m->scroll.worldtlx, NULL, NULL, NULL))
                match_finish_zombie_transform(m, zi);
        }
    }

    memset(&world, 0, sizeof(world));
    world.actors = actor_ptrs;
    world.actor_count = m->actor_count;
    /* @PCNT, the source's own free-running process/frame counter. This was
       left at 0 while the drone engine had no data to act on, which made
       drone_main's two real PCNT gates degenerate: "(PCNT+1) low four bits
       clear" (its every-16-ticks passive check) was never satisfied, so a
       MODE_NORMAL drone could never reach #doact and never selected an
       action script at all, and "PCNT low five bits clear" (its every-32-
       ticks aggression reroll) was satisfied on every single tick instead.
       Same counter the Bret backend already uses for its own pcnt. */
    world.pcnt = m->tick_count;
    world.round_tickcount = (uint16_t)m->tick_count;
    world.first_ladder = 1;

    /* DCSSOUND.ASM's own DUMMY_WAIT lockout, which gates the shove taunts,
       counts down in real time rather than per wrestler. */
    wm_anim_code_tick();

    for (i = 0; i < m->actor_count; ++i) {
        /* WRESTLE.ASM:2418 `callr update_newfacing`, called unconditionally
           for every wrestler process before the human/zombie/drone_main
           branch. The opponent is CLOSEST_NUM's own answer -- see
           match_opponent_of, which stops being `the other slot` once
           buddy mode puts four wrestlers in the ring. */
        wm_arcade_actor_t *opp_of_i = match_opponent_of(m, i);
        if (opp_of_i) wm_arcade_update_newfacing(&m->actors[i], opp_of_i);

        /* WRESTLE.ASM's main loop computes each wrestler's own CLOSEST_*
           fields every tick for every process (drone_main's own AI
           decisions -- block detection, range-band script selection --
           read them exactly like move_bret does), not just for whichever
           actor happens to carry a real Bret backend. Previously this only
           ran inside the Bret branch below, so a drone-controlled actor's
           AI acted on the *previous* tick's stale distances; a drone-
           controlled Bret got fresh data for move_bret but stale data for
           its own drone_main decision that same tick.

           Through calc_closest2 rather than calc_closest, because the
           arcade's own throttle -- every fourth tick, staggered by
           PLYRNUM, unless the opponent is dead -- skips the distance
           update and not just the target choice, and the AI reads those
           distances. See wm/arcade/wm_arcade_closest.h. */
        if (opp_of_i)
            (void)wm_arcade_calc_closest2(&m->actors[i], opp_of_i, world.pcnt);

        if (m->actor_is_human[i]) {
            /*
             * Each wrestler reads its OWN switches -- PLYRNUM picks
             * the set, exactly as get_but_val_cur(PLYRNUM) does.
             * Player one falls back to the tick's own argument so a
             * single-player caller never has to set anything.
             */
            unsigned pn = (unsigned)m->actors[i].player_num;
            const wm_input_state *in = NULL;
            if (pn < 2 && m->player_input_set[pn]) in = &m->player_input[pn];
            else if (pn == 0) in = human_input;
            wm_human_input_commit(&m->actors[i], &m->human_input[i], in);
        } else {
            uint16_t old_but = m->drones[i].but;
            uint16_t old_joy = m->drones[i].joy;
            (void)wm_arcade_drone_main(&m->actors[i], &m->drones[i], &world, cb);
            wm_arcade_drone_commit_inputs(&m->actors[i], &m->drones[i], old_but, old_joy);

            /*
             * DRONE.ASM:2810 drn_taunt's own change_anim1a. It is the
             * only drone script that plays an animation, so rather than
             * hand the drone layer a visual backend it leaves the label
             * on its state and this picks it up -- the same shape as
             * wm_match_death_change_anim above. Bret runs a
             * wm_visual_sequence track rather than the animation VM and
             * has no label route, so his taunt still only sets RISK.
             */
            if (m->drones[i].pending_anim) {
                if (m->actors[i].wrestler_num != WM_ROSTER_BRET) {
                    wm_arcade_roster_callbacks_t taunt_cb =
                        wm_wrestler_roster_callbacks(&m->wrestler_visual[i]);
                    if (taunt_cb.change_anim_label)
                        taunt_cb.change_anim_label(&m->actors[i],
                                                   m->drones[i].pending_anim,
                                                   taunt_cb.user);
                }
                m->drones[i].pending_anim = NULL;
            }
        }

        /*
         * This wrestler's own special-move monitors, run here because
         * the arcade's sit immediately BEFORE him in the process list
         * (GETPRC_INSERT, MPROC.ASM:358) -- see match_tick_smoves_for.
         * His switches have just been read; nothing of his own has
         * moved yet.
         */
        match_tick_smoves_for(m, i);

        /* WRESTLE.ASM:2453 `callr count_button_presses`, right after
           update_joystat and before animate_wrestler: every newly-pressed
           button bumps its own PLYR.EQU counter, and the animation VM's
           ANI_IF_BUTCOUNT_GE/LT branch on those counts. That is the whole
           button-mash mechanic -- a repeated knee to the head keeps going
           only while kick keeps being pressed. Runs for every wrestler,
           human or drone, since it is shared WRESTLE.ASM code and a
           drone's own presses count exactly like a player's; it has to
           happen after this tick's input is committed and before the
           animation reads a count. */
        wm_arcade_count_button_presses(&m->actors[i]);

        /*
         * WRESTLE.ASM:2457 `calla wrestler_veladd` / `callr
         * wrestler_friction`, in the source's own main-loop order: after
         * count_button_presses, BEFORE the animation ticks, and before the
         * confinement pass -- so this tick's animation and this tick's
         * confinement both see the position the velocities just produced.
         *
         * The exec handed over is the wrestler's own running animation:
         * landing stuffs a 1 into ANICNT when MODE_WAITHITOPP is set, which
         * is how an ANI_WAITHITOPP hold ends early. Bret runs the full
         * visual backend, the other seven the shared one.
         */
        wm_wrestler_veladd(&m->actors[i],
                           m->actors[i].wrestler_num == WM_ROSTER_BRET
                               ? &m->bret_visual[i].prog
                               : &m->wrestler_visual[i].prog,
                           0 /* this port has no INPREGAME2 phase */);
        wm_wrestler_friction(&m->actors[i]);

        /*
         * WRESTLE.ASM:2538's getup meter, in the same main-loop pass and
         * before the animation runs -- ANI_WAITROLL reads GETUP_TIME to
         * decide whether the wrestler may start rolling yet, so it has to
         * see this tick's value.
         */
        /* WRESTLE.ASM:2503's countdown block runs immediately before the
           getup meter, in the same main-loop pass. */
        wm_arcade_tick_wrestler_timers(&m->actors[i]);
        wm_arcade_tick_getup_time(&m->actors[i]);

        /*
         * One tick of his own getup_meter process (WRESTLE2.ASM:950).
         * It runs AFTER the countdown above, which is the order two
         * separate processes fall into here and is the order that
         * matters: the meter reads DELAY_METER, and the countdown is
         * what brings it down.
         *
         * The frame it asks for is thrown away. What it is here for is
         * slide_offscr's `movi 18*60,a0 / move a0,*a10(DELAY_METER)`,
         * which nothing else in this port writes; the green bar itself
         * is the display's and there is nothing yet to hand it to.
         */
        {
            wm_getup_meter_frame_t gmf;
            wm_arcade_getup_meter_tick(&m->getup_meter[i], &m->actors[i],
                                       NULL, &gmf);
            (void)gmf;
        }

        /* WRESTLE.ASM::move_wrestler dispatches every wrestler process
           through its own move_xxx; wm_arcade_move_ported_wrestler is that
           dispatcher, and all eight per-wrestler modules behind it are real
           direct ports. Bret is the one with a full visual backend
           (wm/bret_backend.h: real frame data, attack windows, hurt_box);
           the other seven run the same real decision logic through the
           shared, animation-free backend in wm/wrestler_backend.h. */
        {
            /* CLOSEST_NUM's answer, not "the other slot" -- with
               buddy mode a wrestler's partner shares his side. */
            wm_arcade_actor_t *opp = opp_of_i ? opp_of_i : &m->actors[i];
            const wm_arcade_wrestler_profile_t *profile =
                wm_arcade_roster_profile((wm_arcade_roster_id_t)m->actors[i].wrestler_num);
            wm_arcade_roster_env_t env;
            wm_arcade_wrestler_port_bindings_t bind;
            wm_arcade_bret_callbacks_t bret_cb;
            wm_arcade_razor_callbacks_t razor_cb;
            wm_arcade_roster_callbacks_t roster_cb;
            const bool is_bret = m->actors[i].wrestler_num == WM_ROSTER_BRET;

            memset(&env, 0, sizeof(env));
            env.pcnt = m->tick_count;
            env.p1rounds = m->score.p1rounds;
            env.p2rounds = m->score.p2rounds;

            m->wrestler_visual[i].opponent = opp;
            m->wrestler_visual[i].pcnt = m->tick_count;
            m->wrestler_visual[i].attract_mode = !m->has_human;
            m->wrestler_visual[i].instant_combos_on = m->instant_combos_on;
            m->wrestler_visual[i].mode_dead_env = match_mode_dead_env(m, actor_ptrs);
            m->wrestler_visual[i].wrestler_num = m->actors[i].wrestler_num;
            m->wrestler_visual[i].anim_env.opponent = opp;
            m->wrestler_visual[i].anim_env.rng = m->anim_rng;
            m->wrestler_visual[i].anim_env.pcnt = m->tick_count;
            m->wrestler_visual[i].anim_env.sound_user = m->anim_sound_user;
            m->wrestler_visual[i].anim_env.sound = m->anim_sound;
            /* FINISEQ.ASM's coffin globals, shared by both wrestlers:
               the Undertaker's animation and the dead man's push_in_anim
               poll and set the same three. */
            m->wrestler_visual[i].anim_env.coffin = &m->coffin;
            /* PROGRESS.ASM's final-battle queue, borrowed the same
               way: _ani_waitroll reads it when a drone dies. */
            m->wrestler_visual[i].anim_env.final_battle = m->final_battle;
            m->wrestler_visual[i].anim_env.eight_on_one = m->eight_on_one;
            /* @WORLDTLX >> 16. This was left at zero, so every
               routine that reads the scroll's left edge through
               the env -- anim_code's own included -- was answering
               against a camera parked at the origin. */
            m->wrestler_visual[i].anim_env.world_tlx = m->scroll.worldtlx;
            m->wrestler_visual[i].anim_env.roster_user = m;
            m->wrestler_visual[i].anim_env.actor_by_plyrnum = match_actor_by_plyrnum;
            m->wrestler_visual[i].anim_env.kill_smoves = match_kill_smoves;
            /*
             * ANIM.ASM:2130 ANI_SLAVEANIM and everything else that
             * starts an animation on somebody ELSE -- grnd_hit, and
             * the coffin sequence's push_to_coffin and
             * make_wres_disappear. The seam has been declared since
             * the VM was written and never supplied, so every one of
             * those silently did nothing to the other wrestler.
             */
            m->wrestler_visual[i].anim_env.slave_user = m;
            m->wrestler_visual[i].anim_env.change_opp_anim = match_change_anim;
            m->wrestler_visual[i].anim_env.create_proc = match_create_proc;
            m->bret_visual[i].anim_env.create_proc = match_create_proc;
            /* can_pin's process_ptrs sweep. */
            m->wrestler_visual[i].all_actors = actor_ptrs;
            m->wrestler_visual[i].all_actor_count = m->actor_count;
            /* RND_AWARD's own sink, for DO_REVERSAL_MESS's
               `RND_AWARD a8,REVERSAL_AWD`. */
            m->wrestler_visual[i].round_award = match_backend_round_award;
            m->wrestler_visual[i].round_award_user = m;
            m->bret_visual[i].all_actors = actor_ptrs;
            m->bret_visual[i].all_actor_count = m->actor_count;
            m->bret_visual[i].round_award = match_backend_round_award;
            m->bret_visual[i].round_award_user = m;

            m->bret_visual[i].opponent = opp;
            m->bret_visual[i].pcnt = m->tick_count;
            m->bret_visual[i].instant_combos_on = m->instant_combos_on;
            m->bret_visual[i].mode_dead_env = match_mode_dead_env(m, actor_ptrs);
            m->bret_visual[i].anim_env.opponent = opp;
            m->bret_visual[i].anim_env.rng = m->anim_rng;
            m->bret_visual[i].anim_env.pcnt = m->tick_count;
            m->bret_visual[i].anim_env.sound_user = m->anim_sound_user;
            m->bret_visual[i].anim_env.sound = m->anim_sound;
            m->bret_visual[i].anim_env.coffin = &m->coffin;
            /* PROGRESS.ASM's final-battle queue, borrowed the same
               way: _ani_waitroll reads it when a drone dies. */
            m->bret_visual[i].anim_env.final_battle = m->final_battle;
            m->bret_visual[i].anim_env.eight_on_one = m->eight_on_one;
            /* @WORLDTLX >> 16. This was left at zero, so every
               routine that reads the scroll's left edge through
               the env -- anim_code's own included -- was answering
               against a camera parked at the origin. */
            m->bret_visual[i].anim_env.world_tlx = m->scroll.worldtlx;
            m->bret_visual[i].anim_env.roster_user = m;
            m->bret_visual[i].anim_env.actor_by_plyrnum = match_actor_by_plyrnum;
            m->bret_visual[i].anim_env.kill_smoves = match_kill_smoves;
            m->bret_visual[i].anim_env.slave_user = m;
            m->bret_visual[i].anim_env.change_opp_anim = match_change_anim;

            /* Both backends reach the same rope banks. */
            m->wrestler_visual[i].anim_env.rope_user = m;
            m->wrestler_visual[i].anim_env.rope_command = match_rope_command;
            m->wrestler_visual[i].anim_env.rope_set_z = match_rope_set_z;
            m->bret_visual[i].anim_env.rope_user = m;
            m->bret_visual[i].anim_env.rope_command = match_rope_command;
            m->bret_visual[i].anim_env.rope_set_z = match_rope_set_z;
            /* ...and the same crowd. */
            m->wrestler_visual[i].anim_env.crowd_user = m;
            m->wrestler_visual[i].anim_env.crowd_cheer = match_crowd_cheer;
            m->wrestler_visual[i].anim_env.crowd_sound = match_crowd_sound;
            m->wrestler_visual[i].anim_env.crowd_busy = match_crowd_busy;
            m->bret_visual[i].anim_env.crowd_user = m;
            m->bret_visual[i].anim_env.crowd_cheer = match_crowd_cheer;
            m->bret_visual[i].anim_env.crowd_sound = match_crowd_sound;
            m->bret_visual[i].anim_env.crowd_busy = match_crowd_busy;
            m->wrestler_visual[i].anim_env.round_user = m;
            m->wrestler_visual[i].anim_env.win_announce = match_win_announce;
            m->bret_visual[i].anim_env.round_user = m;
            m->bret_visual[i].anim_env.win_announce = match_win_announce;
            /* WRESTLE.ASM:4552 init_reduce_bog: active wrestlers minus
               two, so a 1-on-1 match runs with debris on. */
            m->wrestler_visual[i].anim_env.no_debris = m->debris.no_debris;
            m->wrestler_visual[i].anim_env.reduce_bog =
                m->debris.reduce_bog > 0;
            m->wrestler_visual[i].anim_env.debris_user = m;
            m->wrestler_visual[i].anim_env.create_debris = match_create_debris;
            m->wrestler_visual[i].anim_env.set_no_debris = match_set_no_debris;
            m->bret_visual[i].anim_env.no_debris = m->debris.no_debris;
            m->bret_visual[i].anim_env.reduce_bog = m->debris.reduce_bog > 0;
            m->bret_visual[i].anim_env.debris_user = m;
            m->bret_visual[i].anim_env.create_debris = match_create_debris;
            m->bret_visual[i].anim_env.set_no_debris = match_set_no_debris;
            m->wrestler_visual[i].anim_env.react_debris = match_react_debris;
            m->bret_visual[i].anim_env.react_debris = match_react_debris;
            /* SPECIAL.ASM's projectiles: the animation says who is
               throwing and what, the match makes the object. */
            m->wrestler_visual[i].anim_env.special_user = m;
            m->wrestler_visual[i].anim_env.spawn_special = match_spawn_special;
            m->bret_visual[i].anim_env.special_user = m;
            m->bret_visual[i].anim_env.spawn_special = match_spawn_special;
            m->wrestler_visual[i].anim_env.screen_user = m;
            m->wrestler_visual[i].anim_env.draw_move_name = match_draw_move_name;
            m->bret_visual[i].anim_env.screen_user = m;
            m->bret_visual[i].anim_env.draw_move_name = match_draw_move_name;
            m->wrestler_visual[i].anim_env.screen_shake = match_screen_shake;
            m->bret_visual[i].anim_env.screen_shake = match_screen_shake;
            m->wrestler_visual[i].anim_env.create_dizzy = match_create_dizzy;
            m->bret_visual[i].anim_env.create_dizzy = match_create_dizzy;
            m->wrestler_visual[i].anim_env.set_allow_offscrn =
                match_set_allow_offscrn;
            m->bret_visual[i].anim_env.set_allow_offscrn =
                match_set_allow_offscrn;
            m->wrestler_visual[i].anim_env.wake_round_announce =
                match_wake_round_announce;
            m->bret_visual[i].anim_env.wake_round_announce =
                match_wake_round_announce;
            /*
             * WRESTLE.ASM's match-configuration globals, as this match
             * actually is. NUM_OPPS is the ladder rung's own count now
             * that #1plyr creates the real team -- one on rungs 0-3, two
             * on 4 and 5, three on the final battle. The routines that
             * gate on these read them rather than assuming.
             */
            m->wrestler_visual[i].anim_env.royal_rumble = m->royal_rumble;
            m->wrestler_visual[i].anim_env.pstatus = m->pstatus;
            m->wrestler_visual[i].anim_env.num_opps = m->num_opps;
            m->bret_visual[i].anim_env.royal_rumble = m->royal_rumble;
            m->bret_visual[i].anim_env.pstatus = m->pstatus;
            m->bret_visual[i].anim_env.num_opps = m->num_opps;
            m->wrestler_visual[i].anim_env.award_user = m->anim_award_user;
            m->wrestler_visual[i].anim_env.round_award = m->anim_round_award;
            m->bret_visual[i].anim_env.award_user = m->anim_award_user;
            m->bret_visual[i].anim_env.round_award = m->anim_round_award;
            m->wrestler_visual[i].anim_env.announcer = &m->announcer;
            /* LIFEBAR.ASM:108 message_flag -- one per match, like
               the queue beside it. */
            m->wrestler_visual[i].anim_env.bonus_mess = &m->bonus_mess;
            m->wrestler_visual[i].anim_env.anyone_near_death =
                match_anyone_near_death;
            m->wrestler_visual[i].anim_env.announcer_user = m;
            m->bret_visual[i].anim_env.announcer = &m->announcer;
            /* LIFEBAR.ASM:108 message_flag -- one per match, like
               the queue beside it. */
            m->bret_visual[i].anim_env.bonus_mess = &m->bonus_mess;
            m->bret_visual[i].anim_env.anyone_near_death =
                match_anyone_near_death;
            m->bret_visual[i].anim_env.announcer_user = m;

            bret_cb = wm_bret_backend_callbacks(&m->bret_visual[i]);
            razor_cb = wm_wrestler_razor_callbacks(&m->wrestler_visual[i]);
            roster_cb = wm_wrestler_roster_callbacks(&m->wrestler_visual[i]);

            memset(&bind, 0, sizeof(bind));
            bind.bret = &bret_cb;
            bind.razor = &razor_cb;
            bind.taker = &roster_cb;
            bind.yoko = &roster_cb;
            bind.shawn = &roster_cb;
            bind.bam = &roster_cb;
            bind.doink = &roster_cb;
            bind.lex = &roster_cb;

            /* `callr auto_pin_check` -- one line before the
               per-character dispatch, which is the next statement. */
            match_auto_pin_check(m, i, opp, actor_ptrs);

            if (profile)
                (void)wm_arcade_move_ported_wrestler(profile, &m->actors[i], opp,
                                                     &env, &bind);

            if (is_bret) {
                wm_bret_backend_tick(&m->bret_visual[i], &m->actors[i],
                                     (uint16_t)m->tick_count);
                /* WRESTLE.ASM's main loop calls confine_wrestler (via fix1/
                   fix2) right after set_collision_boxes, every tick, for
                   every wrestler process. It reads OBJ_COLLX1/X2, i.e. the
                   hurt box, which every wrestler now has for real -- the
                   other six are confined in the else branch below, for the
                   same reason and at the same point. Runs before position
                   integration so this tick's confinement uses this tick's
                   own hurt_box. */
                match_confine_actor(m, i, actor_ptrs);
                match_apply_mode_dead(m, i, &m->bret_visual[i].mode_dead_result);
            } else {
                wm_wrestler_backend_tick(&m->wrestler_visual[i], &m->actors[i]);
                /* These six now have a real, moving hurt_box of their own
                   (their animations are program-driven), so confine_wrestler
                   applies to them exactly as it does to Bret -- it reads
                   OBJ_COLLX1/X2, which is what the hurt box is. */
                match_confine_actor(m, i, actor_ptrs);
                match_apply_mode_dead(m, i,
                                      &m->wrestler_visual[i].mode_dead_result);
            }
            /*
             * Position integration used to happen here, through
             * wm_integrate_position -- a placeholder that moved X and Z and
             * was documented as "NOT a source routine". The real one is
             * WRESTLE2.ASM:2282 wrestler_veladd, called above at the
             * source's own place in the loop, and it does X, Y and Z with
             * the ground and gravity. Integrating again here would move
             * every wrestler twice per tick.
             */
        }
    }

    {
        /* One call per bank = one source rope-process tick. There is no
           image adapter here: this port has no renderer to hand the
           per-channel image symbols to, so the scripts advance and the
           drawing half is simply absent. */
        unsigned b;
        for (b = 0; b < WM_MATCH_ROPE_BANKS; ++b)
            wm_rope_runtime_tick(&m->ropes[b], NULL);
    }

    /* ANNOUNCE_VOICE, one line a tick, out through the same audio seam
       every other sound in this port uses. */
    wm_announce_tick_repeat(&m->announcer);   /* REPEAT_DUMMY */
    if (m->crowd.sound_ticks) --m->crowd.sound_ticks;   /* CROWD_DUMMY */
    (void)wm_shake_tick(&m->shake);                    /* UTIL.ASM #shaker */
    /* WRESTLE2.ASM:2214 `move @allow_offscrn,a14 / jrz #ok / dec / ...` */
    if (m->allow_offscrn > 0) --m->allow_offscrn;
    (void)wm_announcer_tick(&m->announcer, m->anim_sound_user, m->anim_sound,
                            NULL);

    {
        wm_arcade_react_callbacks_t react_cb;
        wm_arcade_react_bridge_t bridge;
        wm_arcade_combat_callbacks_t combat_cb;

        memset(&react_cb, 0, sizeof(react_cb));
        react_cb.adjust_health = wm_match_adjust_health;
        /* ANIM.ASM:2253 `RND_AWARD a13,FIRST_HIT_AWD`. This seam has been
           declared in wm/arcade/wm_arcade_react.h and pointed at nothing
           since it was written, so the first hit of a round scored no
           award at all. */
        react_cb.round_first_hit_award = wm_match_first_hit_award;
        /* LIFEBAR.ASM:3302 BONUS_MESS -- see the routine's comment. */
        react_cb.bonus_message = match_react_bonus_message;
        /* REACT1.ASM:746/:753 -- see the routine's comment. */
        react_cb.partner_breakout = match_partner_breakout;
        /* LIFEBAR.ASM:3290 -- the guitar, not the banner. */
        react_cb.first_hit_message = match_first_hit_message;
        /*
         * REACT1.ASM's hit_table dispatch. This seam was declared in
         * wm/arcade/wm_arcade_react.h, a signature-compatible bridge was
         * written for it covering REACT1 through REACT9, and nothing
         * ever assigned it -- so every blow computed its reaction and
         * threw it away. Damage landed; nothing else did.
         */
        react_cb.reaction = wm_match_reaction;
        /*
         * REACT1.ASM:428 `calla good_run_hit / jrc #good_hit`, the very
         * first thing wrestler_hit does: a run collision that is not a
         * genuine run hit is discarded before WHOHITME is even set, so
         * "reversals will start hitting innocent bystanders who run by"
         * cannot happen. REACT5.ASM:127 is the test itself and has been
         * translated since fix38 -- but this seam was never assigned, so
         * wm_arcade_wrestler_hit took its `!callbacks->good_run_hit`
         * early-out and returned WM_WRESTLER_HIT_NEEDS_RUN_HOOK instead.
         * Every AMODE_RUN collision in the game did nothing at all.
         */
        react_cb.good_run_hit = wm_arcade_react5_good_run_hit_callback;
        /*
         * REACT1.ASM:798's own `calla ditch_getup_meter`, on a victim
         * who was BOUNCING or RUNNING when he was hit. A second seam
         * of that name, distinct from the REACT5 one in
         * wm_arcade_react1_callbacks_t, and like it declared and
         * filled by nobody until the getup meter existed to fill it
         * with.
         */
        react_cb.ditch_getup_meter = match_react_slide_getup_meter;
        react_cb.user = m;

        /* The context that dispatcher needs, rebuilt each tick so it
           tracks whatever the match's own seams currently point at. */
        memset(&m->react1_cb, 0, sizeof m->react1_cb);
        m->react1_cb.change_anim = wm_match_react_change_anim;
        m->react1_cb.play_sound = wm_match_react_sound;
        m->react1_cb.triple_sound = wm_match_react_triple_sound;
        m->react1_cb.get_health = wm_match_react_get_health;
        m->react1_cb.victim_has_live_teammates = wm_match_react_live_teammates;
        m->react1_cb.rndper_hi = wm_match_react_rndper_hi;
        m->react1_cb.collisions_off = wm_match_react_collisions_off;
        /*
         * REACT4.ASM:196 and :199, and REACT5's own
         * `calla ditch_getup_meter`. All three seams were declared,
         * NULL-checked at their call sites and assigned by nobody, so a
         * bounce off a stomped wrestler shook no ropes and no screen,
         * and a runner hit mid-run kept his getup meter.
         */
        m->react1_cb.shake_all_ropes = match_react_shake_all_ropes;
        m->react1_cb.shaker2 = match_react_screen_shake;
        m->react1_cb.slide_getup_meter = match_react_slide_getup_meter;
        /*
         * REACT1.ASM:1453 and REACT4.ASM:137's ANIBASE comparisons.
         * Declared and filled by nobody until now, so every label
         * comparison in REACT answered 'not that one'.
         */
        m->react1_cb.attacker_uses_lex_flykick_anim =
            match_react_attacker_uses_lex_flykick;
        m->react1_cb.attacker_anim_tag = match_react_attacker_anim_tag;
        m->react1_cb.move_grade = match_react_move_grade;

        m->react1_cb.user = m;
        memset(&m->react1_ctx, 0, sizeof m->react1_ctx);
        m->react1_ctx.callbacks = &m->react1_cb;

        m->combat_runtime.pcnt = m->tick_count;
        m->combat_runtime.round_tickcount = (uint16_t)m->tick_count;

        bridge.runtime = &m->combat_runtime;
        bridge.callbacks = &react_cb;
        memset(&bridge.last_result, 0, sizeof(bridge.last_result));

        memset(&combat_cb, 0, sizeof(combat_cb));
        combat_cb.wrestler_hit = wm_arcade_wrestler_hit_collision_callback;
        combat_cb.user = &bridge;

        (void)wm_arcade_check_wrestler_collisions(actor_ptrs, m->actor_count,
                                                  m->tick_count, &combat_cb);

        /*
         * object_collisions is part of the same check_collisions call,
         * so the sweep is here -- but it deliberately runs against the
         * projectiles as they stood before this tick's animations, the
         * way the arcade's own main-loop ordering does. The stepping
         * half is match_step_specials, below, where the projectile
         * processes themselves would run.
         */
        match_sweep_specials(m, actor_ptrs);
        /* ...and the projectile processes' own turn, which in the
           arcade runs later in the frame than the sweep above. */
        match_step_specials(m);

        /*
         * COLLIS.ASM:56 overlap_collision, which keeps two wrestlers from
         * standing inside one another. The port has had its body
         * (wm_arcade_resolve_overlap) for a long time with nothing calling
         * it, so nothing ever pushed them apart.
         *
         * It runs after the attack sweep and reads the same hurt boxes,
         * which are this tick's: set_collision_boxes' hurt-box half is
         * applied in each backend's tick above, before confine_wrestler.
         */
        {
            size_t oi;
            for (oi = 0; oi < m->actor_count; ++oi) {
                if (!actor_ptrs[oi] || !actor_ptrs[oi]->active) continue;
                (void)wm_arcade_overlap_collision(actor_ptrs[oi], actor_ptrs,
                                                  m->actor_count);
            }
        }

        /*
         * WRESTLE.ASM:2064 `callr final_confine`, the line straight
         * after check_collisions in the main loop. It re-confines only
         * the wrestlers that have an ATTACH_PROC, because a puppet is
         * moved by his master and may have been confined before he was
         * dragged. Everyone else was confined after their own last
         * move, in the per-wrestler pass above.
         */
        wm_arcade_final_confine(actor_ptrs, m->actor_count);

        /*
         * WRESTLE.ASM:3003 update_links, which the source runs once a
         * tick per wrestler: an ATTACH_PROC that is not pointing back
         * at me is stale and gets dropped. Without it a puppet can
         * stay attached to somebody who has already let go, and be
         * dragged around by him for the rest of the round.
         */
        {
            size_t li;
            for (li = 0; li < m->actor_count; ++li)
                if (actor_ptrs[li] && actor_ptrs[li]->active)
                    wm_round_update_links(actor_ptrs[li]);
        }
    }

    {
        bool was_decided = m->round_state.decided;

        /*
         * WRESTLE2.ASM:4098 match_timer, one tick, and WRESTLE.ASM:
         * 2102's `move @match_time,a0,L / jrnz #loop` -- the main
         * loop's own test on it. @HALT is passed false: this port has
         * no pause state to raise it.
         */
        m->clock_warning = false;
        if (!was_decided) {
            wm_match_clock_tick(&m->clock, false, actor_ptrs,
                                m->actor_count, &m->clock_warning);
            if (wm_match_clock_expired(&m->clock)) {
                if (!m->has_human) {
                    /*
                     * WRESTLE.ASM:2115 `#wraparound`: `move @PSTATUS,
                     * a14 / jrnz #norm`. With nobody playing the demo
                     * rolls the clock back to 99 and keeps going, so
                     * an attract match is never ended by the clock.
                     */
                    wm_match_clock_wrap(&m->clock);
                } else {
                    /*
                     * `#norm`: HALT, the velocities zeroed, the
                     * TIME OUT graphic, and announce_rnd_winner --
                     * of which this port has the decision,
                     * LIFEBAR.ASM:5149 set_winner's #tmout branch.
                     * A -1 is the source's own "neither side landed
                     * a blow" answer and awards the round to nobody,
                     * exactly as a double-KO does.
                     */
                    m->round_state.decided = true;
                    m->round_state.decided_winner_side =
                        wm_match_timeout_winner(actor_ptrs, m->actor_count);
                    m->round_state.pin_timeout = 0;
                }
            }
        }

        wm_arcade_round_tick(&m->round_state, actor_ptrs, m->actor_count);
        if (!was_decided && m->round_state.decided) {
            wm_arcade_match_score_award_round(&m->score, m->round_state.decided_winner_side);
            /*
             * LIFEBAR.ASM:3098's `WRESTLERS_RESET` -- "Cause wrestlers
             * to re-appear in the correct spot to start the next
             * round". Until this, a decided round was a dead end: the
             * score moved and nothing else did, so the match simply
             * stopped happening while it kept ticking.
             *
             * Owed rather than done here; see round_reset_pending.
             * It is owed only while the match is still open, because
             * once match_winner is set the source goes to its own
             * end-of-match path instead, which this port does not
             * have.
             */
            if (m->score.match_winner == 0) m->round_reset_pending = true;
            /*
             * LIFEBAR.ASM:2852 DO_WAIT. The other arm of the same
             * branch: two rounds ends the match and the source goes
             * here instead of to WRESTLERS_RESET. Without it a
             * finished match was a dead end -- exactly the shape of
             * bug the round reset just fixed, one level up.
             */
            else {
                /* LIFEBAR.ASM grants these two in announce_rnd_winner,
                   a few lines before CALL_MATCH_OVER -- so before the
                   DO_WAIT sequence this call starts, not inside it. */
                match_grant_match_awards(m, actor_ptrs);
                wm_match_end_start(&m->match_end);
            }
        }
        /*
         * WRESTLE2.ASM:4235 `CREATE PINHIM_ANIM_PID,pin_prompt`, on the
         * one tick a side is newly wiped out. The prompt itself is
         * presentation this port does not draw; what it decides -- is
         * there a live human in the ring on the winning side, and is
         * the dead side really all dead -- is AWARD.ASM's own, and the
         * @p1pins / @p2pins bump behind it is what the Undertaker's
         * finishing move counts.
         */
        if (m->round_state.prompt_pin) {
            wm_arcade_actor_t *pinner =
                wm_arcade_pin_prompt(actor_ptrs, m->actor_count,
                                     m->round_state.prompt_dead_side);
            if (pinner) {
                wm_arcade_pins_award(&m->pins, (int)pinner->player_side);
                /*
                 * AWARD.ASM:1835 `calla END_MATCH_SPEECH ;do the
                 * obnoxious "PIN HIM!" crap`, and the line immediately
                 * under it -- `MOVI 0BBH,A0 / CALLA triple_sound`, the
                 * source's own "Move name annc snd". Both sit inside
                 * pin_prompt, past the same #fp_dn gate this branch is,
                 * and neither had a caller here.
                 */
                if (m->start_pin_him) m->start_pin_him(m->sound_proc_user);
                if (m->anim_sound)
                    m->anim_sound(m->anim_sound_user, WM_MATCH_MOVE_NAME_SND);
            }
        }
    }

    /*
     * WRESTLE2.ASM:1681 scroll_world, run once per tick from the main
     * loop. Its two gates are the caller's (see wm_arcade_scroll.h):
     * @HALT, which this port has no pause state to set, and
     * @in_finish_move, which the coffin finish raises -- the scroller
     * is meant to stop dead while the Undertaker's camera move owns
     * the view.
     */
    if (!m->in_finish_move) {
        size_t ia = 0, ib = 0;
        if (wm_scroll_pick_pair((const wm_arcade_actor_t *const *)actor_ptrs,
                                m->actor_count, m->has_human, &ia, &ib)) {
            wm_scroll_point pa, pb;
            wm_scroll_update_positions(actor_ptrs[ia], &pa);
            wm_scroll_update_positions(actor_ptrs[ib], &pb);
            wm_scroll_world(&m->scroll, &pa, &pb,
                            (const wm_arcade_actor_t *const *)actor_ptrs,
                            m->actor_count);
        }
    }

    /*
     * FINISEQ.ASM's two coffin processes, the ones ANI_CREATEPROC
     * started above. The driver is the sequence's clock: it walks the
     * mat, the coffin, the door and the tombstone on the source's own
     * sleeps and moves @close_the_door / @close_the_floor /
     * @finish_completed at the points the source moves them. The
     * Undertaker's animation is polling exactly those.
     *
     * `puffs` is how many ltl_exp processes the source would have
     * created this tick. Each one is a smoke object with a random
     * position and lifetime and nothing else; the count is carried so
     * a renderer can make them, and the RNG is not drawn from here --
     * doing so would consume the shared stream for objects nothing
     * yet displays.
     */
    if (wm_coffin_driver_busy(&m->coffin_driver))
        (void)wm_coffin_driver_tick(&m->coffin_driver, &m->coffin);

    /* raise_dead: SLEEP TSEC/2, then raise_dead_anim on the dead man. */
    if (m->raise_dead_delay > 0 && --m->raise_dead_delay == 0) {
        if (m->raise_dead_target)
            match_change_anim(m->raise_dead_target,
                              WM_COFFIN_RAISE_DEAD_ANIM, m);
        m->raise_dead_target = NULL;
    }

    /*
     * TAKER.ASM:697 `#fdone_wait`: the finishing move's own monitor
     * sleeps five ticks at a time until @finish_completed, then clears
     * @in_finish_move and kills the shaker. Without it the flag stays
     * raised and the scroller never restarts.
     */
    if (m->in_finish_move && m->coffin.finish_completed != 0) {
        wm_arcade_und_finish_callbacks_t done;
        memset(&done, 0, sizeof(done));
        done.set_in_finish_move = match_set_in_finish_move;
        done.kill_shake = match_kill_shake;
        done.user = m;
        wm_arcade_und_finish_done(&done);
        m->coffin.finish_completed = 0;
    }

    /*
     * shake_world's `#sw_loop`: jitter, SLEEPK 3, repeat, for as long as
     * the process lives. It runs AFTER the tail above, so the tick that
     * stops it does not also jitter.
     */
    if (m->shake_world_on) {
        if (m->shake_world_delay > 0) {
            --m->shake_world_delay;
        } else {
            wm_arcade_und_finish_callbacks_t scb;
            memset(&scb, 0, sizeof(scb));
            scb.set_world_origin = match_set_world_origin;
            scb.rng = m->anim_rng;
            scb.user = m;
            wm_arcade_und_shake_world(m->shake_world_base_tlx,
                                      m->shake_world_base_tly, &scb);
            m->shake_world_delay = WM_UND_SHAKE_SLEEP - 1;
        }
    }

    /*
     * LIFEBAR.ASM:2642 announce_rnd_winner, started by a pin animation's
     * own win_announce. It ends the round on its own terms rather than
     * waiting for the KO countdown above, and once it has, that countdown
     * has nothing left to decide: annc_rnd_winner_done is exactly the
     * source's own "this round has already been called" flag.
     */
    {
        wm_arcade_round_announce_ctx_t arw;
        memset(&arw, 0, sizeof(arw));
        arw.pcnt = m->tick_count;
        arw.royal_rumble = m->royal_rumble;
        /* @in_finish_move, raised by TAKER.ASM's und_finish_move1 and
           cleared once @finish_completed lands. The announcer holds
           the round while the coffin sequence owns the screen. */
        arw.in_finish_move = m->in_finish_move;
        arw.score = &m->score;
        arw.user = m;
        arw.sound = match_arw_sound;
        arw.match_over = match_arw_match_over;
        if (wm_arcade_round_announce_tick(&m->round_announce, actor_ptrs,
                                          m->actor_count, &arw)) {
            /* #nobuck stamped round_end_time and awarded the round; the
               KO countdown must not award it a second time. */
            m->round_state.decided = true;
            m->round_state.pin_timeout = 0;
            m->round_state.decided_winner_side =
                m->round_announce.winner ? (int)m->round_announce.winner->player_side
                                         : -1;
        }
        /*
         * LIFEBAR.ASM:3071 WRESTLERS_RESET, which lives inside
         * announce_rnd_winner and nowhere else. The reset owed by the
         * KO countdown above is this port's stand-in for the rounds
         * the announcer never runs -- a clock timeout, a double KO --
         * and that stand-in was carrying every live round, because
         * nothing had ever reached win_announce in the match loop. It
         * is reached now, from a raise-arm animation's own ANI_CODE,
         * and the announcer owes its own reset: without this the
         * round it just awarded is the last one that ever starts.
         */
        if (m->round_announce.wrestlers_reset_due) {
            m->round_announce.wrestlers_reset_due = false;
            if (m->score.match_winner == 0) m->round_reset_pending = true;
        }
    }

    ++m->tick_count;
}
