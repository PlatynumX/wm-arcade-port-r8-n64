#include "wm/match.h"
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
 * quads. This port only ever creates one wrestler per side, so the "first
 * player on team" row (index 0) is the only one ever reachable -- no
 * team-battle/royal-rumble/INIT_LADDER_TABLE support exists here.
 * RING_X_CENTER (RING.EQU:22) = 0400h+50 = 1074.
 */
#define WM_MATCH_RING_X_CENTER 1074
#define WM_MATCH_P1_START_X (WM_MATCH_RING_X_CENTER - 85)
#define WM_MATCH_P1_START_Z (1127 + 93)
#define WM_MATCH_P1_START_FACING WM_MOVE_UP_RIGHT
#define WM_MATCH_P2_START_X (WM_MATCH_RING_X_CENTER + 85)
#define WM_MATCH_P2_START_Z (1103 + 93)
#define WM_MATCH_P2_START_FACING WM_MOVE_DOWN_LEFT


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

static bool match_anyone_near_death(void *user) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned i;
    if (!m) return false;
    for (i = 0; i < m->actor_count; ++i)
        if (m->actors[i].life < WM_ANNOUNCE_END_GAME_HEALTH) return true;
    return false;
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

void wm_match_init(wm_match_state *m) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
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

static void init_bret_backends(wm_match_state *m) {
    unsigned i;
    for (i = 0; i < WM_MATCH_MAX_ACTORS; ++i) {
        wm_bret_backend_init(&m->bret_visual[i]);
        /* LIFEBAR.ASM adjust_health's "attract mode never dies" rule
           (PSTATUS==0) -- see wm_arcade_adjust_health. Fixed for the whole
           match: neither start path changes has_human afterward. */
        m->bret_visual[i].attract_mode = !m->has_human;
        if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
            wm_arcade_bret_callbacks_t cb = wm_bret_backend_callbacks(&m->bret_visual[i]);
            wm_arcade_bret_ani_init(&m->actors[i], &cb);
        }
    }
}

void wm_match_start_attract(wm_match_state *m, WmRng *rng) {
    wm_arcade_actor_t *p1, *opp;
    if (!m) return;

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

    place_wrestler(p1, WM_MATCH_P1_START_X, WM_MATCH_P1_START_Z, WM_MATCH_P1_START_FACING);
    place_wrestler(opp, WM_MATCH_P2_START_X, WM_MATCH_P2_START_Z, WM_MATCH_P2_START_FACING);

    wm_arcade_drone_init(&m->drones[0], 0);
    wm_arcade_drone_init(&m->drones[1], 0);

    init_bret_backends(m);

    m->actor_count = WM_MATCH_MAX_ACTORS;
    m->active = true;
    m->tick_count = 0;
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
    wm_anim_code_reset();
    wm_arcade_round_state_init(&m->round_state);
    wm_arcade_round_announce_init(&m->round_announce);
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
}

void wm_match_start_selected(wm_match_state *m, WmRng *rng,
                             uint8_t p1_source_wrestler) {
    wm_arcade_actor_t *p1, *opp;
    if (!m) return;

    p1 = &m->actors[0];
    opp = &m->actors[1];
    init_actor_life(p1);
    init_actor_life(opp);

    /* WRESTLE.ASM:1713-1727 #1plyr: human, PLYRNUM=0, PSIDE_PLYR1, wrestler
       from @index1 (whatever the select screen actually chose). */
    p1->player_num = 0;
    p1->player_side = WM_MATCH_PSIDE_PLYR1;
    p1->wrestler_num = (int32_t)p1_source_wrestler;

    /* WRESTLE.ASM:1733-1741 #ndrone: placeholder opponent draw -- see
       wm/match.h. Not @index2/ladder-derived; PLYRNUM starts at 2. */
    m->opponent_wrestler = wm_match_draw_wrestler_index(rng);
    opp->player_num = 2;
    opp->player_side = WM_MATCH_PSIDE_PLYR2;
    opp->wrestler_num = (int32_t)m->opponent_wrestler;

    p1->smart_target = opp;
    opp->smart_target = p1;

    /* WRESTLE.ASM:2691-2755 (#set0, wrestler-placement init): see
       place_wrestler's own comment above wm_match_start_attract. */
    place_wrestler(p1, WM_MATCH_P1_START_X, WM_MATCH_P1_START_Z, WM_MATCH_P1_START_FACING);
    place_wrestler(opp, WM_MATCH_P2_START_X, WM_MATCH_P2_START_Z, WM_MATCH_P2_START_FACING);

    wm_arcade_drone_init(&m->drones[0], 0);
    wm_arcade_drone_init(&m->drones[1], 0);

    /* Set before init_bret_backends() so its attract_mode wiring
       (!has_human) sees the right value. */
    m->has_human = true;
    m->human_actor_index = 0;
    wm_human_input_init(&m->human_input_state);

    init_bret_backends(m);

    m->actor_count = WM_MATCH_MAX_ACTORS;
    m->active = true;
    m->tick_count = 0;
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
    wm_anim_code_reset();
    wm_arcade_round_state_init(&m->round_state);
    wm_arcade_round_announce_init(&m->round_announce);
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
}

/* wm_arcade_adjust_health's death_anim bridge for the generic hit path:
   unlike wm_bret_backend_adjust_health (which always IS a Bret actor), the
   hit path's victim could be either actor[], and only the one carrying
   WM_ROSTER_BRET has a real backend to dispatch through -- everyone else
   simply gets no death anim, same as this port's every other Bret-only
   boundary. */
