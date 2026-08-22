#include "wm_fix39_runtime.h"
#include "wm_arcade_movement.h"

#include "wm_arcade_attach_anim.h"
#include "wm_arcade_bret.h"
#include "wm_arcade_move_dispatch.h"
#include "wm_arcade_drone_source_tables.h"
#include "wm_arcade_drone_source_ranges.h"
#include "wm_arcade_drone_source_scripts.h"
#include "wm_arcade_drone_source_services.h"
#include "wm_arcade_drone_source_bodies.h"
#include "wm_arcade_razor.h"
#include "wm_arcade_react.h"
#include "wm_arcade_special.h"
#include "wm_arcade_wrestler_port.h"
#include "wmania_hiscore_counter.h"
#include "wmania_hiscore_system.h"
#include "wmania_rng.h"
#include "wmania_rope_runtime.h"
#include "wmania_rope_source_data.h"

#include <limits.h>
#include <string.h>

#define WM_FIX39_ACTOR_COUNT 2u
#define WM_FIX39_ATTRACT_MAX_STEPS 32u
#define WM_FIX39_SPECIAL_SLOTS 8u

/* WRESTLE.ASM reset_start, first team1_starts/team2_starts entries. */
#define WM_FIX39_P1_START_X (WM_RING_X_CENTER - 85)
#define WM_FIX39_P1_START_Z (1127 + 93)
#define WM_FIX39_P1_FACING  9
#define WM_FIX39_P2_START_X (WM_RING_X_CENTER + 85)
#define WM_FIX39_P2_START_Z (1103 + 93)
#define WM_FIX39_P2_FACING  6

static struct {
    WmFix39Status status;
    WmRng rng;
    WmHsSystem hiscore;
    WmAttractState attract;
    WmAttractStep attract_steps[WM_FIX39_ATTRACT_MAX_STEPS];
    WmAttractLive attract_live;
    uint32_t attract_platform_capabilities;
    WmRopeRuntimeBank ropes[4];
    wm_arcade_actor_t actors[WM_FIX39_ACTOR_COUNT];
    wm_arcade_actor_t *actor_ptrs[WM_FIX39_ACTOR_COUNT];
    WmFix39ActorTrace trace[WM_FIX39_ACTOR_COUNT];
    wm_arcade_combat_runtime_t combat_runtime;
    wm_arcade_frame_box_t frame_box[WM_FIX39_ACTOR_COUNT];
    bool frame_box_valid[WM_FIX39_ACTOR_COUNT];
    wm_arcade_react_callbacks_t react_callbacks;
    wm_arcade_react_bridge_t react_bridge;
    wm_arcade_combat_callbacks_t combat_callbacks;
    wm_arcade_drone_callbacks_t drone_callbacks;
    wm_arcade_drone_state_t drone_state[WM_FIX39_ACTOR_COUNT];
    wm_arcade_special_lists_t special_lists;
    wm_arcade_special_obj_t special_pool[WM_FIX39_SPECIAL_SLOTS];
    uint16_t old_p1_buttons;
    uint16_t old_p1_stick;
    bool match_cpu_vs_cpu;
    char recent_initials[2][WM_HS_NUM_INITIALS + 1u];
} g;

static int32_t fixed16(int32_t v)
{
    return v << 16;
}

static int32_t abs32(int32_t v)
{
    if (v == INT32_MIN) return INT32_MAX;
    return v < 0 ? -v : v;
}

static int frontend_to_arcade(unsigned id)
{
    /* include/wm/roster.h order in the current N64 frontend. */
    static const int map[8] = {
        WM_ROSTER_BRET,   /* Bret */
        WM_ROSTER_BAM,    /* Bam Bam */
        WM_ROSTER_YOKO,   /* Yokozuna */
        WM_ROSTER_DOINK,  /* Doink */
        WM_ROSTER_RAZOR,  /* Razor */
        WM_ROSTER_LEX,    /* Lex */
        WM_ROSTER_TAKER,  /* Undertaker */
        WM_ROSTER_SHAWN   /* Shawn */
    };
    return id < 8u ? map[id] : -1;
}

static int actor_index(const wm_arcade_actor_t *a)
{
    if (a == &g.actors[0]) return 0;
    if (a == &g.actors[1]) return 1;
    return -1;
}

static WmFix39ActorTrace *trace_for(wm_arcade_actor_t *a)
{
    int i = actor_index(a);
    return i >= 0 ? &g.trace[i] : 0;
}

static uint16_t stick_direction(int8_t x, int8_t y)
{
    /* The N64 platform already applies its physical-controller deadzone. */
    const int threshold = 12;
    const bool left = x < -threshold;
    const bool right = x > threshold;
    const bool up = y > threshold;
    const bool down = y < -threshold;

    if (up && left) return WM_MOVE_UP_LEFT;
    if (up && right) return WM_MOVE_UP_RIGHT;
    if (down && left) return WM_MOVE_DOWN_LEFT;
    if (down && right) return WM_MOVE_DOWN_RIGHT;
    if (up) return WM_MOVE_UP;
    if (down) return WM_MOVE_DOWN;
    if (left) return WM_MOVE_LEFT;
    if (right) return WM_MOVE_RIGHT;
    return WM_MOVE_ZIP;
}

static uint16_t button_bits(bool light_punch,
                            bool power_punch,
                            bool light_kick,
                            bool power_kick,
                            bool block)
{
    uint16_t v = 0u;
    if (light_punch) v |= WM_BTN_PUNCH;
    if (block)       v |= WM_BTN_BLOCK;
    if (power_punch) v |= WM_BTN_SPUNCH;
    if (light_kick)  v |= WM_BTN_KICK;
    if (power_kick)  v |= WM_BTN_SKICK;
    return v;
}

