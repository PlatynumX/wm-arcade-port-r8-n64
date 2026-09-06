#include "wm/match.h"
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
/* IF_SILENT_ADD_VOICE, reached from the animation VM. */
static void match_announce_if_silent(void *user, uint16_t call) {
    wm_match_state *m = (wm_match_state *)user;
    if (m) (void)wm_announcer_add_if_silent(&m->announcer, call);
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
    wm_arcade_round_state_init(&m->round_state);
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
    wm_arcade_round_state_init(&m->round_state);
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
            m->wrestler_visual[i].anim_env.announcer_user = m;
            m->wrestler_visual[i].anim_env.announce_if_silent =
                match_announce_if_silent;
            m->bret_visual[i].anim_env.announcer_user = m;
            m->bret_visual[i].anim_env.announce_if_silent =
                match_announce_if_silent;

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
    (void)wm_announcer_tick(&m->announcer, m->anim_sound_user, m->anim_sound,
                            NULL);

    {
        wm_arcade_react_callbacks_t react_cb;
        wm_arcade_react_bridge_t bridge;
        wm_arcade_combat_callbacks_t combat_cb;

        memset(&react_cb, 0, sizeof(react_cb));
        react_cb.adjust_health = wm_match_adjust_health;
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
    }

    {
        bool was_decided = m->round_state.decided;
        wm_arcade_round_tick(&m->round_state, actor_ptrs, m->actor_count);
        if (!was_decided && m->round_state.decided)
            wm_arcade_match_score_award_round(&m->score, m->round_state.decided_winner_side);
    }

    ++m->tick_count;
}
