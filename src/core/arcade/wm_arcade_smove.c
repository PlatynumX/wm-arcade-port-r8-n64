/*
 * WRESTLE2.ASM:4058 init_smoves and the special-move watchdogs it
 * creates -- see wm/arcade/wm_arcade_smove.h.
 */
#include "wm/arcade/wm_arcade_smove.h"

#include <stddef.h>
#include <string.h>
#ifdef WM_SMOVE_DEBUG
#include <stdio.h>
#endif

#include "wm/wrestler_anim_tables.h"

/* ------------------------------------------------------------------ */
/* MACROS.H:652 WAITSWITCH_DWN                                        */

uint16_t wm_smove_switch_word(uint16_t but_val_down, uint16_t stick_rel_new) {
    /* `move *a8(BUT_VAL_DOWN),a0 / sll 4,a0 / move *a8(STICK_REL_NEW),a1
       / or a1,a0`. GAME.EQU's B_* constants carry the same shift, so
       the comparison is against this word directly. */
    return (uint16_t)(((uint16_t)(but_val_down << 4)) | stick_rel_new);
}

wm_smove_tick_t wm_smove_waitswitch(int32_t *countdown,
                                    uintptr_t special_move_addr,
                                    uint16_t but_val_down,
                                    uint16_t stick_rel_new,
                                    uint16_t switches,
                                    uint16_t mask) {
    uint16_t word;

    /* `dec a11 / jrz FAILADDR` -- decremented BEFORE the test, so a
       countdown of 0 wraps to -1 and never reaches zero. That is how
       every monitor's opening input gets no deadline. */
    if (countdown) {
        *countdown -= 1;
        if (*countdown == 0) return WM_SMOVE_FAIL;
    }

    /* "already doing a special move" -- he cannot be starting one. */
    if (special_move_addr != 0) return WM_SMOVE_FAIL;

    word = wm_smove_switch_word(but_val_down, stick_rel_new);
    word = (uint16_t)(word & (uint16_t)~mask);   /* `andni MASK,a0` */
    if (word == 0) return WM_SMOVE_WAIT;         /* `jrz lp?` */

    /* `cmpi SWITCHES,a0 / jrne FAILADDR` -- the wrong input RESTARTS
       the sequence, unlike AWARD.ASM's PUPWAITSWITCH, which ignores
       it. The source's own note is beside this line: "subk doesn't
       work -- some out-of-range values in doink." */
    return (word == switches) ? WM_SMOVE_MATCH : WM_SMOVE_FAIL;
}

/* ------------------------------------------------------------------ */
/* The three monitors this port can complete                          */

/*
 * TAKER.ASM:608 und_finish_move1. UP, then DOWN, then PUNCH, and the
 * seven guards behind it are wm_arcade_und_finish_allowed's.
 */
static bool und_finish_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                            wm_smove_fire_t *out) {
    wm_arcade_und_finish_env_t fe;
    const char *anim;

    if (!a || !env || !out) return false;

    memset(&fe, 0, sizeof fe);
    fe.my_pins = env->my_pins;
    fe.player_type = a->plyr_type;
    fe.ring_time = env->ring_time;
    fe.world_tlx = env->world_tlx;
    fe.world_tly = env->world_tly;

    anim = wm_arcade_und_finish_move1(a, env->victim, &fe, env->und_cb);
#ifdef WM_SMOVE_DEBUG
    fprintf(stderr, "guards: vmode=%u pins=%d ptype=%d rt=%d didpin=%d "
            "tx=%d vx=%d max=%d\n", env->victim?env->victim->player_mode:999,
            (int)env->my_pins, (int)a->plyr_type, (int)env->ring_time,
            (int)((a->status_flags & WM_STATUS_DID_PIN)!=0),
            (int)a->x_int, env->victim?(int)env->victim->x_int:-1,
            1174);
#endif
    if (!anim) return false;

    out->anim = anim;
    out->in_finish_move = true;
    return true;
}

/*
 * DOINK.ASM:1092 std_walk_fast. "One time per match", says the source
 * -- the gate is WALK_FAST already being zero, so once it is set the
 * monitor can never complete again even though it keeps running.
 */