static int live_good_run_hit(wm_arcade_actor_t *attacker,
                             wm_arcade_actor_t *victim,
                             void *user)
{
    (void)attacker;
    (void)victim;
    (void)user;
    /* The exact WRESTLE good_run_hit world/velocity predicate is not bound yet.
       Reject run-only hits instead of fabricating it. */
    return 0;
}

static void live_adjust_health(wm_arcade_actor_t *victim,
                               int16_t signed_delta,
                               wm_arcade_actor_t *damage_source,
                               void *user)
{
    int32_t next;
    (void)damage_source;
    (void)user;
    if (!victim) return;
    next = victim->life + signed_delta;
    if (next < 0) next = 0;
    if (next > 100) next = 100;
    victim->life = next;
}

static void live_wrestler_hit(wm_arcade_actor_t *attacker,
                              wm_arcade_actor_t *victim,
                              void *user)
{
    (void)user;
    wm_arcade_wrestler_hit_collision_callback(attacker, victim, &g.react_bridge);
}

static void refresh_distances(void)
{
    wm_arcade_actor_t *a = &g.actors[0];
    wm_arcade_actor_t *b = &g.actors[1];
    int32_t dx = b->x_int - a->x_int;
    int32_t dy = b->y_int - a->y_int;
    int32_t dz = b->z_int - a->z_int;
    int32_t ax = abs32(dx);
    int32_t ay = abs32(dy);
    int32_t az = abs32(dz);
    int32_t d = ax;
    if (ay > d) d = ay;
    if (az > d) d = az;

    a->closest_xdist = ax;
    a->closest_ydist = ay;
    a->closest_zdist = az;
    a->closest_dist = d;
    b->closest_xdist = ax;
    b->closest_ydist = ay;
    b->closest_zdist = az;
    b->closest_dist = d;
}

static void init_actor(wm_arcade_actor_t *a,
                       int player_num,
                       int side,
                       int wrestler_num,
                       int32_t x,
                       int32_t z,
                       int facing)
{
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->player_num = player_num;
    a->player_side = side;
    a->wrestler_num = wrestler_num;
    a->x_int = x;
    a->y_int = 0;
    a->z_int = z;
    a->x_fixed = fixed16(x);
    a->y_fixed = 0;
    a->z_fixed = fixed16(z);
    a->ground_y = WM_MAT_Y;
    a->gravity = WM_ARCADE_GRAVITY;
    a->obj_friction = 0; /* animation/object backend supplies OBJ_FRICTION */
    a->priority = 112;
    a->climbing_thru = 0;
    a->life = 100;
    a->player_mode = WM_PMODE_NORMAL;
    a->anim_mode = 0u;
    a->in_ring = 0;
    a->facing_dir = facing;
    a->new_facing_dir = facing;
}

/* ------------------------------------------------------------------------- */
/* Source callback surfaces that are complete in the supplied ZIPs.         */
/* ------------------------------------------------------------------------- */

static void common_anim_label(wm_arcade_actor_t *a, const char *label, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->animation_label = label;
    ++t->animation_events;
}

static void common_torso_label(wm_arcade_actor_t *a, const char *label, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->torso_animation_label = label;
    ++t->animation_events;
}

static void common_sound_label(wm_arcade_actor_t *a, const char *label, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->sound_label = label;
    ++t->sound_events;
}

static void common_start_special_label(wm_arcade_actor_t *a, const char *label, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->external_special_label = label;
    ++t->external_special_events;
    /* Do not fabricate resolve_label_token: source animation/script pointer
       resolution is a separate shared arcade service still not supplied. */
}

static void source_keep_attached(wm_arcade_actor_t *a, void *user)
{
    (void)user;
    (void)wm_arcade_keep_attached(a);
    ++g.status.attachment_service_calls;
}

static void source_master_keep_attached(wm_arcade_actor_t *a, void *user)
{
    (void)user;
    (void)wm_arcade_master_keep_attached(a);
    ++g.status.attachment_service_calls;
}

static void bret_anim(wm_arcade_actor_t *a, wm_arcade_bret_anim_id_t id, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->animation_token = (int32_t)id;
    ++t->animation_events;
}

static void bret_torso(wm_arcade_actor_t *a, wm_arcade_bret_anim_id_t id, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->torso_animation_token = (int32_t)id;
    ++t->animation_events;
}

static void bret_sound(wm_arcade_actor_t *a, wm_arcade_bret_sound_id_t id, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->sound_token = (int32_t)id;
    ++t->sound_events;
}

static void razor_anim(wm_arcade_actor_t *a, wm_arcade_razor_anim_id_t id, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->animation_token = (int32_t)id;
    ++t->animation_events;
}

static void razor_torso(wm_arcade_actor_t *a, wm_arcade_razor_anim_id_t id, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->torso_animation_token = (int32_t)id;
    ++t->animation_events;
}

static void razor_sound(wm_arcade_actor_t *a, wm_arcade_razor_sound_id_t id, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->sound_token = (int32_t)id;
    ++t->sound_events;
}

static const wm_arcade_roster_callbacks_t common_callbacks = {
    .change_anim_label = common_anim_label,
    .change_torso_label = common_torso_label,
    .sound_label = common_sound_label,
    .master_keep_attached = source_master_keep_attached,
    .keep_attached = source_keep_attached,
    .start_special_label = common_start_special_label,
    .user = 0
};

static const wm_arcade_bret_callbacks_t bret_callbacks = {
    .change_anim = bret_anim,
    .change_torso_anim = bret_torso,
    .sound = bret_sound,
    .master_keep_attached = source_master_keep_attached,
    .keep_attached = source_keep_attached,
    .user = 0
};

static const wm_arcade_razor_callbacks_t razor_callbacks = {
    .change_anim = razor_anim,
    .change_torso_anim = razor_torso,
    .sound = razor_sound,
    .master_keep_attached = source_master_keep_attached,
    .keep_attached = source_keep_attached,
    .user = 0
};