static void wm_match_death_change_anim(wm_arcade_actor_t *victim,
                                       wm_arcade_react1_anim_group_t anim,
                                       void *user) {
    wm_match_state *m = (wm_match_state *)user;
    unsigned i;
    if (!m || anim != WM_R1_ANIM_FALL_BACK) return;
    for (i = 0; i < m->actor_count; ++i) {
        if (&m->actors[i] == victim) {
            if (m->actors[i].wrestler_num == WM_ROSTER_BRET) {
                wm_bret_backend_change_anim(victim, WM_BRET_ANIM_FALL_BACK,
                                            &m->bret_visual[i]);
            }
            return;
        }
    }
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
           branch -- WM_MATCH_MAX_ACTORS==2, so the other slot is always the
           opponent (same fixed-pair substitution wm_arcade_calc_closest
           below already makes). */
        wm_arcade_update_newfacing(&m->actors[i], &m->actors[1u - i]);

        /* WRESTLE.ASM's main loop computes each wrestler's own CLOSEST_*
           fields every tick for every process (drone_main's own AI
           decisions -- block detection, range-band script selection --
           read them exactly like move_bret does), not just for whichever
           actor happens to carry a real Bret backend. Previously this only
           ran inside the Bret branch below, so a drone-controlled actor's
           AI acted on the *previous* tick's stale distances; a drone-
           controlled Bret got fresh data for move_bret but stale data for
           its own drone_main decision that same tick. */
        wm_arcade_calc_closest(&m->actors[i], &m->actors[1u - i]);

        if (m->has_human && i == m->human_actor_index) {
            wm_human_input_commit(&m->actors[i], &m->human_input_state, human_input);
        } else {
            uint16_t old_but = m->drones[i].but;
            uint16_t old_joy = m->drones[i].joy;
            (void)wm_arcade_drone_main(&m->actors[i], &m->drones[i], &world, cb);
            wm_arcade_drone_commit_inputs(&m->actors[i], &m->drones[i], old_but, old_joy);
        }

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

        /* WRESTLE.ASM::move_wrestler dispatches every wrestler process
           through its own move_xxx; wm_arcade_move_ported_wrestler is that
           dispatcher, and all eight per-wrestler modules behind it are real
           direct ports. Bret is the one with a full visual backend
           (wm/bret_backend.h: real frame data, attack windows, hurt_box);
           the other seven run the same real decision logic through the
           shared, animation-free backend in wm/wrestler_backend.h. */
        {
            /* WM_MATCH_MAX_ACTORS==2: the other slot is always the opponent. */
            wm_arcade_actor_t *opp = &m->actors[1u - i];
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
            m->wrestler_visual[i].wrestler_num = m->actors[i].wrestler_num;
            m->wrestler_visual[i].anim_env.opponent = opp;
            m->wrestler_visual[i].anim_env.rng = m->anim_rng;
            m->wrestler_visual[i].anim_env.pcnt = m->tick_count;
            m->wrestler_visual[i].anim_env.sound_user = m->anim_sound_user;
            m->wrestler_visual[i].anim_env.sound = m->anim_sound;

            m->bret_visual[i].opponent = opp;
            m->bret_visual[i].pcnt = m->tick_count;
            m->bret_visual[i].anim_env.opponent = opp;
            m->bret_visual[i].anim_env.rng = m->anim_rng;
            m->bret_visual[i].anim_env.pcnt = m->tick_count;
            m->bret_visual[i].anim_env.sound_user = m->anim_sound_user;
            m->bret_visual[i].anim_env.sound = m->anim_sound;

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
             * actually is: no royal rumble, PSTATUS 0 for the attract
             * match and 1 for the single human, and exactly one opponent
             * because the ladder team draw is not translated. The
             * routines that gate on these read them rather than assuming.
             */
            m->wrestler_visual[i].anim_env.royal_rumble = false;
            m->wrestler_visual[i].anim_env.pstatus = m->has_human ? 1 : 0;
            m->wrestler_visual[i].anim_env.num_opps = 1;
            m->bret_visual[i].anim_env.royal_rumble = false;
            m->bret_visual[i].anim_env.pstatus = m->has_human ? 1 : 0;
            m->bret_visual[i].anim_env.num_opps = 1;
            m->wrestler_visual[i].anim_env.award_user = m->anim_award_user;
            m->wrestler_visual[i].anim_env.round_award = m->anim_round_award;
            m->bret_visual[i].anim_env.award_user = m->anim_award_user;
            m->bret_visual[i].anim_env.round_award = m->anim_round_award;
            m->wrestler_visual[i].anim_env.announcer = &m->announcer;
            m->wrestler_visual[i].anim_env.anyone_near_death =
                match_anyone_near_death;
            m->wrestler_visual[i].anim_env.announcer_user = m;
            m->bret_visual[i].anim_env.announcer = &m->announcer;
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
                wm_arcade_confine_wrestler(&m->actors[i]);
            } else {
                wm_wrestler_backend_tick(&m->wrestler_visual[i], &m->actors[i]);
                /* These six now have a real, moving hurt_box of their own
                   (their animations are program-driven), so confine_wrestler
                   applies to them exactly as it does to Bret -- it reads
                   OBJ_COLLX1/X2, which is what the hurt box is. */
                wm_arcade_confine_wrestler(&m->actors[i]);
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
        react_cb.user = m;

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
    }

    {
        bool was_decided = m->round_state.decided;
        wm_arcade_round_tick(&m->round_state, actor_ptrs, m->actor_count);
        if (!was_decided && m->round_state.decided)
            wm_arcade_match_score_award_round(&m->score, m->round_state.decided_winner_side);
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
        arw.royal_rumble = false;
        /* @in_finish_move: SPECIAL.ASM's finishing-move flag, which this
           port has no finishing-move sequence to set. */
        arw.in_finish_move = false;
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
    }

    ++m->tick_count;
}