static bool walk_fast_gate(const wm_arcade_actor_t *a,
                           const wm_smove_env_t *env) {
    (void)env;
    if (!a) return false;
    if (a->walk_fast != 0) return false;
    return a->player_mode == WM_PMODE_NORMAL;
}

static bool walk_fast_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                           wm_smove_fire_t *out) {
    (void)env;
    if (!a || !out) return false;
    /* The mode is tested a second time after the rotation completes:
       `move *a8(PLYRMODE),a0 / cmpi MODE_NORMAL,a0 / jrnz #lp0`. */
    if (a->player_mode != WM_PMODE_NORMAL) return false;
    out->walk_fast = WM_SMOVE_WALK_FAST_TIME;
    /*
     * What is NOT here, and is not a simplification but a deliberate
     * stop: the rest of std_walk_fast is presentation. Doink alone
     * gets a `movi 30000h,a0 / move a0,*a8(OBJ_YVEL)` hop; then
     * ADD_VOICE (211h for Doink, 17eh for everyone else), then three
     * OBJ_CONST/M_CONNON flashes three ticks apart, then Doink's three
     * honks, then a wait on WALK_FAST running out and two more
     * flashes. All of it is object control and audio, which this port
     * routes through its own seams rather than from inside a monitor.
     */
    return true;
}

/*
 * DOINK.ASM:1216 std_taunt. The same eight-way rotation, starting at
 * UP instead of AWAY and with BLOCK held throughout -- the mask is
 * B_BLOCK, which keeps the held button from failing each step.
 */
static bool taunt_gate(const wm_arcade_actor_t *a,
                       const wm_smove_env_t *env) {
    (void)env;
    return a && a->player_mode == WM_PMODE_BLOCK;
}

/* "move *a8(BUT_VAL_CUR),a14 / btst PLAYER_BLOCK_BIT,a14 / jrz #lp0",
   between the opening UP and the timeout. */
static bool taunt_mid_gate(const wm_arcade_actor_t *a,
                           const wm_smove_env_t *env) {
    (void)env;
    return a && (a->but_val_cur & WM_BTN_BLOCK) != 0;
}

static bool taunt_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                       wm_smove_fire_t *out) {
    const char *anim;

    if (!a || !env || !out) return false;

    /* The same two tests again after the rotation, then one more. */
    if (!(a->but_val_cur & WM_BTN_BLOCK)) return false;
    if (a->player_mode != WM_PMODE_BLOCK) return false;
    /* "no taunts if all opponents are dead." */
    if (env->victim && env->victim->player_mode == WM_PMODE_DEAD)
        return false;

    /* `FACETBL #taunt_tbl / calla change_anim1a` -- the same table
       do_taunt reads, which the port already carries. */
    anim = (a->wrestler_num >= 0 &&
            a->wrestler_num < WM_WRESTLER_ANIM_SLOTS)
         ? wm_wrestler_taunt_anims[a->wrestler_num] : NULL;
    if (!anim) return false;

    out->anim = anim;
    out->risk = (uint16_t)WM_SMOVE_TAUNT_RISK;
    return true;
}

/*
 * The head-hold family's fire(), shared by all 41 rows. Which row is
 * running is carried on the run itself, because the monitor
 * descriptor is built from the generated table rather than written
 * out here.
 */
static bool hdhold_fire(wm_arcade_actor_t *a, const wm_smove_env_t *env,
                        wm_smove_fire_t *out) {
    const wm_smove_hdhold_t *row = (const wm_smove_hdhold_t *)env->hdhold_row;
    wm_arcade_actor_t *victim = NULL;
    if (!row) return false;
    return wm_smove_hdhold_fire(row, a, &victim, out) != WM_SMOVE_HH_NOTHING;
}

/*
 * The gate: live only while somebody has a head hold, either way
 * round. `move *a8(PLYRMODE),a0 / cmpi MODE_HEADHOLD,a0 / jrz #cont /
 * cmpi MODE_HEADHELD,a0 / jrnz #lp0`.
 */