static const wm_arcade_wrestler_port_bindings_t wrestler_bindings = {
    .bret = &bret_callbacks,
    .razor = &razor_callbacks,
    .taker = &common_callbacks,
    .yoko = &common_callbacks,
    .shawn = &common_callbacks,
    .bam = &common_callbacks,
    .doink = &common_callbacks,
    .lex = &common_callbacks
};

static void queued_special_token(wm_arcade_actor_t *a, uintptr_t token, void *user)
{
    WmFix39ActorTrace *t = trace_for(a);
    (void)user;
    if (!t) return;
    t->animation_token = (int32_t)token;
    ++t->animation_events;
}

typedef struct {
    const wm_arcade_roster_env_t *env;
} WmFix39DispatchContext;

static void live_character_move(wm_arcade_actor_t *a,
                                wm_arcade_move_handler_id_t which,
                                void *user)
{
    WmFix39DispatchContext *ctx = (WmFix39DispatchContext *)user;
    const wm_arcade_wrestler_profile_t *profile;
    wm_arcade_actor_t *opp;
    WmFix39ActorTrace *t;
    int index = actor_index(a);
    (void)which;
    if (index < 0) return;

    profile = wm_arcade_roster_profile((wm_arcade_roster_id_t)a->wrestler_num);
    opp = &g.actors[index ^ 1];
    t = &g.trace[index];
    t->last_character_step = wm_arcade_move_ported_wrestler(
        profile, a, opp, ctx ? ctx->env : 0, &wrestler_bindings);
}

static void init_source_character_animation(wm_arcade_actor_t *a)
{
    if (!a) return;
    if (a->wrestler_num == WM_ROSTER_BRET)
        wm_arcade_bret_ani_init(a, &bret_callbacks);
    else if (a->wrestler_num == WM_ROSTER_RAZOR)
        wm_arcade_razor_ani_init(a, &razor_callbacks);
    /* The other six source files do not expose a separate ani_init entry. */
}

static uint32_t drone_rndrng0_upto(uint32_t max_inclusive, void *user)
{
    return wm_rng_rndrng0((WmRng *)user, max_inclusive);
}

static uint32_t drone_rnd_upto(uint32_t mask, void *user)
{
    return wm_rng_rnd_mask((WmRng *)user, mask);
}

static wm_arcade_actor_t *drone_closest_actor(wm_arcade_actor_t *actor, void *user)
{
    (void)user;
    return actor ? actor->smart_target : 0;
}

static int32_t drone_closest_dist_for(const wm_arcade_actor_t *actor, void *user)
{
    (void)user;
    return actor ? actor->closest_dist : INT32_MAX;
}

static void drone_seek_dir_dist(wm_arcade_actor_t *actor, wm_arcade_drone_state_t *d, void *user)
{
    wm_arcade_actor_t *opp;
    int32_t dx, dz;
    uint16_t dir = WM_MOVE_ZIP;
    (void)user;
    if (!actor || !d) return;
    opp = actor->smart_target;
    if (!opp) { d->joy = WM_MOVE_ZIP; return; }
    dx = opp->x_int - actor->x_int;
    dz = opp->z_int - actor->z_int;
    /* DRN_SEEKDIR 4/12 is initialized from which X side the drone occupies.
       Preserve that source side choice while steering on the live X/Z plane. */
    if (dx > 8) dir |= WM_MOVE_RIGHT;
    else if (dx < -8) dir |= WM_MOVE_LEFT;
    if (dz > 8) dir |= WM_MOVE_DOWN;
    else if (dz < -8) dir |= WM_MOVE_UP;
    if (d->mode == -3) {
        /* Hang-back reverses the horizontal component selected by source. */
        if (dir & WM_MOVE_LEFT) dir = (uint16_t)((dir & ~WM_MOVE_LEFT) | WM_MOVE_RIGHT);
        else if (dir & WM_MOVE_RIGHT) dir = (uint16_t)((dir & ~WM_MOVE_RIGHT) | WM_MOVE_LEFT);
    }
    d->joy = dir;
}

static int drone_script_seek(wm_arcade_actor_t *self, wm_arcade_drone_state_t *d, void *user)
{
    int32_t threshold;
    (void)user;
    if (!self || !d || !self->smart_target) return 1;
    drone_seek_dir_dist(self, d, user);
    /* Source DRN_SEEKDIST is a small distance class, not a raw world unit.
       The direct C core uses the same class to terminate seek commands. */
    threshold = d->seek_dist <= 0 ? 32 : d->seek_dist * 32;
    return self->closest_dist <= threshold ? 0 : 1;
}

static int drone_check_combo_go(wm_arcade_actor_t *actor, void *user)
{
    (void)actor; (void)user;
    /* No fabricated combo rejection: non-negative follows DRONE.ASM's
       normal path into the source script selector. */
    return 0;
}

