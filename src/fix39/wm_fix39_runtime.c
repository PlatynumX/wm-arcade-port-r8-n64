#include "wm_fix39_runtime.h"

#include "wm_arcade_roster.h"
#include "wmania_hiscore_counter.h"
#include "wmania_hiscore_system.h"
#include "wmania_rng.h"
#include "wmania_rope_runtime.h"

#include <limits.h>
#include <string.h>

#define WM_FIX39_ACTOR_COUNT 2u
#define WM_FIX39_ATTRACT_MAX_STEPS 32u

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
    WmRopeRuntimeBank ropes[4];
    wm_arcade_actor_t actors[WM_FIX39_ACTOR_COUNT];
    wm_arcade_actor_t *actor_ptrs[WM_FIX39_ACTOR_COUNT];
    uint16_t old_p1_buttons;
    uint16_t old_p1_stick;
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
    a->player_mode = WM_PMODE_NORMAL;
    a->anim_mode = 0u;
    a->in_ring = 0;
    a->facing_dir = facing;
    a->new_facing_dir = facing;
}

void wm_fix39_runtime_init(void)
{
    unsigned i;

    memset(&g, 0, sizeof(g));

    /* RAND is uninitialized/BSS state in the arcade program; reset baseline 0. */
    wm_rng_init(&g.rng, 0u, 0, 0, 0);

    /* Factory HSTD tables are source data. Counter use waits for GET_ADJ value. */
    wm_hs_system_init(&g.hiscore, 0u);
    g.status.hiscore_tables_valid = wm_hs_system_table_cmos_check(&g.hiscore);

    wm_attract_init(&g.attract);
    g.status.attract_step_count = 0u;

    for (i = 0u; i < 4u; ++i) {
        wm_rope_runtime_init_bank(&g.ropes[i], (WmRopeBank)i, false);
    }

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

size_t wm_fix39_attract_cycle_begin(void)
{
    size_t n;
    n = wm_attract_build_cycle(&g.attract,
                               g.attract_steps,
                               WM_FIX39_ATTRACT_MAX_STEPS);
    g.status.attract_step_count = n;
    return n;
}

const WmAttractStep *wm_fix39_attract_step(size_t index)
{
    if (index >= g.status.attract_step_count) return 0;
    return &g.attract_steps[index];
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

    g.actor_ptrs[0] = &g.actors[0];
    g.actor_ptrs[1] = &g.actors[1];
    g.actors[0].smart_target = &g.actors[1];
    g.actors[1].smart_target = &g.actors[0];
    refresh_distances();

    g.old_p1_buttons = 0u;
    g.old_p1_stick = 0u;
    g.status.pcnt = 0u;
    g.status.round_tickcount = 0u;

    /* RING.ASM rope processes are created at match setup. */
    for (i = 0u; i < 4u; ++i) {
        wm_rope_runtime_init_bank(&g.ropes[i], (WmRopeBank)i, false);
    }

    /*
     * DRONE.ASM is intentionally not started here. Stage25 still names raw
     * tables/scripts plus a separate source `rnd` service that are not in the
     * supplied bundle. Calling it with guessed callbacks would regress fidelity.
     */
    g.status.drone_runtime_ready = false;
    g.status.match_started = true;
}

bool wm_fix39_match_started(void)
{
    return g.status.match_started;
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
    unsigned i;
    (void)run; /* RUN is a separate joystick/run behavior service in source. */

    if (!g.status.match_started) return;

    p1 = &g.actors[0];
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
    refresh_distances();

    /* Live translated rope process ticks; no image adapter is needed at idle. */
    for (i = 0u; i < 4u; ++i) {
        wm_rope_runtime_tick(&g.ropes[i], 0);
    }

    ++g.status.pcnt;
    ++g.status.round_tickcount;
}

const wm_arcade_actor_t *wm_fix39_actor(size_t index)
{
    if (index >= WM_FIX39_ACTOR_COUNT) return 0;
    return &g.actors[index];
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

const WmFix39Status *wm_fix39_status(void)
{
    return &g.status;
}