static bool hdhold_gate(const wm_arcade_actor_t *a,
                        const wm_smove_env_t *env) {
    const wm_smove_hdhold_t *row;
    if (!a || !env) return false;
    if (a->player_mode != WM_PMODE_HEADHOLD &&
        a->player_mode != WM_PMODE_HEADHELD)
        return false;
    row = (const wm_smove_hdhold_t *)env->hdhold_row;
    if (!row) return true;
    /* `move *a8(GETUP_TIME),a0 / jrnz #lp0`. */
    if (row->needs_getup_clear && a->getup_time != 0) return false;
    /* `calla CHECK_COMBO_GO / jrlt #lp0` -- a signed less-than, so a
       negative answer refuses and zero does not. The combo state is
       the actor's own combo_count, which is what CHECK_COMBO_GO
       reads. */
    if (row->needs_combo && a->combo_count <= 0) return false;
    return true;
}

/*
 * One monitor descriptor per generated row, built once. They are not
 * written out because they differ only in the six columns the
 * extractor read; building them here keeps the source data and the
 * runtime shape in one place.
 */
static wm_smove_monitor_t HDHOLD[64];
static size_t hdhold_built;

static void build_hdhold(void) {
    size_t i, n;
    if (hdhold_built) return;
    n = wm_smove_hdhold_count;
    if (n > sizeof HDHOLD / sizeof HDHOLD[0])
        n = sizeof HDHOLD / sizeof HDHOLD[0];
    for (i = 0; i < n; ++i) {
        const wm_smove_hdhold_t *r = &wm_smove_hdhold[i];
        wm_smove_monitor_t *m = &HDHOLD[i];
        memset(m, 0, sizeof(*m));
        m->name = r->name;
        m->file = r->file;
        m->step[0] = r->step[0];
        m->step[1] = r->step[1];
        m->step[2] = r->step[2];
        m->steps = 3;
        m->timeout = r->timeout;
        m->gate = hdhold_gate;
        m->fire = hdhold_fire;
        /* `SLEEPK 20 / jruc #lp` -- it goes round again rather than
           dying, so a head hold can be turned into move after move. */
        m->one_shot = false;
        m->hdhold = r;
    }
    hdhold_built = n;
}

/*
 * The registry. A monitor is here because its whole body is
 * translated; the entries that resolve to NULL are what makes the
 * remaining gap countable.
 */
static const wm_smove_monitor_t MONITORS[] = {
    {
        "und_finish_move1", "TAKER.ASM",
        { { WM_J_UP, 0 }, { WM_J_DOWN, 0 }, { WM_B_PUNCH, WM_J_ALL } },
        3, WM_SMOVE_TIMEOUT_FINISH,
        false, NULL, NULL, und_finish_fire,
        true,                      /* `#fi1_exit DIE` */
        NULL                       /* not a head-hold row */
    },
    {
        "std_walk_fast", "DOINK.ASM",
        { { WM_J_AWAY, 0 }, { WM_J_DOWN_AWAY, 0 }, { WM_J_DOWN, 0 },
          { WM_J_DOWN_TOWARD, 0 }, { WM_J_TOWARD, 0 },
          { WM_J_UP_TOWARD, 0 }, { WM_J_UP, 0 }, { WM_J_UP_AWAY, 0 } },
        8, WM_SMOVE_TIMEOUT_DOINK,
        false, walk_fast_gate, NULL, walk_fast_fire,
        true,                      /* the tail ends in DIE */
        NULL
    },
    {
        "std_taunt", "DOINK.ASM",
        { { WM_J_UP, WM_B_BLOCK }, { WM_J_UP_TOWARD, WM_B_BLOCK },
          { WM_J_TOWARD, WM_B_BLOCK }, { WM_J_DOWN_TOWARD, WM_B_BLOCK },
          { WM_J_DOWN, WM_B_BLOCK }, { WM_J_DOWN_AWAY, WM_B_BLOCK },
          { WM_J_AWAY, WM_B_BLOCK }, { WM_J_UP_AWAY, WM_B_BLOCK } },
        8, WM_SMOVE_TIMEOUT_DOINK,
        true,                      /* `PLYR_TYPE != 0 -> SUCIDE` */
        taunt_gate, taunt_mid_gate, taunt_fire,
        true,                      /* DIE after setting RISK */
        NULL
    },
};

#define MONITOR_COUNT (sizeof MONITORS / sizeof MONITORS[0])