static void init_drone_callbacks(void)
{
    wm_arcade_drone_source_service_reset_handlers();
    (void)wm_arcade_drone_source_install_generated_bodies();
    memset(&g.drone_callbacks, 0, sizeof(g.drone_callbacks));
    g.drone_callbacks.rnd_upto = drone_rnd_upto;
    g.drone_callbacks.rndrng0_upto = drone_rndrng0_upto;
    g.drone_callbacks.closest_actor = drone_closest_actor;
    g.drone_callbacks.closest_actor_for = drone_closest_actor;
    g.drone_callbacks.closest_dist_for = drone_closest_dist_for;
    g.drone_callbacks.block_base_pct = wm_arcade_drone_source_block_base_pct;
    g.drone_callbacks.block_attack_pct = wm_arcade_drone_source_block_attack_pct;
    g.drone_callbacks.headhold_delay_max = wm_arcade_drone_source_headhold_delay_max;
    g.drone_callbacks.headheld_delay_max = wm_arcade_drone_source_headheld_delay_max;
    g.drone_callbacks.check_combo_go = drone_check_combo_go;
    g.drone_callbacks.seek_dir_dist = drone_seek_dir_dist;
    g.drone_callbacks.range_script_list = wm_arcade_drone_source_range_script_list;
    g.drone_callbacks.resolve_script = wm_arcade_drone_source_resolve_script;
    g.drone_callbacks.script_skill_pct = wm_arcade_drone_source_script_skill_pct;
    g.drone_callbacks.script_seek = drone_script_seek;
    g.drone_callbacks.script_call = wm_arcade_drone_source_service_dispatch;
    g.drone_callbacks.user = &g.rng;

    g.status.drone_rndrng0_ready = true;
    g.status.drone_scalar_tables_ready = wm_arcade_drone_source_tables_ready();
    g.status.drone_range_tables_ready = wm_arcade_drone_source_ranges_ready();
    /* DRONE.ASM calls both rnd and RNDRNG0.  Do not alias the still-unrecovered
       plain rnd service to RNDRNG0 merely because their callback shapes match. */
    g.status.drone_plain_rnd_ready = true;
    g.status.drone_scripts_ready = wm_arcade_drone_source_scripts_ready();
    g.status.drone_runtime_ready = g.status.drone_scalar_tables_ready &&
        g.status.drone_rndrng0_ready && g.status.drone_plain_rnd_ready &&
        g.status.drone_range_tables_ready && g.status.drone_scripts_ready;
}

void wm_fix39_runtime_init(void)
{
    unsigned i;

    memset(&g, 0, sizeof(g));

    /* RAND is uninitialized/BSS state in the arcade program; reset baseline 0. */
    wm_rng_init(&g.rng, 0u, 0, 0, 0);
    init_drone_callbacks();

    /* Factory HSTD tables are source data. Counter use waits for GET_ADJ value. */
    wm_hs_system_init(&g.hiscore, 0u);
    g.status.hiscore_tables_valid = wm_hs_system_table_cmos_check(&g.hiscore);

    wm_attract_init(&g.attract);
    wm_attract_live_reset(&g.attract_live);
    g.attract_platform_capabilities = 0u;
    g.status.attract_step_count = 0u;
    g.status.attract_platform_capabilities = 0u;

    for (i = 0u; i < 4u; ++i)
        wm_rope_runtime_init_bank(&g.ropes[i], (WmRopeBank)i, false);

    wm_arcade_combat_runtime_init(&g.combat_runtime);
    memset(&g.react_callbacks, 0, sizeof(g.react_callbacks));
    g.react_callbacks.good_run_hit = live_good_run_hit;
    g.react_callbacks.adjust_health = live_adjust_health;
    g.react_bridge.runtime = &g.combat_runtime;
    g.react_bridge.callbacks = &g.react_callbacks;
    memset(&g.combat_callbacks, 0, sizeof(g.combat_callbacks));
    g.combat_callbacks.wrestler_hit = live_wrestler_hit;
    wm_arcade_special_lists_init(&g.special_lists);
    for (i = 0u; i < WM_FIX39_SPECIAL_SLOTS; ++i)
        wm_arcade_special_obj_init(&g.special_pool[i]);

    /* These are deliberately false until the corresponding direct source
       service/adapter is supplied.  They prevent "wired" from meaning guessed. */
    g.status.movement_integrator_ready = true;
    g.status.animation_backend_ready = false;
    g.status.collision_boxes_ready = false;
    g.status.camera_onscreen_inputs_ready = false;
    g.status.ring_line_services_ready = true;
    g.status.secret_input_scheduler_ready = false;
    g.status.health_service_ready = true;
    g.status.audio_label_service_ready = false;
    g.status.special_spawn_command_service_ready = false;
    g.status.ringout_operator_state_ready = false;
    g.status.rope_renderer_ready = false;
    g.status.hiscore_persistence_ready = false;
    g.status.attract_renderer_ready = false;
    g.status.attract_demo_setup_ready = true;
    g.status.postmatch_router_ready = true;
    g.status.story_plan_ready = true;
    g.status.fireworks_plan_ready = true;

    g.status.initialized = true;
}

void wm_fix39_rng_set_entropy(uint32_t hcount, uint32_t sp_value)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_rng_set_latched_inputs(&g.rng, hcount, sp_value);
}

uint32_t wm_fix39_mainloop_step(uint32_t hcount, uint32_t sp_value)
{
    wm_fix39_rng_set_entropy(hcount, sp_value);
    return wm_rng_mainloop_step(&g.rng);
}

uint32_t wm_fix39_rndrng0(uint32_t maximum_inclusive)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_rng_rndrng0(&g.rng, maximum_inclusive);
}

uint32_t wm_fix39_rng_state(void)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return g.rng.rand_state;
}

const wm_arcade_drone_callbacks_t *wm_fix39_drone_callbacks(void)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return &g.drone_callbacks;
}

const wm_arcade_drone_state_t *wm_fix39_drone_state(size_t index)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (index >= WM_FIX39_ACTOR_COUNT) return 0;
    return &g.drone_state[index];
}

size_t wm_fix39_attract_cycle_begin(void)
{
    size_t n;
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_attract_live_reset(&g.attract_live);
    n = wm_attract_build_cycle(&g.attract,
                               g.attract_steps,
                               WM_FIX39_ATTRACT_MAX_STEPS);
    g.status.attract_step_count = n;
    ++g.status.attract_cycles_built;
    return n;
}

const WmAttractStep *wm_fix39_attract_step(size_t index)
{
    if (index >= g.status.attract_step_count) return 0;
    return &g.attract_steps[index];
}

static uint32_t attract_cap_for_screen(WmAttractScreen screen)
{
    switch (screen) {
        case WM_FIX39_ATTRACT_DESIGNER_HINT:
            return WM_FIX39_ATTRACT_CAP_DESIGNER_HINT;
        case WM_FIX39_ATTRACT_GENERAL_TIPS:
            return WM_FIX39_ATTRACT_CAP_GENERAL_TIPS;
        case WM_FIX39_ATTRACT_COPYRIGHT:
            return WM_FIX39_ATTRACT_CAP_COPYRIGHT;
        case WM_FIX39_ATTRACT_AAMA:
            return WM_FIX39_ATTRACT_CAP_AAMA;
        case WM_FIX39_ATTRACT_OPERATOR_MESSAGE:
            return WM_FIX39_ATTRACT_CAP_OPERATOR_MESSAGE;
        case WM_FIX39_ATTRACT_TIME_DATE:
            return WM_FIX39_ATTRACT_CAP_TIME_DATE;
        case WM_FIX39_ATTRACT_HISCORES:
            return WM_FIX39_ATTRACT_CAP_HISCORES;
        default:
            return 0u;
    }
}

void wm_fix39_attract_set_platform_capabilities(uint32_t capabilities)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    capabilities &= WM_FIX39_ATTRACT_CAP_LIVE_ALL;
    g.attract_platform_capabilities = capabilities;
    g.status.attract_platform_capabilities = capabilities;
    g.status.attract_renderer_ready =
        capabilities == WM_FIX39_ATTRACT_CAP_LIVE_ALL;
}

WmAttractOwner wm_fix39_attract_step_owner(size_t index)
{
    const WmAttractStep *step = wm_fix39_attract_step(index);
    if (!step) return WM_ATTRACT_OWNER_PENDING_DEPENDENCY;
    return wm_attract_live_owner(step->screen);
}

bool wm_fix39_attract_step_runnable(size_t index)
{
    const WmAttractStep *step = wm_fix39_attract_step(index);
    WmAttractOwner owner;
    uint32_t cap;
    if (!step) return false;
    owner = wm_attract_live_owner(step->screen);
    if (owner == WM_ATTRACT_OWNER_EXISTING_FRONTEND) return true;
    if (owner != WM_ATTRACT_OWNER_FIX39_LIVE) return false;
    cap = attract_cap_for_screen(step->screen);
    return cap != 0u && (g.attract_platform_capabilities & cap) != 0u;
}

void wm_fix39_attract_note_pending_skip(size_t index)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (wm_fix39_attract_step(index) != 0 && !wm_fix39_attract_step_runnable(index))
        ++g.status.attract_pending_skips;
}

bool wm_fix39_attract_screen_begin(size_t index)
{
    const WmAttractStep *step;
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (!wm_fix39_attract_step_runnable(index)) {
        wm_attract_live_reset(&g.attract_live);
        return false;
    }
    step = wm_fix39_attract_step(index);
    if (!step || wm_attract_live_owner(step->screen) != WM_ATTRACT_OWNER_FIX39_LIVE) {
        wm_attract_live_reset(&g.attract_live);
        return false;
    }
    if (!wm_attract_live_begin(&g.attract_live, step)) return false;
    if (g.attract_live.waiting_external) ++g.status.attract_external_waits;
    return true;
}

bool wm_fix39_attract_screen_signal_external_result(bool available)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_attract_live_signal_external_result(&g.attract_live, available);
}

bool wm_fix39_attract_screen_signal_external_complete(void)
{
    return wm_fix39_attract_screen_signal_external_result(true);
}

bool wm_fix39_attract_screen_tick(bool any_button)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (!g.attract_live.active && !g.attract_live.done) return false;
    if (g.attract_live.active) ++g.status.attract_live_ticks;
    return wm_attract_live_tick(&g.attract_live, any_button);
}

const WmAttractLive *wm_fix39_attract_live_state(void)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return &g.attract_live;
}

const WmHsSystem *wm_fix39_hiscore_system(void)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return &g.hiscore;
}

void wm_fix39_hiscore_bind_reset_value(uint32_t adjusted_reset_value)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    g.hiscore.adjusted_reset_value = adjusted_reset_value;
    wm_hs_counter_init(&g.hiscore.reset_counter, adjusted_reset_value);
    g.status.hiscore_reset_value_bound = true;
}

bool wm_fix39_hiscore_player_start_or_continue(uint32_t *remaining_out)
{
    uint32_t remaining;
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (!g.status.hiscore_reset_value_bound) return false;
    remaining = wm_hs_system_player_start_or_continue(&g.hiscore);
    if (remaining_out != 0) *remaining_out = remaining;
    return true;
}

const char *wm_fix39_hiscore_recent_initials(bool world_championship)
{
    const WmHsTable *table;
    char *out;
    size_t i;
    if (!g.status.initialized) wm_fix39_runtime_init();
    table = wm_hs_system_table_const(&g.hiscore,
        world_championship ? WM_HS_TABLE_BEATEN : WM_HS_TABLE_INTER);
    out = g.recent_initials[world_championship ? 0 : 1];
    if (!table || !table->entries) {
        out[0] = '\0';
        return out;
    }
    for (i = 0u; i < WM_HS_NUM_INITIALS; ++i)
        out[i] = (char)table->entries[1].initials[i];
    out[WM_HS_NUM_INITIALS] = '\0';
    return out;
}