const wm_smove_monitor_t *wm_smove_monitor_find(const char *name) {
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < MONITOR_COUNT; ++i)
        if (strcmp(MONITORS[i].name, name) == 0) return &MONITORS[i];
    build_hdhold();
    for (i = 0; i < hdhold_built; ++i)
        if (strcmp(HDHOLD[i].name, name) == 0) return &HDHOLD[i];
    return NULL;
}

size_t wm_smove_monitor_count(void) {
    build_hdhold();
    return MONITOR_COUNT + hdhold_built;
}

const wm_smove_monitor_t *wm_smove_monitor_at(size_t i) {
    if (i < MONITOR_COUNT) return &MONITORS[i];
    build_hdhold();
    i -= MONITOR_COUNT;
    return (i < hdhold_built) ? &HDHOLD[i] : NULL;
}

/* ------------------------------------------------------------------ */
/* init_smoves / reset_smoves / kill_smove_procs                      */

size_t wm_smove_init(int32_t wrestler_num, bool is_drone,
                     wm_smove_run_t *runs, size_t capacity,
                     size_t *unported) {
    const wm_wrestler_smove_table *tbl;
    size_t made = 0, missing = 0, i;

    if (unported) *unported = 0;
    if (!runs || capacity == 0) return 0;

    /* `move *a13(WRESTLERNUM),a2,W / X32 a2 / addi #special_moves,a2 /
       move *a2,a2,L / jrz #done` -- a zero table pointer means this
       wrestler has no watchdogs at all. */
    if (wrestler_num < 0 || wrestler_num >= WM_WRESTLER_ANIM_SLOTS)
        return 0;
    tbl = &wm_wrestler_smoves[wrestler_num];
    if (!tbl->labels || tbl->count <= 0) return 0;

    for (i = 0; i < (size_t)tbl->count && made < capacity; ++i) {
        const wm_smove_monitor_t *m = wm_smove_monitor_find(tbl->labels[i]);
        if (!m) { ++missing; continue; }
        /* std_taunt's first two instructions: a drone kills itself
           rather than watching for the code. The process IS created --
           it just does not survive its own first tick -- so this skips
           it here and counts it as made-and-gone rather than absent. */
        if (m->humans_only && is_drone) continue;
        memset(&runs[made], 0, sizeof(runs[made]));
        runs[made].monitor = m;
        ++made;
    }

    if (unported) *unported = missing;
    return made;
}

void wm_smove_reset(wm_smove_run_t *runs, size_t count) {
    size_t i;
    if (!runs) return;
    /* "writing their SM_RESET_ADDRESSes to their PWAKEs, and setting
       their PTIMEs to 1" -- back to the routine's entry, awake next
       tick. A monitor that DIEd is gone from ACTIVE and is NOT
       revived, so `dead` survives a reset. */
    for (i = 0; i < count; ++i) {
        if (runs[i].dead) continue;
        runs[i].at = 0;
        runs[i].countdown = 0;
        runs[i].armed = false;
    }
}

void wm_smove_kill(wm_smove_run_t *runs, size_t count) {
    size_t i;
    if (!runs) return;
    for (i = 0; i < count; ++i) runs[i].dead = true;
}