void wm_fix39_match_begin(unsigned frontend_p1, unsigned frontend_p2)
{
    unsigned i;
    if (!g.status.initialized) wm_fix39_runtime_init();

    {
        int p1_wrestler = frontend_to_arcade(frontend_p1);
        int p2_wrestler = frontend_to_arcade(frontend_p2);
        if (p1_wrestler < 0 || p2_wrestler < 0) {
            g.status.match_started = false;
            return;
        }
        init_actor(&g.actors[0], 0, 0, p1_wrestler,
                   WM_FIX39_P1_START_X, WM_FIX39_P1_START_Z, WM_FIX39_P1_FACING);
        init_actor(&g.actors[1], 1, 1, p2_wrestler,
                   WM_FIX39_P2_START_X, WM_FIX39_P2_START_Z, WM_FIX39_P2_FACING);
    }

    memset(g.trace, 0, sizeof(g.trace));
    memset(g.frame_box, 0, sizeof(g.frame_box));
    memset(g.frame_box_valid, 0, sizeof(g.frame_box_valid));
    g.status.collision_boxes_ready = false;
    g.actor_ptrs[0] = &g.actors[0];
    g.actor_ptrs[1] = &g.actors[1];
    g.actors[0].smart_target = &g.actors[1];
    g.actors[1].smart_target = &g.actors[0];
    refresh_distances();

    init_source_character_animation(&g.actors[0]);
    init_source_character_animation(&g.actors[1]);

    g.old_p1_buttons = 0u;
    g.old_p1_stick = 0u;
    g.match_cpu_vs_cpu = false;
    wm_arcade_drone_init(&g.drone_state[0], 15);
    wm_arcade_drone_init(&g.drone_state[1], 20);
    g.status.pcnt = 0u;
    g.status.round_tickcount = 0u;
    g.status.wrestler_dispatch_ticks = 0u;
    g.status.wrestler_dispatch_ticks_by_player[0] = 0u;
    g.status.wrestler_dispatch_ticks_by_player[1] = 0u;
    g.status.drone_ticks = 0u;
    g.status.drone_input_ticks = 0u;
    g.status.rope_process_ticks = 0u;
    g.status.ringout_process_ticks = 0u;
    g.status.special_process_ticks = 0u;
    g.status.combat_collision_ticks = 0u;
    g.status.attachment_service_calls = 0u;

    wm_arcade_combat_runtime_init(&g.combat_runtime);
    wm_arcade_special_lists_init(&g.special_lists);
    for (i = 0u; i < WM_FIX39_SPECIAL_SLOTS; ++i)
        wm_arcade_special_obj_init(&g.special_pool[i]);

    /* RING.ASM rope processes are created at match setup. */
    for (i = 0u; i < 4u; ++i)
        wm_rope_runtime_init_bank(&g.ropes[i], (WmRopeBank)i, false);

    /* Chunk 6 activation: the live P2 CPU path now owns DRONE state and
       consumes the generated historical tables/scripts every source tick. */
    g.status.drone_scalar_tables_ready = wm_arcade_drone_source_tables_ready();
    g.status.drone_range_tables_ready = wm_arcade_drone_source_ranges_ready();
    g.status.drone_scripts_ready = wm_arcade_drone_source_scripts_ready();
    g.status.drone_runtime_ready = g.status.drone_scalar_tables_ready &&
        g.status.drone_rndrng0_ready && g.status.drone_plain_rnd_ready &&
        g.status.drone_range_tables_ready && g.status.drone_scripts_ready;
    g.status.attract_demo_setup_ready = true;
    g.status.postmatch_router_ready = true;
    g.status.story_plan_ready = true;
    g.status.fireworks_plan_ready = true;
    g.status.match_started = true;
}

bool wm_fix39_match_started(void)
{
    return g.status.match_started;
}

void wm_fix39_match_set_cpu_vs_cpu(bool enabled)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    g.match_cpu_vs_cpu = enabled;
}