bool wm_smove_tick(wm_smove_run_t *run, wm_arcade_actor_t *a,
                   const wm_smove_env_t *env, wm_smove_fire_t *out) {
    const wm_smove_monitor_t *m;
    wm_smove_tick_t r;

    if (out) memset(out, 0, sizeof(*out));
    if (!run || !run->monitor || run->dead || !a || !env) return false;
    m = run->monitor;

    /*
     * `#lp0: SLEEPK 1 / #lp: <gate> / jrnz #lp0`. The gate is re-tested
     * every pass before the sequence starts, and a failure inside the
     * sequence lands back on `#lp`, which re-tests it too.
     */
    if (!run->armed) {
        /* A head-hold monitor's gate reads its own row's two extra
           conditions, so it needs the row the same way fire() does. */
        if (m->gate) {
            wm_smove_env_t g = *env;
            g.hdhold_row = m->hdhold;
            if (!m->gate(a, &g)) return false;
        }
        run->at = 0;
        run->countdown = 0;      /* `clr a11` -- no deadline on step 0 */
        run->armed = true;
        return false;            /* the SLEEPK before the first wait */
    }

    r = wm_smove_waitswitch(&run->countdown, a->special_move_addr,
                            a->but_val_down, a->stick_rel_new,
                            m->step[run->at].switches,
                            m->step[run->at].mask);
    if (r == WM_SMOVE_WAIT) return false;
    if (r == WM_SMOVE_FAIL) { run->armed = false; return false; }

    run->at += 1;

    /* The extra test between step 0 and the timeout, and then the one
       `movi #TIMEOUT,a11` the whole rest of the sequence shares. */
    if (run->at == 1) {
        if (m->mid_gate && !m->mid_gate(a, env)) {
            run->armed = false;
            return false;
        }
        run->countdown = m->timeout;
    }

    if (run->at < m->steps) return false;

    run->armed = false;
    if (!m->fire) return false;
    if (m->hdhold) {
        /* One fire() serves all 41 generated rows; this is how it
           learns which one is running. */
        wm_smove_env_t local = *env;
        local.hdhold_row = m->hdhold;
        if (!m->fire(a, &local, out)) return false;
    } else if (!m->fire(a, env, out)) {
        return false;
    }
    if (m->one_shot) run->dead = true;
    return true;
}

/* ------------------------------------------------------------------ */
/* The head-hold family's shared tail                                 */

wm_smove_hh_result_t wm_smove_hdhold_fire(const wm_smove_hdhold_t *row,
                                          wm_arcade_actor_t *a,
                                          wm_arcade_actor_t **victim,
                                          wm_smove_fire_t *out) {
    wm_arcade_actor_t *target;
    bool reversing;

    if (victim) *victim = NULL;
    if (out) memset(out, 0, sizeof(*out));
    if (!row || !a) return WM_SMOVE_HH_NOTHING;

    /*
     * `move *a8(PLYRMODE),a0 / cmpi MODE_HEADHOLD,a0 / jrz #slam /
     * cmpi MODE_HEADHELD,a0 / jrnz #lp0`. Which mode he is in at THIS
     * moment decides whose move it is -- not who started the hold.
     */
    if (a->player_mode == WM_PMODE_HEADHOLD) {
        reversing = false;
    } else if (a->player_mode == WM_PMODE_HEADHELD) {
        /* The reversal path, and only if this move has one. */
        if (!row->reversal) return WM_SMOVE_HH_NOTHING;
        /* `move *a8(I_WILL_DIE),A14 / jrnz #lp0` -- a man already on
           his way out cannot reverse. The slam path has no such test;
           it is on the reversal side only. */
        if (a->i_will_die) return WM_SMOVE_HH_NOTHING;
        reversing = true;
    } else {
        return WM_SMOVE_HH_NOTHING;
    }

    /* `move *a8(IMMOBILIZE_TIME),a14 / jrnz #lp0 ;ignore` -- on both
       paths, and the source's own comment is that one word. */
    if (a->immobilize_time != 0) return WM_SMOVE_HH_NOTHING;

    /* "target WHOHITME -- don't hit anyone else" on the reversal,
       WHOIHIT on the slam. SMRTTGT is the smart-target write. */
    target = reversing ? a->who_hit_me : a->who_i_hit;
    if (victim) *victim = target;
    a->smart_target = target;

    if (out) {
        out->anim = row->anim;
        /*
         * FACE24 picks between the `_2_` and `_4_` forms by facing.
         * PLAYER_RIGHT_BIT is WM_MOVE_RIGHT, the same test
         * read_switches uses for the relative stick.
         */
        if (row->anim_flipped && !(a->facing_dir & WM_MOVE_RIGHT))
            out->anim = row->anim_flipped;
        /* The slam's bonus message; a reversal gets DO_REVERSAL_MESS
           instead, which is a different message and not this one. */
        out->bonus = reversing ? -1 : row->bonus;
        out->victim_immobilize = WM_SMOVE_HH_VICTIM_IMMOBILIZE;
    }

    return reversing ? WM_SMOVE_HH_REVERSAL : WM_SMOVE_HH_SLAM;
}