void wm_fix39_match_tick(int8_t stick_x, int8_t stick_y,
                         bool run,
                         bool light_punch,
                         bool power_punch,
                         bool light_kick,
                         bool power_kick,
                         bool block)
{
    wm_arcade_actor_t *p1;
    uint16_t now_but;
    uint16_t now_stick;
    uint16_t changed;
    wm_arcade_roster_env_t env;
    WmFix39DispatchContext ctx;
    wm_arcade_move_callbacks_t move_cb;
    unsigned i;
    (void)run; /* RUN is a separate joystick/run behavior service in source. */

    if (!g.status.match_started) return;

    p1 = &g.actors[0];
    if (!g.match_cpu_vs_cpu) {
        now_but = button_bits(light_punch, power_punch,
                              light_kick, power_kick, block);
        now_stick = stick_direction(stick_x, stick_y);

        changed = (uint16_t)(now_but ^ g.old_p1_buttons);
        p1->but_val_cur = now_but;
        p1->but_val_down = (uint16_t)(changed & now_but);
        p1->but_val_up = (uint16_t)(changed & g.old_p1_buttons);

        changed = (uint16_t)(now_stick ^ g.old_p1_stick);
        p1->stick_val_cur = now_stick;
        p1->stick_val_down = (uint16_t)(changed & now_stick);
        p1->stick_val_up = (uint16_t)(changed & g.old_p1_stick);
        p1->move_dir = now_stick;

        g.old_p1_buttons = now_but;
        g.old_p1_stick = now_stick;
    }

    if (g.status.drone_runtime_ready) {
        wm_arcade_drone_world_t world;
        memset(&world, 0, sizeof(world));
        world.actors = g.actor_ptrs;
        world.actor_count = WM_FIX39_ACTOR_COUNT;
        world.pcnt = g.status.pcnt;
        world.round_tickcount = g.status.round_tickcount;
        world.first_ladder = 0;
        if (g.match_cpu_vs_cpu) {
            wm_arcade_drone_step_result_t dr0 = wm_arcade_drone_main(
                &g.actors[0], &g.drone_state[0], &world, &g.drone_callbacks);
            ++g.status.drone_ticks;
            if (dr0 == WM_DRONE_STEP_INPUT || dr0 == WM_DRONE_STEP_BLOCK ||
                g.actors[0].but_val_cur != 0u || g.actors[0].stick_val_cur != 0u)
                ++g.status.drone_input_ticks;
            g.actors[0].move_dir = g.actors[0].stick_val_cur;
        }
        wm_arcade_drone_step_result_t dr = wm_arcade_drone_main(
            &g.actors[1], &g.drone_state[1], &world, &g.drone_callbacks);
        ++g.status.drone_ticks;
        if (dr == WM_DRONE_STEP_INPUT || dr == WM_DRONE_STEP_BLOCK ||
            g.actors[1].but_val_cur != 0u || g.actors[1].stick_val_cur != 0u)
            ++g.status.drone_input_ticks;
        g.actors[1].move_dir = g.actors[1].stick_val_cur;
    }

    /* WRESTLE.ASM loop: ARE_WE_IN_RING is refreshed before the movement
       services.  The exact line service is now source-backed. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i)
        g.actors[i].in_ring = wm_ring_inring_field(
            (int16_t)g.actors[i].x_int, (int16_t)g.actors[i].z_int);

    /* keep_onscreen remains a platform-input bridge because it needs the
       real WORLDTLX/OLD_PSTATUS/meter state.  WRESTLE2 wrestler_veladd and
       WRESTLE wrestler_friction themselves are now live in their source order. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        wm_arcade_wrestler_veladd(&g.actors[i], false, false);
        wm_arcade_wrestler_friction(&g.actors[i]);
    }
    refresh_distances();

    memset(&env, 0, sizeof(env));
    env.pcnt = g.status.pcnt;
    memset(&move_cb, 0, sizeof(move_cb));
    ctx.env = &env;
    move_cb.change_anim_special = queued_special_token;
    move_cb.character_move = live_character_move;
    move_cb.user = &ctx;

    /* WRESTLE.ASM move_wrestler owns character dispatch.  Both actors enter
       it every live source tick; P2 now receives the live DRONE-generated input. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        (void)wm_arcade_move_wrestler(&g.actors[i], 0, &move_cb);
        ++g.status.wrestler_dispatch_ticks;
        ++g.status.wrestler_dispatch_ticks_by_player[i];
    }

    /* The complete ROPES.ASM process interpreter is live.  Rendering the
       source image symbols waits for a real N64 rope-object image adapter. */
    for (i = 0u; i < 4u; ++i) {
        wm_rope_runtime_tick(&g.ropes[i], 0);
        ++g.status.rope_process_ticks;
    }

    /* COLLIS.ASM bridge: frame hurt boxes are supplied by the presentation/
       animation backend only when it has the exact current source-image IANI3
       metadata.  Never synthesize geometry here.  Once both current frame
       boxes are bound, run the existing direct COLLIS/REACT1 port in the
       original wrestler-loop position. */
    g.status.collision_boxes_ready =
        g.frame_box_valid[0] && g.frame_box_valid[1];
    if (g.status.collision_boxes_ready) {
        for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i)
            wm_arcade_set_hurt_box(&g.actors[i], &g.frame_box[i]);
        g.combat_runtime.pcnt = g.status.pcnt;
        g.combat_runtime.round_tickcount = g.status.round_tickcount;
        (void)wm_arcade_check_wrestler_collisions(
            g.actor_ptrs, WM_FIX39_ACTOR_COUNT,
            g.status.round_tickcount, &g.combat_callbacks);
        ++g.status.combat_collision_ticks;
    }

    /* Deliberate source seams:
       - keep_onscreen still needs live camera/operator/meter inputs from the platform;
       - COLLIS.ASM now runs only when exact current-frame IANI3 hurt boxes are bound;
       - no ring-out damage until RING_OUT_ON/operator + health services exist;
       - no projectile process ticks until source animation opcodes spawn them. */

    g.combat_runtime.pcnt = g.status.pcnt;
    ++g.status.pcnt;
    ++g.status.round_tickcount;
}

const wm_arcade_actor_t *wm_fix39_actor(size_t index)
{
    if (index >= WM_FIX39_ACTOR_COUNT) return 0;
    return &g.actors[index];
}

const WmFix39ActorTrace *wm_fix39_actor_trace(size_t index)
{
    if (index >= WM_FIX39_ACTOR_COUNT) return 0;
    return &g.trace[index];
}

bool wm_fix39_match_set_frame_box(size_t index,
                                  const wm_arcade_frame_box_t *frame)
{
    if (index >= WM_FIX39_ACTOR_COUNT || !frame)
        return false;
    g.frame_box[index] = *frame;
    g.frame_box_valid[index] = true;
    g.status.collision_boxes_ready = g.frame_box_valid[0] && g.frame_box_valid[1];
    return true;
}

void wm_fix39_match_clear_frame_box(size_t index)
{
    if (index >= WM_FIX39_ACTOR_COUNT)
        return;
    memset(&g.frame_box[index], 0, sizeof(g.frame_box[index]));
    g.frame_box_valid[index] = false;
    g.status.collision_boxes_ready = false;
}

int32_t wm_fix39_match_life(size_t index)
{
    if (index >= WM_FIX39_ACTOR_COUNT)
        return 0;
    return g.actors[index].life;
}

static void actor_to_onscreen(const wm_arcade_actor_t *a,
                              int16_t climbing_thru,
                              uint32_t saved_a8,
                              uint32_t saved_a9,
                              uint32_t saved_a10,
                              WmRingOnscreenPlayer *p)
{
    memset(p, 0, sizeof(*p));
    p->x_int = (int16_t)a->x_int;
    p->xvel_fp = a->x_vel;
    p->inring = (int16_t)a->in_ring;
    p->player_mode = (int16_t)a->player_mode;
    p->animode = a->anim_mode;
    p->climbing_thru = climbing_thru;
    p->getup_time = (int16_t)a->getup_time;
    p->player_dizzy = (int16_t)a->dizzy;
    p->meter_proc_exists = a->meter_proc != 0;
    p->meter_saved_a8 = saved_a8;
    p->meter_saved_a9 = saved_a9;
    p->meter_saved_a10 = saved_a10;
}

static void onscreen_to_actor(const WmRingOnscreenPlayer *p,
                              wm_arcade_actor_t *a)
{
    a->x_vel = p->xvel_fp;
    a->player_mode = (uint16_t)p->player_mode;
    a->anim_mode = p->animode;
}

WmRingOnscreenEvents wm_fix39_keep_onscreen_before_velocity(
    const WmFix39OnscreenInputs *inputs)
{
    WmRingOnscreenPlayer p1;
    WmRingOnscreenPlayer p2;
    WmRingOnscreenState s;
    WmRingOnscreenEvents e;

    memset(&e, 0, sizeof(e));
    if (!g.status.match_started || inputs == 0) return e;

    actor_to_onscreen(&g.actors[0],
                      inputs->p1_climbing_thru,
                      inputs->p1_meter_saved_a8,
                      inputs->p1_meter_saved_a9,
                      inputs->p1_meter_saved_a10,
                      &p1);
    actor_to_onscreen(&g.actors[1],
                      inputs->p2_climbing_thru,
                      inputs->p2_meter_saved_a8,
                      inputs->p2_meter_saved_a9,
                      inputs->p2_meter_saved_a10,
                      &p2);
    memset(&s, 0, sizeof(s));
    s.old_pstatus = inputs->old_pstatus;
    s.worldtlx_int = inputs->worldtlx_int;
    s.allow_offscrn = inputs->allow_offscreen_io != 0
        ? *inputs->allow_offscreen_io : 0u;
    s.p1 = &p1;
    s.p2 = &p2;

    e = wm_ring_keep_onscreen_tick(&s);
    onscreen_to_actor(&p1, &g.actors[0]);
    onscreen_to_actor(&p2, &g.actors[1]);
    if (inputs->allow_offscreen_io != 0)
        *inputs->allow_offscreen_io = s.allow_offscrn;
    return e;
}

bool wm_fix39_rope_process_alive(unsigned bank)
{
    if (bank >= 4u) return false;
    return g.ropes[bank].process_alive;
}

bool wm_fix39_rope_apply_command(WmRopeBank bank,
                                 WmRopeAction action,
                                 uint8_t selector,
                                 int32_t wrestler_z_fp16)
{
    WmRopeCommand command;
    if (!g.status.initialized) wm_fix39_runtime_init();
    if ((unsigned)bank >= 4u) return false;
    if (!wm_rope_resolve_command(bank, action, selector,
                                 wrestler_z_fp16, &command))
        return false;
    return wm_rope_runtime_apply_resolved_command(
        &g.ropes[(unsigned)bank], &command,
        wm_rope_source_program_resolver, 0);
}


static uint32_t completion_rng(void *user, uint32_t inclusive_max)
{
    (void)user;
    return wm_fix39_rndrng0(inclusive_max);
}

bool wm_fix39_attract_demo_plan(uint16_t amode_loops,
                                bool music_adjustment_nonzero,
                                WmAttractDemoPlan *out)
{
    if (!out) return false;
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_attract_demo_plan_make(out, amode_loops, music_adjustment_nonzero,
                              completion_rng, 0);
    return true;
}

wm_postmatch_result wm_fix39_postmatch_route(const wm_postmatch_input *in)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_match_route_after_match(in);
}

void wm_fix39_rumble_plan(bool human_won, bool anyone_bought_in,
                          wm_rumble_plan *out)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_rumble_build_plan(human_won, anyone_bought_in, out);
}

void wm_fix39_finale_plan(bool eight_on_one, wm_finale_plan *out)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_finale_build_plan(eight_on_one, out);
}

void wm_fix39_story_plan(uint8_t wrestler_index, wm_story_plan *out)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_story_build_plan(wrestler_index, out);
}

void wm_fix39_fireworks_plan(wm_fireworks_plan *out)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_fireworks_build_plan(out);
}

void wm_fix39_game_beaten_plan(const wm_game_beaten_input *in,
                               wm_game_beaten_plan *out)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_game_beaten_build_plan(in, completion_rng, 0, out);
}

bool wm_fix39_hiscore_begin_winstreak(uint8_t human_player_index,
                                      uint8_t wrestler_index,
                                      uint32_t old_winstreak_binary,
                                      WmHsPendingEntry *pending)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_hs_begin_winstreak(&g.hiscore, human_player_index,
                                 wrestler_index, old_winstreak_binary, pending);
}

bool wm_fix39_hiscore_begin_pin_speed(uint8_t actor_index,
                                      uint8_t wrestler_index,
                                      uint8_t current_round,
                                      uint32_t match_timer_bcd,
                                      WmHsPendingEntry *pending)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_hs_begin_pin_speed(&g.hiscore, actor_index, wrestler_index,
                                 current_round, match_timer_bcd, pending);
}

bool wm_fix39_hiscore_begin_beaten(uint8_t human_player_index,
                                   uint8_t wrestler_index,
                                   bool world_belt,
                                   WmHsPendingEntry *pending)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_hs_begin_beaten_game(&g.hiscore, human_player_index,
                                   wrestler_index, world_belt, pending);
}

bool wm_fix39_hiscore_begin_tag_time(uint8_t human_player_index,
                                     uint32_t match_timer_bcd,
                                     WmHsPendingEntry *pending)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_hs_begin_tag_time(&g.hiscore, human_player_index,
                                match_timer_bcd, pending);
}

uint16_t wm_fix39_hiscore_commit_pending(
    const WmHsPendingEntry *pending,
    const uint8_t initials[WM_HS_NUM_INITIALS])
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_hs_commit_pending(&g.hiscore, pending, initials);
}

const WmFix39Status *wm_fix39_status(void)
{
    return &g.status;
}
