#include "wm_fix39_runtime.h"
#include "wm_arcade_anim_combat.h"
#include "wm_arcade_source_attack_frames.h"
#include "wm_arcade_source_animation_runtime.h"
#include "wm_arcade_source_animation_catalog.h"
#include "wm_arcade_target_offsets.h"
#include "wm_arcade_wimp_frame.h"
#include "wm/character_assets.h"
#include "wm_arcade_movement.h"
#include <stdio.h>

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
#include "wm_arcade_react5_core.h"
#include "wm_arcade_react9_core.h"
#include "wm_arcade_special.h"
#include "wm_arcade_wrestler_port.h"
#include "wmania_hiscore_counter.h"
#include "wmania_hiscore_system.h"
#include "wmania_hiscore_persist.h"
#include "wmania_attract_secret.h"
#include "wmania_ring_out.h"
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
    const char *rope_image_symbol[4][WM_FIX39_ROPE_CHANNEL_COUNT][2];
    WmRopeRuntimeAdapter rope_render_adapter;
    wm_arcade_actor_t actors[WM_FIX39_ACTOR_COUNT];
    wm_arcade_actor_t *actor_ptrs[WM_FIX39_ACTOR_COUNT];
    WmFix39ActorTrace trace[WM_FIX39_ACTOR_COUNT];
    wm_source_anim_runtime_t source_anim[WM_FIX39_ACTOR_COUNT];
    wm_source_anim_runtime_t source_torso[WM_FIX39_ACTOR_COUNT];
    wm_source_anim_services_t source_anim_services;
    wm_arcade_combat_runtime_t combat_runtime;
    wm_arcade_frame_box_t frame_box[WM_FIX39_ACTOR_COUNT];
    bool frame_box_valid[WM_FIX39_ACTOR_COUNT];
    wm_arcade_react_callbacks_t react_callbacks;
    wm_arcade_react1_callbacks_t react1_callbacks;
    wm_arcade_react1_context_t react1_context;
    wm_arcade_react_bridge_t react_bridge;
    wm_arcade_combat_callbacks_t combat_callbacks;
    wm_arcade_special_callbacks_t special_callbacks;
    struct {
        bool valid;
        int32_t x, z;
        bool flip_x, blocking;
    } presenter_pose[WM_FIX39_ACTOR_COUNT];
    struct {
        bool valid;
        bool active;
        bool uses_z;
        wm_arcade_attack_on_z_args_t zargs;
        wm_arcade_attack_on_args_t args;
    } presenter_attack[WM_FIX39_ACTOR_COUNT];
    wm_arcade_drone_callbacks_t drone_callbacks;
    wm_arcade_drone_state_t drone_state[WM_FIX39_ACTOR_COUNT];
    wm_arcade_special_lists_t special_lists;
    wm_arcade_special_obj_t special_pool[WM_FIX39_SPECIAL_SLOTS];
    WmRingOutPlayer ringout[WM_FIX39_ACTOR_COUNT];
    bool ring_out_on;
    uint16_t allow_offscreen;
    WmAttractSecretState secret_state;
    WmHsSaveBackend hiscore_backend;
    bool hiscore_backend_bound;
    uint16_t old_p1_buttons;
    uint16_t old_p1_stick;
    bool match_cpu_vs_cpu;
    WmFix39CameraState camera;
    int32_t native_opp_xvel[WM_FIX39_ACTOR_COUNT];
    uint32_t native_last_fling[WM_FIX39_ACTOR_COUNT];
    uint32_t native_last_hiptoss[WM_FIX39_ACTOR_COUNT];
    uint32_t native_last_skick[WM_FIX39_ACTOR_COUNT];
    uint32_t native_last_spunch[WM_FIX39_ACTOR_COUNT];
    int32_t native_close_the_door;
    int32_t native_guy_up;
    int32_t native_guy_in;
    char recent_initials[2][WM_HS_NUM_INITIALS + 1u];
} g;

static void live_rope_set_image(void *user, WmRopeBank bank, WmRopeChannel channel,
                                WmRopeHalf half, const char *source_image_symbol)
{
    (void)user;
    if ((unsigned)bank >= 4u || (unsigned)channel >= WM_FIX39_ROPE_CHANNEL_COUNT ||
        (unsigned)half >= 2u) return;
    g.rope_image_symbol[(unsigned)bank][(unsigned)channel][(unsigned)half] = source_image_symbol;
}

const char *wm_fix39_rope_image_symbol(WmRopeBank bank, WmRopeChannel channel, WmRopeHalf half)
{
    if ((unsigned)bank >= 4u || (unsigned)channel >= WM_FIX39_ROPE_CHANNEL_COUNT ||
        (unsigned)half >= 2u) return 0;
    return g.rope_image_symbol[(unsigned)bank][(unsigned)channel][(unsigned)half];
}

static int32_t fixed16(int32_t v)
{
    return v << 16;
}

static int32_t abs32(int32_t v)
{
    if (v == INT32_MIN) return INT32_MAX;
    return v < 0 ? -v : v;
}

/* Source-facing authority.  GAME.EQU encodes the eight compass directions as
   UP=1, DOWN=2, LEFT=4, RIGHT=8.  WRESTLE.ASM seeds P1/P2 as 9/6, which is
   exactly the sign-vector from each wrestler to the other at the source start
   positions.  NEW_FACING_DIR tracks that target direction; normal animation
   auto-facing copies it into FACING_DIR/OBJ_CONTROL, while MODE_NOAUTOFLIP
   deliberately preserves the current attack/turn facing until the sequence
   releases that bit. */
static uint16_t source_dir_to_opponent(const wm_arcade_actor_t *self,
                                       const wm_arcade_actor_t *opp)
{
    uint16_t dir = WM_MOVE_ZIP;
    int32_t dx, dz;
    if (!self || !opp) return dir;
    dx = opp->x_int - self->x_int;
    dz = opp->z_int - self->z_int;
    if (dx > 0) dir = (uint16_t)(dir | WM_MOVE_RIGHT);
    else if (dx < 0) dir = (uint16_t)(dir | WM_MOVE_LEFT);
    if (dz > 0) dir = (uint16_t)(dir | WM_MOVE_DOWN);
    else if (dz < 0) dir = (uint16_t)(dir | WM_MOVE_UP);
    return dir;
}

static void live_source_face_opponents(void)
{
    unsigned i;
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        wm_arcade_actor_t *a = &g.actors[i];
        wm_arcade_actor_t *opp = a->smart_target;
        uint16_t desired = source_dir_to_opponent(a, opp);
        if (desired == WM_MOVE_ZIP) continue;
        a->new_facing_dir = desired;
        if ((a->anim_mode & WM_ARCADE_MODE_NOAUTOFLIP) == 0u) {
            a->facing_dir = desired;
            /* COLLIS.ASM mirrors X geometry with OBJ_CONTROL/B_FLIPH.
               Facing left therefore uses FLIPH; facing right clears it. */
            if (desired & WM_MOVE_LEFT) a->obj_control |= WM_OBJ_FLIPH;
            else if (desired & WM_MOVE_RIGHT) a->obj_control &= (uint16_t)~WM_OBJ_FLIPH;
        }
    }
}

int wm_fix39_frontend_to_arcade_roster(unsigned id)
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
    (void)user;
    /* REACT5.ASM's direct good_run_hit translation is already in the bundle.
       Use it instead of the old fail-closed placeholder. */
    return wm_arcade_react5_good_run_hit_callback(attacker, victim, 0);
}

static int32_t live_get_health(const wm_arcade_actor_t *victim, void *user)
{
    (void)user;
    return victim ? victim->life : 0;
}

static int live_no_teammates(const wm_arcade_actor_t *victim, void *user)
{
    (void)victim; (void)user;
    return 0; /* current live match path is source 1v1 */
}

static int live_rndper_hi(uint16_t argument, void *user)
{
    WmRng *rng = (WmRng *)user;
    if (!rng) return 0;
    /* RNDPER's caller branches on HI after a 0..255 percentage-style sample.
       Preserve the source probability threshold with the shared arcade RNG. */
    return wm_rng_rndrng0(rng, 255u) > argument;
}

static uint16_t source_anim_round_tick(void *user)
{
    (void)user; return g.status.round_tickcount;
}
static uint32_t source_anim_pcnt(void *user)
{
    (void)user; return g.status.pcnt;
}
static uint32_t source_anim_rndrng0(uint32_t max_inclusive, void *user)
{
    (void)user; return wm_rng_rndrng0(&g.rng,max_inclusive);
}
static int source_anim_rndper_hi(uint16_t probability, void *user)
{
    (void)user; return live_rndper_hi(probability,&g.rng);
}
static void source_anim_sound(wm_arcade_actor_t *a,const char *token,int32_t raw,void *user)
{
    WmFix39ActorTrace *tr=trace_for(a); (void)user;
    if(tr){tr->sound_label=token;tr->sound_token=raw;++tr->sound_events;}
}
static void source_anim_native_am_i_dead(wm_arcade_actor_t *a)
{
    if(!a)return;
    a->anim_mode &= (uint16_t)~WM_ARCADE_MODE_STATUS;
    if(a->life<=0){a->anim_mode|=WM_ARCADE_MODE_STATUS;a->player_mode=WM_PMODE_DEAD;}
    else if(a->player_mode==WM_PMODE_DEAD)a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_anim_native_ckzpos(wm_arcade_actor_t *a)
{
    if(!a)return;
    /* DNKSEQ2/HRT/RZR/... ckzpos: slide toward ring middle near front/rear ropes. */
    if(a->z_int>0x510)a->z_vel=-0x24000;
    else if(a->z_int<=0x442)a->z_vel=0x24000;
}
static void source_anim_native_no_bk_xvel(wm_arcade_actor_t *a)
{
    int32_t x; if(!a)return; x=a->x_vel;
    if(!(a->facing_dir&WM_MOVE_RIGHT))x=-x;
    if(x<0){a->x_vel=0;a->z_vel=0;}
}
static void source_anim_native_choose_2or4(wm_arcade_actor_t *a)
{
    if(!a) return;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;
    if(!(a->new_facing_dir&WM_MOVE_UP))a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_anim_native_make_norm(wm_arcade_actor_t *a)
{
    if(!a)return;
    /* Source make_norm restores normal drawable object mode.  Preserve FLIPH and
       combat-visible flags; clear temporary invisible/ghost animation state. */
    a->anim_mode&=(uint16_t)~(WM_ARCADE_MODE_INVISIBLE|WM_ARCADE_MODE_GHOST);
}
static wm_arcade_actor_t *source_native_opp(wm_arcade_actor_t *a)
{
    if(!a) return 0;
    return a->who_i_hit ? a->who_i_hit : (a->attach_proc ? a->attach_proc : a->smart_target);
}
static void source_native_pause_opp(wm_arcade_actor_t*a){wm_arcade_actor_t*o=source_native_opp(a);if(!o)return;
o->ani_count=25;o->anim_mode|=WM_ARCADE_MODE_UNINT;}
static void source_native_zero_butn(wm_arcade_actor_t*a){if(a)a->delay_butns=0;}
static void source_native_store_opp_xvel(wm_arcade_actor_t*a){int i=actor_index(a);wm_arcade_actor_t*o=a?a->smart_target:0;if(i>=0&&o)g.native_opp_xvel[i]=o->x_vel;}
static void source_native_merge_xvels(wm_arcade_actor_t*a){int i=actor_index(a);if(i>=0&&a)a->x_vel=(g.native_opp_xvel[i]+a->x_vel)>>2;}
static void source_native_reverse_xvel(wm_arcade_actor_t*a){if(a)a->x_vel=-(a->x_vel>>2);}
static void source_native_clear_opp_counts(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(!o)return;
o->punchb_count=o->blockb_count=o->spunchb_count=o->kickb_count=o->skickb_count=0;}
static void source_native_head_grab_time(wm_arcade_actor_t*a){if(!a)return;
a->last_headhold=g.status.pcnt;source_native_clear_opp_counts(a);}
static void source_native_check_xvel(wm_arcade_actor_t*a){wm_arcade_actor_t*o=source_native_opp(a);if(!a)return;
a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(!o||o->in_ring)return;
if(a->x_int<=WM_RING_X_CENTER){if(a->x_vel<0){a->x_vel=0x18000;a->anim_mode|=WM_ARCADE_MODE_STATUS;}}
else if(a->x_vel>=0){a->x_vel=-0x18000;a->anim_mode|=WM_ARCADE_MODE_STATUS;}}
static void source_native_set_opp_facing(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(o)o->facing_dir^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);}
static void source_native_half_vels(wm_arcade_actor_t*a){if(a){a->x_vel>>=1;a->y_vel=0x20000;}}
static void source_native_set_opp_xflip(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(o)o->obj_control^=WM_OBJ_FLIPH;}
static void source_native_reattach(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(!a||!o)return;
a->attach_proc=o;o->attach_proc=a;}
static void source_native_ck_dead_opp(wm_arcade_actor_t*a){wm_arcade_actor_t*o=source_native_opp(a);if(!a)return;
a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(o&&o->life<=0)a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_go_high(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->smart_target:0;if(o&&o->wrestler_num!=3)o->y_vel=0x40000;}
static void source_native_halve_bk_xvel(wm_arcade_actor_t*a){int32_t x;if(!a)return;
x=a->x_vel;if(!(a->facing_dir&WM_MOVE_RIGHT))x=-x;if(x<0)a->x_vel>>=1;}
static void source_native_fix_bnc_flip(wm_arcade_actor_t*a){if(!a)return;
if(a->x_int<WM_RING_X_CENTER)a->obj_control|=WM_OBJ_FLIPH;else a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;}
static void source_native_set_dir_face(wm_arcade_actor_t*a){if(!a)return;
if(a->in_ring)a->new_facing_dir=(a->x_int<WM_RING_X_CENTER)?10:6;else a->new_facing_dir=(a->x_int<WM_RING_X_CENTER)?6:10;}
static void source_native_set_trgt(wm_arcade_actor_t*a){if(!a)return;
a->tgt_xoff=(a->x_int<WM_RING_X_CENTER)?(WM_RING_X_CENTER-0xf8-60):(WM_RING_X_CENTER+0xf8+60);a->tgt_zoff=WM_RING_Z_CENTER;a->tgt_yoff=WM_MAT_Y;}
static void source_native_ckspin(wm_arcade_actor_t*a){if(a&&!(a->facing_dir&WM_MOVE_UP))a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_not_in_ring(wm_arcade_actor_t*a){if(a)a->in_ring=1;}
static void source_native_set_zvel1(wm_arcade_actor_t*a){if(!a)return;
if(a->facing_dir&WM_MOVE_UP)a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;else a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_set_zvel2(wm_arcade_actor_t*a){if(a)a->z_vel=-0x50000;}
static void source_native_set_zvel3(wm_arcade_actor_t*a){if(a)a->z_vel=-0x7c000;}
static void source_native_check_raisearm(wm_arcade_actor_t*a){if(!a)return;
if(a->status_flags&WM_STATUS_DID_RAISEARM)a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;else a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_buckoff_vels(wm_arcade_actor_t*a){if(!a)return;
a->x_vel=(a->x_int<=WM_RING_X_CENTER)?0x20000:-0x20000;a->z_vel=(a->z_int<=WM_RING_Z_CENTER)?0x40000:-0x40000;a->y_vel=0x50000;}
static void source_native_tgt_ground(wm_arcade_actor_t*a){if(a)a->tgt_yoff=0;}
static void source_native_zero_x(wm_arcade_actor_t*a){if(a&&a->closest_xdist<=64)a->x_vel=0;}
static void source_native_free_toss_check(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->smart_target:0;if(!a)return;
a->anim_mode|=WM_ARCADE_MODE_STATUS;if(o&&abs32(o->z_int-a->z_int)>=15)a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;}
static void source_native_setup_freetoss(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(!a)return;
a->anim_mode=0;if(o){o->immobilize_time=20;a->smart_target=o;}}
static void source_native_clr_climb(wm_arcade_actor_t*a){if(a){a->climbing_thru=0;a->safe_time=1;}}
static void source_native_set_opp_y(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;int32_t x;if(!o)return;
o->y_vel=0x50000;o->z_vel=0x20000;x=-0x30000;if(!(o->new_facing_dir&WM_MOVE_RIGHT))x=-x;o->x_vel=x;}
static void source_native_set_wrestler_xflip(wm_arcade_actor_t*a){if(!a)return;
if(a->facing_dir&WM_MOVE_RIGHT)a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;else a->obj_control|=WM_OBJ_FLIPH;}
static void source_native_hit_ground(wm_arcade_actor_t*a){if(!a)return;
a->y_int=a->ground_y;a->y_fixed=a->ground_y<<16;}
static void source_native_setopp_deadanim(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->attach_proc:0;if(o)o->status_flags|=WM_STATUS_DEAD_ANIM;}
static void source_native_opp_grav(wm_arcade_actor_t*a,int low){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(o)o->gravity=WM_ARCADE_GRAVITY-(low?0x1000:0);}
static void source_native_ckongrnd(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->smart_target:0;if(!a)return;
a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(o&&o->player_mode==WM_PMODE_ONGROUND)a->anim_mode|=WM_ARCADE_MODE_STATUS;}
static void source_native_get_off(wm_arcade_actor_t*a,int which){if(!a)return;
if(which==4){a->z_vel=-0x20000;a->y_vel=0x10000;}else{a->z_vel=0x30000;a->y_vel=0x20000;}}
static void source_native_delay_whoihit(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(o)o->delay_meter=55;}
static void source_native_set_immob(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;if(o)o->immobilize_time=60;}
static void source_native_target_whoihit(wm_arcade_actor_t*a){if(!a)return;
a->status_flags|=WM_STATUS_SMART_ATTACK;a->smart_target=a->who_i_hit;}
static void source_native_blocked_vels(wm_arcade_actor_t*a){if(a){a->y_vel=0x30000;a->x_vel=-(a->x_vel>>1);}}
static void source_native_optimal_position(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_i_hit:0;int32_t dx;if(!a||!o)return;
dx=0x460000;if(a->facing_dir&WM_MOVE_LEFT)dx=-dx;o->x_fixed=a->x_fixed+dx;o->x_int=o->x_fixed>>16;}
static void source_native_set_position(wm_arcade_actor_t*a){wm_arcade_actor_t*o=a?a->who_hit_me:0;if(!a||!o)return;/* Source position writes are commented out; palette side effects only. */a->my_pal=a->obj_pal;}
static void __attribute__((unused)) source_native_pause_state(wm_arcade_actor_t*a){if(a)a->anim_mode|=WM_ARCADE_MODE_UNINT;}
/* Combat2DH: source-backed ANI_CODE completion pass.
 * These are direct translations of native routines present in the supplied
 * WRESTLE/WRESTLE2/FINISEQ/character ASM payload. */
static void source_native_get_leap(wm_arcade_actor_t *a)
{
    int away=0;
    if(!a)return;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;
    if((a->x_vel|a->z_vel)==0){a->anim_mode|=WM_ARCADE_MODE_STATUS;return;}
    if(a->new_facing_dir==WM_MOVE_UP_LEFT||a->new_facing_dir==WM_MOVE_DOWN_LEFT)away=WM_MOVE_RIGHT;
    else if(a->new_facing_dir==WM_MOVE_UP_RIGHT||a->new_facing_dir==WM_MOVE_DOWN_RIGHT)away=WM_MOVE_LEFT;
    if(away && (a->move_dir&away))a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_native_adjust_facing(wm_arcade_actor_t *a,int taker)
{
    if(!a)return;
    if(taker){a->facing_dir&=~(WM_MOVE_LEFT|WM_MOVE_DOWN);a->facing_dir|=(WM_MOVE_RIGHT|WM_MOVE_UP);a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;}
    else{a->facing_dir|=(WM_MOVE_LEFT|WM_MOVE_DOWN);a->facing_dir&=~(WM_MOVE_RIGHT|WM_MOVE_UP);a->obj_control|=WM_OBJ_FLIPH;}
}
static void source_native_clear_link(wm_arcade_actor_t *a){if(a)a->attach_proc=0;}
static void source_native_inc_loop(wm_arcade_actor_t *a)
{
    if (!a) {
        return;
    }
    ++a->usr_var1;
    a->anim_mode &= (uint16_t)~WM_ARCADE_MODE_STATUS;
    if (a->usr_var1 > 2) {
        a->anim_mode |= WM_ARCADE_MODE_STATUS;
    }
}
static void source_native_face_inside(wm_arcade_actor_t *a,int use_opp)
{
    int outside=0, yes=0, reverse;
    wm_arcade_actor_t *o;
    if(!a)return;
    if(use_opp){o=source_native_opp(a);outside=o?o->in_ring:0;}
    if(a->x_int<WM_RING_X_CENTER){if(outside)yes=(a->new_facing_dir&WM_MOVE_LEFT)!=0;}
    else{if(!outside)yes=1;else yes=(a->new_facing_dir&WM_MOVE_LEFT)!=0;}
    reverse=(a->wrestler_num==3||a->wrestler_num==6);
    if(yes^reverse)a->obj_control|=WM_OBJ_FLIPH;else a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;
}
static void source_native_set_xdrift(wm_arcade_actor_t *a)
{
    if(!a||a->in_ring)return;
    if(abs32(a->x_int-WM_RING_X_CENTER)<0x60)return;
    a->x_vel=(a->x_int>WM_RING_X_CENTER)?-0x30000:0x30000;
}
static void source_native_hit_nearest(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=source_native_opp(a); if(!a||!o)return;
    a->who_i_hit=o; o->status_flags|=WM_STATUS_PINNED;
}
static void source_native_set_tbukl_airmode(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=source_native_opp(a); if(!a)return;
    a->player_mode=(o&&o->player_mode==WM_PMODE_DEAD)?WM_PMODE_INAIR:WM_PMODE_INAIR2;
}
static void source_native_set_my_pal(wm_arcade_actor_t *a){if(!a)return;a->obj_pal=a->my_pal;a->status_flags&=~WM_STATUS_TEMP_PAL;}
static void source_native_set_pinable_bit(wm_arcade_actor_t *a)
{
    if(a)a->status_flags|=WM_STATUS_PINABLE;
}
static void source_native_set_opp_xy(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=a?a->who_i_hit:0;int32_t xv;if(!o)return;
    o->y_vel=0x20000;xv=-0x20000;if(!(o->new_facing_dir&WM_MOVE_RIGHT))xv=-xv;o->x_vel=xv;
}
static void source_native_guy_flag(wm_arcade_actor_t *a,int in)
{
    (void)a; if(in)g.native_guy_in=1;else g.native_guy_up=1;
}
static void source_native_is_guy_flag(wm_arcade_actor_t *a,int in)
{
    int v;if(!a)return;v=in?g.native_guy_in:g.native_guy_up;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(v)a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_native_change_primary(wm_arcade_actor_t *a,const char *label)
{
    int i=actor_index(a);if(i<0||!label)return;
    if(wm_source_anim_runtime_change(&g.source_anim[i],a,(uint8_t)a->wrestler_num,label))wm_source_anim_runtime_tick(&g.source_anim[i],a);
}
static void source_native_stand_or_dizzy(wm_arcade_actor_t *a,int dizzy)
{
    static const char *stand[9]={"hrt_stand_anim","rzr_stand_anim","und_stand_anim","yok_stand_anim","shn_stand_anim","bam_stand_anim","dnk_stand_anim",0,"lex_stand_anim"};
    static const char *diz[9]={"hrt_fdizzy_anim","rzr_fdizzy_anim","und_fdizzy_anim","yok_fdizzy_anim","shn_fdizzy_anim","bam_fdizzy_anim","dnk_fdizzy_anim",0,"lex_fdizzy_anim"};
    int w;if(!a)return;w=a->wrestler_num;if(w<0||w>8)return;source_native_change_primary(a,dizzy?diz[w]:stand[w]);
}
static void source_native_attach_victim(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=a?a->who_i_hit:0;int i;if(!a||!o)return;
    if(o->player_mode!=WM_PMODE_DEAD)o->player_mode=WM_PMODE_PUPPET;
    o->attach_proc=a;a->attach_proc=o;o->getup_time=0;wm_arcade_wrestler_collisions_off(o);
    i=actor_index(o);if(i>=0&&wm_source_anim_runtime_change(&g.source_anim[i],o,(uint8_t)o->wrestler_num,"wres_slave_anim"))wm_source_anim_runtime_tick(&g.source_anim[i],o);
}
static void source_native_x_flip(wm_arcade_actor_t *a){if(a)a->facing_dir^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);}
static void source_native_setup_run(wm_arcade_actor_t *a)
{
    static const char *run[9]={"hrt_run_anim","rzr_run_anim","und_run_anim","yok_run_anim","shn_run_anim","bam_run_anim","dnk_run_anim","dnk_run_anim","lex_run_anim"};
    int h,w;if(!a)return;h=a->stick_val_cur&(WM_MOVE_LEFT|WM_MOVE_RIGHT);if(!h)h=a->facing_dir&(WM_MOVE_LEFT|WM_MOVE_RIGHT);
    if(h!=(a->facing_dir&(WM_MOVE_LEFT|WM_MOVE_RIGHT))){int v=a->facing_dir&(WM_MOVE_UP|WM_MOVE_DOWN);a->new_facing_dir=h|v;a->facing_dir=a->new_facing_dir;}
    a->getup_time=0;a->usr_var1=0;a->run_time=0;a->move_dir=a->facing_dir&(WM_MOVE_LEFT|WM_MOVE_RIGHT);
    a->facing_dir=a->move_dir|(a->new_facing_dir&(WM_MOVE_UP|WM_MOVE_DOWN));w=a->wrestler_num;if(w>=0&&w<=8)source_native_change_primary(a,run[w]);
    a->player_mode=WM_PMODE_RUNNING;a->delay_butns=1;
}
static void source_native_dead_or_dying(wm_arcade_actor_t *a)
{
    if (!a) {
        return;
    }
    a->anim_mode &= (uint16_t)~WM_ARCADE_MODE_STATUS;
    if (a->i_will_die || a->life <= 0) {
        a->anim_mode |= WM_ARCADE_MODE_STATUS;
    }
}
static void source_native_get_xvel(wm_arcade_actor_t *a)
{
    int right;int32_t xv;if(!a)return;right=(a->facing_dir&WM_MOVE_RIGHT)!=0;xv=a->x_vel;
    if(xv==0){a->x_vel=right?0x20000:-0x20000;return;}
    if((right&&xv<0)||(!right&&xv>=0)){a->x_vel=0;return;}
    a->x_vel=right?0x40000:-0x40000;
}
static void source_native_choose_dir(wm_arcade_actor_t *a)
{
    int dir;if(!a)return;dir=(a->obj_control&WM_OBJ_FLIPH)?WM_MOVE_RIGHT:WM_MOVE_LEFT;dir|=WM_MOVE_DOWN;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;if(a->new_facing_dir&WM_MOVE_UP){a->anim_mode|=WM_ARCADE_MODE_STATUS;dir=(dir&~WM_MOVE_DOWN)|WM_MOVE_UP;}a->facing_dir=dir;
}
static void source_native_ck_flip(wm_arcade_actor_t *a)
{
    int f;if(!a)return;if(a->x_int<=WM_RING_X_CENTER){f=WM_MOVE_RIGHT|WM_MOVE_DOWN;if(a->obj_control&WM_OBJ_FLIPH){a->obj_control^=WM_OBJ_FLIPH;f^=(WM_MOVE_LEFT|WM_MOVE_RIGHT);}}
    else{f=WM_MOVE_LEFT|WM_MOVE_DOWN;if(!(a->obj_control&WM_OBJ_FLIPH)){a->obj_control^=WM_OBJ_FLIPH;f^=(WM_MOVE_RIGHT|WM_MOVE_RIGHT);}}a->facing_dir=f;
}
/* Combat2DH: complete the remaining source-contained ANI_CODE state helpers.
 * These translations are constrained to bodies present in the supplied Midway
 * ASM payload. External-only audit/audio/effect symbols are left to their
 * existing event service rather than invented. */
static void source_native_status(wm_arcade_actor_t *a,int yes)
{
    if(!a)return;
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_STATUS;
    if(yes)a->anim_mode|=WM_ARCADE_MODE_STATUS;
}
static void source_native_full_check_roll(wm_arcade_actor_t *a)
{
    if(!a)return;
    source_native_status(a,0);
    if(a->z_int>(WM_RING_Z_CENTER+20))return;
    a->stick_val_cur|=WM_MOVE_DOWN;
    a->z_vel=0x18000; /* WRESTLE2 do_roll MOVE_DOWN source value. */
    source_native_status(a,1);
}
static void source_native_full_delay(wm_arcade_actor_t *a,uint32_t *stamp,uint32_t ticks)
{
    uint32_t old,now;int i=actor_index(a);if(!a||i<0)return;
    old=stamp[i];now=g.status.pcnt;stamp[i]=now;source_native_status(a,(uint32_t)(now-old)<ticks);
}
static void source_native_full_elbow_tgt2(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=source_native_opp(a);int area;int16_t x,y,z;if(!a||!o)return;
    area=(((a->facing_dir^o->facing_dir)&(WM_MOVE_LEFT|WM_MOVE_RIGHT))==0)?0:2; /* head : groin */
    if(wm_source_target_offsets((uint16_t)o->player_mode,(uint8_t)o->wrestler_num,(uint16_t)area,&x,&y,&z)){
        a->tgt_xoff=x;a->tgt_yoff=y;a->tgt_zoff=z;
    }
}
static void source_native_full_set_target(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=source_native_opp(a);int area;int16_t x,y,z;if(!a||!o)return;
    if(a->closest_xdist<0x40)area=1; /* TGT_CHEST */
    else if(((a->obj_control^o->obj_control)&WM_OBJ_FLIPH)!=0)area=0; /* TGT_HEAD */
    else area=3; /* TGT_KNEES */
    if(wm_source_target_offsets((uint16_t)o->player_mode,(uint8_t)o->wrestler_num,(uint16_t)area,&x,&y,&z)){
        a->tgt_xoff=x;a->tgt_yoff=y;a->tgt_zoff=z;
    }
}
static void source_native_full_set_zvel(wm_arcade_actor_t *a)
{
    wm_arcade_actor_t *o=source_native_opp(a);int ticks;if(!a||!o)return;
    ticks=abs32(a->x_int-o->x_int)/7;if(ticks<=0){a->z_vel=0;return;}
    a->z_vel=((o->z_int-a->z_int)<<16)/ticks;
}
static void source_native_full_grnd_hit(wm_arcade_actor_t *a)
{
    static const char *hit[9]={"hrt_hitonground_anim","rzr_hitonground_anim","und_hitonground_anim","yok_hitonground_anim","shn_hitonground_anim","bam_hitonground_anim","dnk_hitonground_anim",0,"lex_hitonground_anim"};
    wm_arcade_actor_t *o=a?a->who_i_hit:0;int w;if(!a||!o)return;w=o->wrestler_num;
    if(w>=0&&w<=8&&hit[w])source_native_change_primary(o,hit[w]);
    if(a->wrestler_num!=2){a->z_fixed=o->z_fixed+(1<<16);a->z_int=a->z_fixed>>16;o->z_fixed-=2<<16;o->z_int=o->z_fixed>>16;}
}
static void source_native_full_set_new_position(wm_arcade_actor_t *a)
{
    static const int16_t pos[][4]={
      {WM_RING_X_CENTER,WM_RING_Z_CENTER,WM_MAT_Y,0},{WM_RING_TOP_LEFT,WM_RING_Z_CENTER,WM_MAT_Y,0},{WM_RING_TOP_RIGHT,WM_RING_Z_CENTER,WM_MAT_Y,0},
      {WM_RING_BOT_LEFT,WM_RING_BOT,WM_MAT_Y,0},{WM_RING_BOT_RIGHT,WM_RING_BOT,WM_MAT_Y,0},
      {675,1184,0,1},{1475,921,0,1},{965,683,0,1},{1253,656,0,1},{814,1648,0,1},{1415,1608,0,1},{1097,1648,0,1},{659,696,0,1},{528,1657,0,1},{1652,1615,0,1},{1709,699,0,1},{1791,1174,0,1},{1874,1548,0,1},
      {WM_RING_X_CENTER,WM_RING_Z_CENTER,WM_MAT_Y,0}};
    size_t i,chosen=(sizeof(pos)/sizeof(pos[0]))-1;int tl=(int)(g.camera.worldtlx_fp16>>16);if(!a)return;
    for(i=0;i+1<sizeof(pos)/sizeof(pos[0]);++i){if(pos[i][0]>=tl+25&&pos[i][0]<=tl+375){chosen=i;break;}}
    a->x_int=pos[chosen][0];a->z_int=pos[chosen][1];a->y_int=pos[chosen][2];a->ground_y=pos[chosen][2];a->in_ring=pos[chosen][3];
    a->x_fixed=a->x_int<<16;a->z_fixed=a->z_int<<16;a->y_fixed=a->y_int<<16;
}
static void source_native_full_set_speeds(wm_arcade_actor_t *a)
{
    int z;if(!a)return;a->x_vel=((1250+7-a->x_int)<<16)/16;z=a->z_int;if(z<((WM_RING_Z_CENTER+20+28)&0xfff)){z=((WM_RING_Z_CENTER+20+28)&0xfff);a->z_int=z;a->z_fixed=z<<16;}
    a->z_vel=((((WM_RING_Z_CENTER+20+28)&0xfff)-z)<<16)/16;
}
static void source_native_full_tbukl_confine(wm_arcade_actor_t *a)
{
    int i,enemy_out=0,enemy_alive=0;if(!a)return;
    if(a->wrestler_num==3){a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_NOCONFINE;return;}
    for(i=0;i<(int)WM_FIX39_ACTOR_COUNT;i++){wm_arcade_actor_t *o=&g.actors[i];if(!o->active||o==a||o->player_side==a->player_side)continue;if(o->player_mode!=WM_PMODE_DEAD)enemy_alive=1;if(o->in_ring)enemy_out=1;}
    if(enemy_out){a->anim_mode|=WM_ARCADE_MODE_NOCONFINE;return;}
    a->anim_mode&=(uint16_t)~WM_ARCADE_MODE_NOCONFINE;
    if(a->in_ring){a->in_ring=0;a->ground_y=WM_MAT_Y;if(a->z_int<WM_RING_TOP)a->z_int=WM_RING_TOP;(void)enemy_alive;}
}
static void source_native_full_tgt_tbukl(wm_arcade_actor_t *a)
{
    const WmRingBoundarySeed *line;int x;if(!a)return;line=wm_ring_boundary_seed(a->x_int>WM_RING_X_CENTER?WM_RING_BOUNDARY_RIGHT_ROPE:WM_RING_BOUNDARY_LEFT_ROPE);
    x=wm_ring_calc_line_x(line,(int16_t)(WM_RING_Z_CENTER-16));a->tgt_xoff=x;a->tgt_zoff=WM_RING_Z_CENTER-16;a->tgt_yoff=WM_MAT_Y+80;
}
static void source_native_full_restore_pal(wm_arcade_actor_t *a){if(!a)return;a->obj_pal=a->my_pal;a->status_flags&=~WM_STATUS_TEMP_PAL;}
static void source_native_full_make_white(wm_arcade_actor_t *a){if(a)a->anim_mode&=(uint16_t)~(WM_ARCADE_MODE_INVISIBLE|WM_ARCADE_MODE_GHOST);}
static void source_native_full_close_door(wm_arcade_actor_t *a){(void)a;g.native_close_the_door=2;}
static void source_native_full_is_door_open(wm_arcade_actor_t *a){source_native_status(a,g.native_close_the_door!=0);}
static void source_native_full_effect(wm_arcade_actor_t *a,const char *label){source_anim_sound(a,label,0,0);}
static void source_anim_code(wm_arcade_actor_t *a,const char *label,void *user)
{
    (void)user; if(!a||!label)return;
    /* State-affecting ANI_CODE callbacks used by the eight shipped wrestlers.
       Audio/visual-only callbacks are recorded by source_anim_sound elsewhere. */
    if(!strcmp(label,"am_I_dead")){source_anim_native_am_i_dead(a);return;}
    if(!strcmp(label,"ckzpos")){source_anim_native_ckzpos(a);return;}
    if(!strcmp(label,"no_bk_xvel")){source_anim_native_no_bk_xvel(a);return;}
    if(!strcmp(label,"choose_2or4")){source_anim_native_choose_2or4(a);return;}
    if(!strcmp(label,"make_norm")){source_anim_native_make_norm(a);return;}
    if(!strcmp(label,"pause_opp")){source_native_pause_opp(a);return;}
    if(!strcmp(label,"zero_butn")){source_native_zero_butn(a);return;}
    if(!strcmp(label,"store_opp_xvel")){source_native_store_opp_xvel(a);return;}
    if(!strcmp(label,"merge_xvels")){source_native_merge_xvels(a);return;}
    if(!strcmp(label,"reverse_xvel")){source_native_reverse_xvel(a);return;}
    if(!strcmp(label,"clear_opp_counts")){source_native_clear_opp_counts(a);return;}
    if(!strcmp(label,"head_grab_time")){source_native_head_grab_time(a);return;}
    if(!strcmp(label,"check_xvel")){source_native_check_xvel(a);return;}
    if(!strcmp(label,"set_opp_facing")){source_native_set_opp_facing(a);return;}
    if(!strcmp(label,"half_vels")){source_native_half_vels(a);return;}
    if(!strcmp(label,"set_opp_xflip")){source_native_set_opp_xflip(a);return;}
    if(!strcmp(label,"reattach")){source_native_reattach(a);return;}
    if(!strcmp(label,"ck_dead_opp")){source_native_ck_dead_opp(a);return;}
    if(!strcmp(label,"go_high")){source_native_go_high(a);return;}
    if(!strcmp(label,"halve_bk_xvel")){source_native_halve_bk_xvel(a);return;}
    if(!strcmp(label,"fix_bnc_flip")){source_native_fix_bnc_flip(a);return;}
    if(!strcmp(label,"SET_DIR_FACE")){source_native_set_dir_face(a);return;}
    if(!strcmp(label,"set_trgt")){source_native_set_trgt(a);return;}
    if(!strcmp(label,"ckspin")){source_native_ckspin(a);return;}
    if(!strcmp(label,"NOT_IN_RING")){source_native_not_in_ring(a);return;}
    if(!strcmp(label,"set_zvel1")){source_native_set_zvel1(a);return;}
    if(!strcmp(label,"set_zvel2")){source_native_set_zvel2(a);return;}
    if(!strcmp(label,"set_zvel3")){source_native_set_zvel3(a);return;}
    if(!strcmp(label,"check_raisearm_bit")){source_native_check_raisearm(a);return;}
    if(!strcmp(label,"set_buckoff_vels")){source_native_buckoff_vels(a);return;}
    if(!strcmp(label,"tgt_ground")){source_native_tgt_ground(a);return;}
    if(!strcmp(label,"zero_x")||!strcmp(label,"zero_x_4")){source_native_zero_x(a);return;}
    if(!strcmp(label,"free_toss_check")){source_native_free_toss_check(a);return;}
    if(!strcmp(label,"setup_freetoss")){source_native_setup_freetoss(a);return;}
    if(!strcmp(label,"clr_climb")){source_native_clr_climb(a);return;}
    if(!strcmp(label,"set_opp_y")){source_native_set_opp_y(a);return;}
    if(!strcmp(label,"set_wrestler_xflip")){source_native_set_wrestler_xflip(a);return;}
    if(!strcmp(label,"hit_ground")){source_native_hit_ground(a);return;}
    if(!strcmp(label,"setopp_deadanim")){source_native_setopp_deadanim(a);return;}
    if(!strcmp(label,"SET_OPP_GRAV_LOW")){source_native_opp_grav(a,1);return;}
    if(!strcmp(label,"SET_OPP_GRAV_NORM")){source_native_opp_grav(a,0);return;}
    if(!strcmp(label,"ckongrnd")){source_native_ckongrnd(a);return;}
    if(!strcmp(label,"get_off")){source_native_get_off(a,0);return;}
    if(!strcmp(label,"get_off4")){source_native_get_off(a,4);return;}
    if(!strcmp(label,"delay_whoihit")){source_native_delay_whoihit(a);return;}
    if(!strcmp(label,"set_immob")){source_native_set_immob(a);return;}
    if(!strcmp(label,"target_whoihit")){source_native_target_whoihit(a);return;}
    if(!strcmp(label,"blocked_vels")){source_native_blocked_vels(a);return;}
    if(!strcmp(label,"SET_OPTIMAL_POSITION")){source_native_optimal_position(a);return;}
    if(!strcmp(label,"set_position")){source_native_set_position(a);return;}
    if(!strcmp(label,"get_leap")){source_native_get_leap(a);return;}
    if(!strcmp(label,"adjust_facing")){source_native_adjust_facing(a,0);return;}
    if(!strcmp(label,"adjust_taker_facing")){source_native_adjust_facing(a,1);return;}
    if(!strcmp(label,"clear_link")){source_native_clear_link(a);return;}
    if(!strcmp(label,"inc_loop")){source_native_inc_loop(a);return;}
    if(!strcmp(label,"face_inside")){source_native_face_inside(a,0);return;}
    if(!strcmp(label,"tbukl_flip")){source_native_face_inside(a,1);return;}
    if(!strcmp(label,"set_xdrift")){source_native_set_xdrift(a);return;}
    if(!strcmp(label,"hit_nearest")){source_native_hit_nearest(a);return;}
    if(!strcmp(label,"set_tbukl_airmode")){source_native_set_tbukl_airmode(a);return;}
    if(!strcmp(label,"set_my_pal")){source_native_set_my_pal(a);return;}
    if(!strcmp(label,"set_pinable_bit")){source_native_set_pinable_bit(a);return;}
    if(!strcmp(label,"set_opp_xy")){source_native_set_opp_xy(a);return;}
    if(!strcmp(label,"guy_is_up")){source_native_guy_flag(a,0);return;}
    if(!strcmp(label,"guy_is_in")){source_native_guy_flag(a,1);return;}
    if(!strcmp(label,"is_guy_up")){source_native_is_guy_flag(a,0);return;}
    if(!strcmp(label,"is_he_in")){source_native_is_guy_flag(a,1);return;}
    if(!strcmp(label,"stand_wrestler")){source_native_stand_or_dizzy(a,0);return;}
    if(!strcmp(label,"dizzy_wrestler")){source_native_stand_or_dizzy(a,1);return;}
    if(!strcmp(label,"attach_victim")){source_native_attach_victim(a);return;}
    if(!strcmp(label,"x_flip")){source_native_x_flip(a);return;}
    if(!strcmp(label,"setup_run")){source_native_setup_run(a);return;}
    if(!strcmp(label,"dead_or_dying")){source_native_dead_or_dying(a);return;}
    if(!strcmp(label,"get_xvel")){source_native_get_xvel(a);return;}
    if(!strcmp(label,"choose_dir")){source_native_choose_dir(a);return;}
    if(!strcmp(label,"ck_flip")){source_native_ck_flip(a);return;}
    if(!strcmp(label,"check_roll")){source_native_full_check_roll(a);return;}
    if(!strcmp(label,"fling_delay")){source_native_full_delay(a,g.native_last_fling,3u*60u);return;}
    if(!strcmp(label,"hiptoss_delay")){source_native_full_delay(a,g.native_last_hiptoss,3u*60u);return;}
    if(!strcmp(label,"skick_delay")){source_native_full_delay(a,g.native_last_skick,2u*60u);return;}
    if(!strcmp(label,"spunch_delay")){source_native_full_delay(a,g.native_last_spunch,2u*60u);return;}
    if(!strcmp(label,"elbow_tgt2")){source_native_full_elbow_tgt2(a);return;}
    if(!strcmp(label,"set_target")){source_native_full_set_target(a);return;}
    if(!strcmp(label,"set_zvel")){source_native_full_set_zvel(a);return;}
    if(!strcmp(label,"grnd_hit")){source_native_full_grnd_hit(a);return;}
    if(!strcmp(label,"set_new_position")){source_native_full_set_new_position(a);return;}
    if(!strcmp(label,"set_speeds")){source_native_full_set_speeds(a);return;}
    if(!strcmp(label,"set_tbukl_confine")){source_native_full_tbukl_confine(a);return;}
    if(!strcmp(label,"tgt_tbukl")){source_native_full_tgt_tbukl(a);return;}
    if(!strcmp(label,"restore_pal")){source_native_full_restore_pal(a);return;}
    if(!strcmp(label,"make_white")||!strcmp(label,"make_black")){source_native_full_make_white(a);return;}
    if(!strcmp(label,"close_door")){source_native_full_close_door(a);return;}
    if(!strcmp(label,"is_door_open")){source_native_full_is_door_open(a);return;}
    if(!strcmp(label,"draw_ddt_name")||!strcmp(label,"win_announce")||!strcmp(label,"shake_all_ropes")||!strcmp(label,"start_smoke")||!strcmp(label,"start_sparks")||!strcmp(label,"set_pal")||!strcmp(label,"set_skeleton_pal")){source_native_full_effect(a,label);return;}
    if(!strcmp(label,"DO_CROWD_CHEER")){WmFix39ActorTrace *tr=trace_for(a);if(tr)++tr->external_special_events;return;}
    if(!strcmp(label,"HIT_THE_MAT")||!strcmp(label,"SMALL_BOUNCE")||
       !strcmp(label,"SMALL_RUN")||!strcmp(label,"impact_sound")||
       !strcmp(label,"CALL_MISSES")||!strcmp(label,"DO_GRUNT")||
       !strcmp(label,"DO_WAIL")||!strcmp(label,"DO_SCREAM")||
       !strcmp(label,"MAKE_HIM_SCREAM")){
        source_anim_sound(a,label,0,0); return;
    }
    /* Preserve the native label for diagnostics instead of silently pretending
       the call executed.  Native routines outside ANIM.ASM remain separately
       auditable source services. */
    {WmFix39ActorTrace *tr=trace_for(a);if(tr){tr->external_special_label=label;++tr->external_special_events;}}
}
static bool source_anim_change_other(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const char *label,void *user)
{
    int i=actor_index(o);(void)a;(void)user;if(i<0||!label)return false;
    if(!wm_source_anim_runtime_change(&g.source_anim[i],o,(uint8_t)o->wrestler_num,label))return false;
    wm_source_anim_runtime_tick(&g.source_anim[i],o); return true;
}
static bool source_anim_force_other(wm_arcade_actor_t *a,wm_arcade_actor_t *o,const char *frame,void *user)
{
    int i=actor_index(o);(void)a;(void)user;if(i<0||!frame)return false;
    wm_source_anim_runtime_force_frame(&g.source_anim[i],frame);return true;
}
static int source_anim_do_roll(wm_arcade_actor_t *a,void *user)
{
    uint16_t d;(void)user;if(!a)return 0;d=a->stick_val_cur&(WM_MOVE_UP|WM_MOVE_DOWN);
    if(!d){a->z_vel=0;return 0;}
    /* WRESTLE2.ASM do_roll: direction controls sign.  Character-specific roll
       image progression remains owned by the animation frame table. */
    a->z_vel=(d&WM_MOVE_DOWN)?0x18000:-0x18000;return 1;
}
static int source_anim_buttons_down(wm_arcade_actor_t *a,void *user)
{
    (void)user;return a?(int)(a->but_val_down|a->stick_val_down):0;
}
static void source_anim_allow_offscreen(int ticks,void *user)
{
    (void)user;g.allow_offscreen=(uint16_t)(ticks<0?0:ticks);
}
static void init_source_anim_services(void)
{
    memset(&g.source_anim_services,0,sizeof(g.source_anim_services));
    g.source_anim_services.round_tick=source_anim_round_tick;
    g.source_anim_services.pcnt=source_anim_pcnt;
    g.source_anim_services.rndrng0=source_anim_rndrng0;
    g.source_anim_services.rndper_hi=source_anim_rndper_hi;
    g.source_anim_services.sound=source_anim_sound;
    g.source_anim_services.code=source_anim_code;
    g.source_anim_services.change_other_anim=source_anim_change_other;
    g.source_anim_services.force_other_frame=source_anim_force_other;
    g.source_anim_services.do_roll=source_anim_do_roll;
    g.source_anim_services.buttons_down=source_anim_buttons_down;
    g.source_anim_services.set_allow_offscreen=source_anim_allow_offscreen;
    g.source_anim_services.combat_runtime=&g.combat_runtime;
    g.source_anim_services.react=&g.react_callbacks;
    g.source_anim_services.user=&g;
}

static bool live_start_source_anim(wm_arcade_actor_t *a, const char *label, bool torso)
{
    int i=actor_index(a); wm_source_anim_runtime_t *rt; bool ok;
    if(i<0||!label)return false;
    rt=torso?&g.source_torso[i]:&g.source_anim[i];
    ok=wm_source_anim_runtime_change(rt,a,(uint8_t)a->wrestler_num,label);
    if(ok)wm_source_anim_runtime_tick(rt,a); /* ANIM.ASM change_anim1a/2a */
    return ok;
}

static const char *live_default_stand_label(const wm_arcade_actor_t *a)
{
    bool face2=a && (a->facing_dir&WM_MOVE_UP);
    if(!a)return 0;
    switch(a->wrestler_num){
    case WM_ROSTER_BRET:return face2?"hrt_stand2_anim":"hrt_stand4_anim";
    case WM_ROSTER_RAZOR:return face2?"rzr_stand2_anim":"rzr_stand4_anim";
    case WM_ROSTER_TAKER:return face2?"und_stand2_anim":"und_stand4_anim";
    case WM_ROSTER_YOKO:return face2?"yok_stand2_anim":"yok_stand4_anim";
    case WM_ROSTER_SHAWN:return face2?"shn_stand2_anim":"shn_stand4_anim";
    case WM_ROSTER_BAM:return face2?"bam_stand2_anim":"bam_stand4_anim";
    case WM_ROSTER_DOINK:return face2?"dnk_stand2_anim":"dnk_stand4_anim";
    case WM_ROSTER_LEX:return face2?"lex_stand2_anim":"lex_stand4_anim";
    default:return 0;}
}

static void live_react_anim(wm_arcade_actor_t *victim,
                            wm_arcade_react1_anim_group_t group,
                            void *user)
{
    WmFix39ActorTrace *tr=trace_for(victim);
    const char *label= victim ? wm_source_reaction_anim_label((uint8_t)victim->wrestler_num,(int)group,victim->facing_dir) : 0;
    (void)user;
    if(tr){tr->animation_token=-(int32_t)group;tr->animation_label=label;++tr->animation_events;}
    if(label)(void)live_start_source_anim(victim,label,false);
}

static void live_react_sound(wm_arcade_actor_t *victim,
                             wm_arcade_react1_sound_t sound,
                             void *user)
{
    WmFix39ActorTrace *t = trace_for(victim);
    (void)user;
    if (t) {
        t->sound_token = -(int32_t)sound;
        ++t->sound_events;
    }
}

static void live_react_collisions_off(wm_arcade_actor_t *victim, void *user)
{
    (void)user;
    wm_arcade_wrestler_collisions_off(victim);
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

static void live_reaction_dispatch(wm_arcade_actor_t *attacker,
                                   wm_arcade_actor_t *victim,
                                   wm_arcade_reaction_id_t reaction,
                                   int16_t *pending,
                                   int16_t *newdir,
                                   void *user)
{
    (void)user;
    ++g.status.reaction_dispatches;
    wm_arcade_react123456789_reaction_callback(attacker, victim, reaction,
                                               pending, newdir, &g.react1_context);
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
/* Already-ported shared services, now connected to the live match spine.    */
/* ------------------------------------------------------------------------- */

static wm_arcade_special_obj_t *live_alloc_special_slot(void)
{
    unsigned i;
    for (i = 0u; i < WM_FIX39_SPECIAL_SLOTS; ++i) {
        if (!g.special_pool[i].in_list)
            return &g.special_pool[i];
    }
    return 0;
}

static bool live_spawn_special(wm_arcade_actor_t *owner,
                               wm_arcade_special_kind_t kind)
{
    wm_arcade_special_obj_t *obj;
    if (!owner) return false;
    obj = live_alloc_special_slot();
    if (!obj) return false;
    switch (kind) {
        case WM_SP_KIND_DOINK_PIE:
            wm_arcade_spawn_doink_pie(&g.special_lists, obj, owner); break;
        case WM_SP_KIND_BAM_FIREBALL:
            wm_arcade_spawn_bam_fireball(&g.special_lists, obj, owner); break;
        case WM_SP_KIND_TAKER_SPIRIT:
            wm_arcade_spawn_taker_spirit(&g.special_lists, obj, owner); break;
        case WM_SP_KIND_TAKER_REAPER:
            wm_arcade_spawn_taker_reaper(&g.special_lists, obj, owner); break;
        case WM_SP_KIND_YOKO_SALT:
            wm_arcade_spawn_yoko_salt(&g.special_lists, obj, owner); break;
        default:
            return false;
    }
    return true;
}

static bool live_ring_adjust_health(void *user,
                                    uint8_t player_num,
                                    int16_t delta,
                                    int16_t source_a10_zero)
{
    wm_arcade_actor_t *a;
    int32_t next;
    (void)user;
    (void)source_a10_zero;
    if (player_num >= WM_FIX39_ACTOR_COUNT) return false;
    a = &g.actors[player_num];
    next = a->life + delta;
    if (next < 0) next = 0;
    if (next > 100) next = 100;
    a->life = next;
    return next == 0;
}

static void live_ringout_refresh(unsigned i)
{
    WmRingOutPlayer *r = &g.ringout[i];
    wm_arcade_actor_t *a = &g.actors[i];
    r->active = a->active != 0;
    r->player_num = (uint8_t)a->player_num;
    r->player_side = (int16_t)a->player_side;
    r->player_mode = (int16_t)a->player_mode;
    r->status_flags = (uint16_t)a->status_flags;
    r->inring = (int16_t)a->in_ring;
    r->ptime = (uint16_t)(a->ptime > 0 ? a->ptime : 1);
    r->ground_y = (int16_t)a->ground_y;
    r->object_y_int = (int16_t)a->y_int;
    r->closest_num = (uint8_t)(i ^ 1u);
}

static void live_ringout_tick(void)
{
    unsigned i;
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i)
        live_ringout_refresh(i);
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        WmRingOutEvents e = wm_ring_are_we_in_ring_tick(
            &g.ringout[i], g.ringout, WM_FIX39_ACTOR_COUNT,
            g.status.pcnt, 60u, false, g.ring_out_on,
            live_ring_adjust_health, 0);
        g.actors[i].player_mode = (uint16_t)g.ringout[i].player_mode;
        if (e.spawn_kill_when_hit_ground &&
            wm_ring_kill_when_hit_ground_ready(&g.ringout[i])) {
            (void)wm_ring_kill_when_hit_ground_apply(
                &g.ringout[i], live_ring_adjust_health, 0);
        }
        ++g.status.ringout_process_ticks;
    }
}

static void live_keep_onscreen_from_camera(void)
{
    WmFix39OnscreenInputs in;
    memset(&in, 0, sizeof(in));
    in.worldtlx_int = (int16_t)(g.camera.worldtlx_fp16 >> 16);
    in.old_pstatus = WM_RING_KEEP_REQUIRED_OLD_PSTATUS;
    in.allow_offscreen_io = &g.allow_offscreen;
    in.p1_climbing_thru = (int16_t)g.actors[0].climbing_thru;
    in.p2_climbing_thru = (int16_t)g.actors[1].climbing_thru;
    (void)wm_fix39_keep_onscreen_before_velocity(&in);
    g.status.camera_onscreen_inputs_ready = true;
}

/* Direct translation of the ordinary PSTATUS==0 attract path in
 * WRESTLE2.ASM::scroll_world.  The source chooses the first live drone and
 * CLOSEST_NUM; with the current two live source actors that resolves to 0/1.
 * This routine deliberately does not infer coordinates from presenter pixels. */
static void live_scroll_world_attract(void)
{
    const wm_arcade_actor_t *a = 0;
    const wm_arcade_actor_t *b = 0;
    size_t i;
    int32_t target, delta, next;

    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        if (g.actors[i].active && g.actors[i].player_mode != WM_PMODE_DEAD) {
            a = &g.actors[i];
            b = a->smart_target;
            break;
        }
    }
    if (!a) {
        a = &g.actors[0];
        b = &g.actors[1];
    }
    if (!b || !b->active || b->player_mode == WM_PMODE_DEAD)
        b = (a == &g.actors[0]) ? &g.actors[1] : &g.actors[0];

    /* X: midpoint - 200, 20px dead zone, 1/8 easing, source fence limits. */
    target = (int32_t)(((int64_t)a->x_fixed + (int64_t)b->x_fixed) / 2) - (200 << 16);
    delta = target - g.camera.worldtlx_fp16;
    if (delta < 0) {
        delta += (20 << 16);
        if (delta < 0) {
            next = g.camera.worldtlx_fp16 + (delta >> 3);
            if (next >= (0x12f << 16) && next <= (0x648 << 16))
                g.camera.worldtlx_fp16 = next;
        }
    } else {
        delta -= (20 << 16);
        if (delta >= 0) {
            next = g.camera.worldtlx_fp16 + (delta >> 3);
            if (next >= (0x12f << 16) && next <= (0x648 << 16))
                g.camera.worldtlx_fp16 = next;
        }
    }

    /* Y: ((average Z integer) * Y_SCALE_MULTIPLIER) - average Y - 0xd8,
       then 1/4 easing.  RING.EQU Y_SCALE_MULTIPLIER is 0x3566. */
    {
        int32_t zavg_int = (int32_t)(((int64_t)a->z_fixed + (int64_t)b->z_fixed) >> 17);
        int32_t zscreen_fp16 = zavg_int * 0x3566;
        int32_t yavg_fp16 = (int32_t)(((int64_t)a->y_fixed + (int64_t)b->y_fixed) / 2);
        target = zscreen_fp16 - yavg_fp16 - (0x0d8 << 16);
        delta = target - g.camera.worldtly_fp16;
        next = g.camera.worldtly_fp16 + (delta >> 2);
        /* WRESTLE2.ASM only rejects values beyond the front fence. */
        if (next <= (0x97 << 16))
            g.camera.worldtly_fp16 = next;
    }
}

const WmFix39CameraState *wm_fix39_camera_state(void)
{
    return &g.camera;
}

int16_t wm_fix39_camera_worldtlx_int(void)
{
    return (int16_t)(g.camera.worldtlx_fp16 >> 16);
}

int16_t wm_fix39_camera_worldtly_int(void)
{
    return (int16_t)(g.camera.worldtly_fp16 >> 16);
}

/* ------------------------------------------------------------------------- */
/* Source callback surfaces that are complete in the supplied ZIPs.         */
/* ------------------------------------------------------------------------- */

static void common_anim_label(wm_arcade_actor_t *a, const char *label, void *user)
{
    WmFix39ActorTrace *tr=trace_for(a); (void)user;
    if(tr){tr->animation_label=label;++tr->animation_events;}
    (void)live_start_source_anim(a,label,false);
}

static void common_torso_label(wm_arcade_actor_t *a, const char *label, void *user)
{
    WmFix39ActorTrace *tr=trace_for(a); (void)user;
    if(tr){tr->torso_animation_label=label;++tr->animation_events;}
    (void)live_start_source_anim(a,label,true);
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
    /* Connect source labels to the SPECIAL.ASM constructors already ported
       in this bundle. Unknown process labels remain ordinary wrestler-special
       processes and are not misclassified as projectiles. */
    if (label != 0) {
        if (strstr(label, "salt") != 0)
            (void)live_spawn_special(a, WM_SP_KIND_YOKO_SALT);
        else if (strstr(label, "reaper") != 0)
            (void)live_spawn_special(a, WM_SP_KIND_TAKER_REAPER);
        else if (strstr(label, "spirit") != 0)
            (void)live_spawn_special(a, WM_SP_KIND_TAKER_SPIRIT);
        else if (strstr(label, "pie") != 0)
            (void)live_spawn_special(a, WM_SP_KIND_DOINK_PIE);
        else if (strstr(label, "fireball") != 0 || strstr(label, "fire_ball") != 0)
            (void)live_spawn_special(a, WM_SP_KIND_BAM_FIREBALL);
    }
}

/* Combat2CZ: direct WRESTLE.ASM::execute_walk/change_walk_anim/set_velocities.
   The earlier BN translation only set X/Z velocity and omitted the source
   change_anim1/change_anim2 calls.  That leaves a completed attack frame on
   screen while DRONE input keeps moving the actor -- the frozen-pose glide
   seen in the attract capture. */
static const char *source_walk_prefix(const wm_arcade_actor_t *a)
{
    static const char *const p[9]={"hrt","rzr","und","yok","shn","bam","dnk","dnk","lex"};
    if(!a || a->wrestler_num<0 || a->wrestler_num>8) return "dnk";
    return p[a->wrestler_num];
}
static const char *source_prefixed_anim(const wm_arcade_actor_t *a,const char *suffix)
{
    static char slots[4][64]; static unsigned slot;
    char *b=slots[(slot++)&3u];
    (void)snprintf(b,64,"%s_%s",source_walk_prefix(a),suffix?suffix:"");
    return b;
}
static const char *source_rotate_anim_label(const wm_arcade_actor_t *a)
{
    static const char *const s[4][4]={
      {"stand2_anim","2_to_4_turn_anim","2_to_6_turn_anim","2_to_8_turn_anim"},
      {"4_to_2_turn_anim","stand4_anim","4_to_6_turn_anim","4_to_8_turn_anim"},
      {"6_to_2_turn_anim","6_to_4_turn_anim","stand6_anim","6_to_8_turn_anim"},
      {"8_to_2_turn_anim","8_to_4_turn_anim","8_to_6_turn_anim","stand8_anim"}};
    int f=(int)wm_arcade_convert_facing((uint16_t)a->facing_dir)>>1;
    int n=(int)wm_arcade_convert_facing((uint16_t)a->new_facing_dir)>>1;
    if (f < 0 || f > 3) {
        f = 0;
    }
    if (n < 0 || n > 3) {
        n = f;
    }
    return source_prefixed_anim(a,s[f][n]);
}
static const char *source_leg_walk_label(const wm_arcade_actor_t *a,int mi,int fi)
{
    static const char *const s[8][8]={
      {"walk1_f2_anim","walk1_f2_anim","walk1_f4_anim","walk1_f4_anim","walk1_f4_anim","walk1_f4_anim","walk1_f2_anim","walk1_f2_anim"},
      {"walk2_f2_anim","walk2_f2_anim","walk2_f2_anim","walk2_f4_anim","walk8_f4_anim","walk8_f4_anim","walk4_f2_anim","walk4_f2_anim"},
      {"walk2_f2_anim","walk2_f2_anim","walk2_f2_anim","walk4_f4_anim","walk4_f4_anim","walk8_f4_anim","walk6_f2_anim","walk6_f2_anim"},
      {"walk2_f2_anim","walk8_f2_anim","walk4_f4_anim","walk4_f4_anim","walk2_f4_anim","walk6_f4_anim","walk2_f2_anim","walk6_f2_anim"},
      {"walk5_f2_anim","walk5_f2_anim","walk5_f4_anim","walk5_f4_anim","walk5_f4_anim","walk5_f4_anim","walk5_f2_anim","walk5_f2_anim"},
      {"walk2_f2_anim","walk6_f2_anim","walk2_f2_anim","walk6_f4_anim","walk2_f4_anim","walk4_f4_anim","walk2_f2_anim","walk8_f2_anim"},
      {"walk2_f2_anim","walk6_f2_anim","walk6_f2_anim","walk8_f4_anim","walk4_f4_anim","walk4_f4_anim","walk2_f2_anim","walk2_f2_anim"},
      {"walk2_f2_anim","walk4_f2_anim","walk6_f2_anim","walk8_f4_anim","walk6_f4_anim","walk2_f4_anim","walk2_f2_anim","walk2_f2_anim"}};
    if(mi<0||mi>7||fi<0||fi>7)return 0;
    return source_prefixed_anim(a,s[mi][fi]);
}
static const char *source_torso_walk_label(const wm_arcade_actor_t *a,int f4,int n4)
{
    static const char *const s[4][4]={
      {"torso2_anim","2_to_4_turn2_anim","2_to_6_turn2_anim","2_to_8_turn2_anim"},
      {"4_to_2_turn2_anim","torso4_anim","4_to_6_turn2_anim","4_to_8_turn2_anim"},
      {"6_to_2_turn2_anim","6_to_4_turn2_anim","torso6_anim","6_to_8_turn2_anim"},
      {"8_to_2_turn2_anim","8_to_4_turn2_anim","8_to_6_turn2_anim","torso8_anim"}};
    if(f4<0||f4>3||n4<0||n4>3)return 0;
    return source_prefixed_anim(a,s[f4][n4]);
}
static void source_execute_walk(wm_arcade_actor_t *a, void *user)
{
    static const int32_t vel[8][2] = {
        {0, -0x3a000}, {0x31000, -0x31000}, {0x3a000, 0}, {0x31000, 0x31000},
        {0, 0x3a000}, {-0x31000, 0x31000}, {-0x3a000, 0}, {-0x31000, -0x31000}
    };
    uint16_t d; int idx,fi,f4,n4,ai; int32_t xv,zv; wm_arcade_actor_t *opp;
    (void)user; if(!a)return;

    /* WRESTLE.ASM: INTURN suppresses normal walk processing, but at stick rest
       still clears stale velocity.  Secondary INTURN is independent. */
    ai=actor_index(a);
    if((a->anim_mode&WM_ARCADE_MODE_INTURN) ||
       (ai>=0 && (g.source_torso[ai].mode_shadow&WM_ARCADE_MODE_INTURN))){
        if((a->move_dir&0x0f)==0){a->x_vel=0;a->z_vel=0;}
        return;
    }
    a->attack_type=0;
    d=(uint16_t)(a->move_dir&0x0fu);
    if(d==WM_MOVE_ZIP || d==3u || d==7u || d>=11u){
        a->move_dir=WM_MOVE_ZIP;a->x_vel=0;a->z_vel=0;
        common_anim_label(a,source_rotate_anim_label(a),0);
        return;
    }
    if(d&WM_MOVE_LEFT)a->obj_control|=WM_OBJ_FLIPH;
    else if(d&WM_MOVE_RIGHT)a->obj_control&=(uint16_t)~WM_OBJ_FLIPH;
    idx=(int)wm_arcade_convert_facing(d);
    if(idx<0||idx>7){a->x_vel=0;a->z_vel=0;return;}
    xv=vel[idx][0];zv=vel[idx][1];opp=a->smart_target;
    if(!a->walk_fast&&opp&&opp->player_mode!=WM_PMODE_ONGROUND&&opp->player_mode!=WM_PMODE_DEAD){
        uint16_t xpair=(uint16_t)((d|a->facing_dir)&(WM_MOVE_LEFT|WM_MOVE_RIGHT));
        uint16_t zpair=(uint16_t)((d|a->facing_dir)&(WM_MOVE_UP|WM_MOVE_DOWN));
        if(xpair==(WM_MOVE_LEFT|WM_MOVE_RIGHT))xv=(xv*230)>>8;
        if(zpair==(WM_MOVE_UP|WM_MOVE_DOWN))zv=(zv*230)>>8;
    }else if(!a->walk_fast&&opp&&(opp->player_mode==WM_PMODE_ONGROUND||opp->player_mode==WM_PMODE_DEAD))xv=(xv*384)>>8;
    a->x_vel=xv;a->z_vel=zv;

    /* WRESTLE.ASM::change_walk_anim.  Directional label matrices are common
       across all eight characters; only the source prefix differs. */
    a->consecutive_hits=0;
    a->ani_speed=(uint16_t)((a->walk_fast || (opp&&opp->player_mode==WM_PMODE_ONGROUND))?0xcd:0x100);
    fi=(int)wm_arcade_convert_facing((uint16_t)a->facing_dir);
    f4=fi>>1;n4=((int)wm_arcade_convert_facing((uint16_t)a->new_facing_dir))>>1;
    ai=actor_index(a);
    if(ai>=0 && !(g.source_torso[ai].mode_shadow&WM_ARCADE_MODE_UNINT))
        common_torso_label(a,source_torso_walk_label(a,f4,n4),0);
    common_anim_label(a,source_leg_walk_label(a,idx,fi),0);
}

/* Combat2CX: source callback wrapper is injected before the runtime's
   concrete drone callback definition, so provide the C prototype first. */
static int drone_check_combo_go(wm_arcade_actor_t *actor, void *user);

static void source_count_button_presses(wm_arcade_actor_t *a)
{
    uint16_t d;
    if (!a) return;
    d=a->but_val_down;
    if (d & WM_BTN_PUNCH)  ++a->punchb_count;
    if (d & WM_BTN_BLOCK)  ++a->blockb_count;
    if (d & WM_BTN_SPUNCH) ++a->spunchb_count;
    if (d & WM_BTN_KICK)   ++a->kickb_count;
    if (d & WM_BTN_SKICK)  ++a->skickb_count;
}
static int source_check_combo_go_port(wm_arcade_actor_t *a, void *user)
{
    return drone_check_combo_go(a,user);
}
static void source_adjust_health_port(wm_arcade_actor_t *a, int delta, void *user)
{
    int32_t v; (void)user; if(!a)return;
    v=a->life+delta; if(v<0)v=0; if(v>100)v=100; a->life=v;
}
static void source_set_raisearm_bit_port(wm_arcade_actor_t *a, void *user)
{
    (void)user; if(a)a->status_flags|=WM_STATUS_DID_RAISEARM;
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
    WmFix39ActorTrace *tr=trace_for(a); const char *label=wm_source_bret_anim_label((int)id); (void)user; if(id==WM_BRET_ANIM_GRABFLING_FACE24)label=(a && (a->facing_dir&WM_MOVE_UP))?"hrt_2_grabfling_anim":"hrt_4_grabfling_anim";
    if(tr){tr->animation_token=(int32_t)id;tr->animation_label=label;++tr->animation_events;}
    if(label)(void)live_start_source_anim(a,label,false);
}

static void bret_torso(wm_arcade_actor_t *a, wm_arcade_bret_anim_id_t id, void *user)
{
    WmFix39ActorTrace *tr=trace_for(a); const char *label=wm_source_bret_anim_label((int)id); (void)user; if(id==WM_BRET_ANIM_GRABFLING_FACE24)label=(a && (a->facing_dir&WM_MOVE_UP))?"hrt_2_grabfling_anim":"hrt_4_grabfling_anim";
    if(tr){tr->torso_animation_token=(int32_t)id;tr->torso_animation_label=label;++tr->animation_events;}
    if(label)(void)live_start_source_anim(a,label,true);
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
    WmFix39ActorTrace *tr=trace_for(a); const char *label=wm_source_razor_anim_label((int)id); (void)user;
    if(tr){tr->animation_token=(int32_t)id;tr->animation_label=label;++tr->animation_events;}
    if(label)(void)live_start_source_anim(a,label,false);
}

static void razor_torso(wm_arcade_actor_t *a, wm_arcade_razor_anim_id_t id, void *user)
{
    WmFix39ActorTrace *tr=trace_for(a); const char *label=wm_source_razor_anim_label((int)id); (void)user;
    if(tr){tr->torso_animation_token=(int32_t)id;tr->torso_animation_label=label;++tr->animation_events;}
    if(label)(void)live_start_source_anim(a,label,true);
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
    .check_combo_go = source_check_combo_go_port,
    .adjust_health = source_adjust_health_port,
    .set_raisearm_bit = source_set_raisearm_bit_port,
    .execute_walk = source_execute_walk,
    .change_anim_label = common_anim_label,
    .change_torso_label = common_torso_label,
    .sound_label = common_sound_label,
    .master_keep_attached = source_master_keep_attached,
    .keep_attached = source_keep_attached,
    .start_special_label = common_start_special_label,
    .user = 0
};

static const wm_arcade_bret_callbacks_t bret_callbacks = {
    .check_combo_go = source_check_combo_go_port,
    .adjust_health = source_adjust_health_port,
    .set_raisearm_bit = source_set_raisearm_bit_port,
    .execute_walk = source_execute_walk,
    .change_anim = bret_anim,
    .change_torso_anim = bret_torso,
    .sound = bret_sound,
    .master_keep_attached = source_master_keep_attached,
    .keep_attached = source_keep_attached,
    .user = 0
};

static const wm_arcade_razor_callbacks_t razor_callbacks = {
    .check_combo_go = source_check_combo_go_port,
    .adjust_health = source_adjust_health_port,
    .set_raisearm_bit = source_set_raisearm_bit_port,
    .execute_walk = source_execute_walk,
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

/* Combat2BL: direct translation of DRONE.ASM::drone_seekdirdist,
   #drn_getxz and drone_seekxz. DRN_SEEKDIR is a 16-way angular offset
   around the opponent; DRN_SEEKDIST selects the source 50-pixel radius
   bands. It is NOT a "walk toward opponent / reverse when hanging back"
   boolean. */
static const int16_t drone_seek_sine_t[6][20] = {
    {-50,-46,-35,-19,0,19,35,46,50,46,35,19,0,-19,-35,-46,-50,-46,-35,-19},
    {-100,-92,-71,-38,0,38,71,92,100,92,71,38,0,-38,-71,-92,-100,-92,-71,-38},
    {-150,-139,-106,-57,0,57,106,139,150,139,106,57,0,-57,-106,-139,-150,-139,-106,-57},
    {-200,-185,-141,-76,0,76,141,185,200,185,141,76,0,-76,-141,-185,-200,-185,-141,-76},
    {-250,-231,-177,-95,0,95,177,231,250,231,177,95,0,-95,-177,-231,-250,-231,-177,-95},
    {-300,-277,-212,-115,0,114,212,277,300,277,212,114,0,-114,-212,-277,-300,-277,-212,-115}
};

static bool drone_seek_source_target(const wm_arcade_actor_t *opp,
                                     int seek_dir, int seek_dist,
                                     int32_t *tx, int32_t *tz)
{
    int d = seek_dist;
    int a = seek_dir & 15;
    int32_t x, z;
    if (!opp || !tx || !tz) return false;
    if (d < 0) d = 0;
    if (d > 5) d = 5;
    /* DRONE.ASM #drn_getxz: Z uses sine_t[a], X uses sine_t[a+4]. */
    z = opp->z_int + drone_seek_sine_t[d][a];
    x = opp->x_int + drone_seek_sine_t[d][a + 4];
    /* Exact #drn_getxz ring guards: RING_X_CENTER +/- 220, RING_TOP/BOT. */
    if (x < (1074 - 220) || x > (1074 + 220) || z < 1023 || z > 1345)
        return false;
    *tx = x; *tz = z; return true;
}

static uint16_t drone_seek_source_joy(const wm_arcade_actor_t *actor,
                                      int32_t tx, int32_t tz, int32_t range)
{
    int32_t dx, dz;
    uint16_t joy = WM_MOVE_ZIP;
    if (!actor) return joy;
    dx = actor->x_int - tx;
    dz = actor->z_int - tz;
    if (dx < -range) joy |= WM_MOVE_RIGHT;
    else if (dx > range) joy |= WM_MOVE_LEFT;
    if (dz < -range) joy |= WM_MOVE_DOWN;
    else if (dz > range) joy |= WM_MOVE_UP;
    return joy;
}

static void drone_seek_dir_dist(wm_arcade_actor_t *actor, wm_arcade_drone_state_t *d, void *user)
{
    wm_arcade_actor_t *opp;
    int dir, dist;
    int32_t tx=0, tz=0;
    bool valid=false;
    uint16_t oldjoy;
    if (!actor || !d) return;
    opp = actor->smart_target;
    if (!opp) { d->joy = WM_MOVE_ZIP; return; }
    oldjoy = d->joy;
    dir = d->seek_dir & 15;
    dist = d->seek_dist;

    /* DRONE.ASM tries the requested direction first, then walks outward
       +1/-1 for seven pairs until #drn_getxz returns an in-ring target. */
    valid = drone_seek_source_target(opp, dir, dist, &tx, &tz);
    if (!valid) {
        int plus=dir, minus=dir;
        for (int n=0; n<7 && !valid; ++n) {
            plus=(plus+1)&15;
            if (drone_seek_source_target(opp, plus, dist, &tx, &tz)) { dir=plus; valid=true; break; }
            minus=(minus-1)&15;
            if (drone_seek_source_target(opp, minus, dist, &tx, &tz)) { dir=minus; valid=true; break; }
        }
    }
    if (!valid) { d->joy = WM_MOVE_ZIP; return; }
    d->seek_dir = dir;
    d->joy = drone_seek_source_joy(actor, tx, tz, 30);

    /* At the seek point, modes -2/-3 choose a new +/-2/3 angular offset.
       Source restores old joy for this tick to reduce the direction glitch. */
    if (d->joy == WM_MOVE_ZIP && d->mode < -1) {
        uint32_t r = drone_rnd_upto(3u, user);
        int delta = (r==0u) ? -2 : (r==1u) ? -3 : (r==2u) ? 2 : 3;
        d->joy = oldjoy;
        d->seek_dir = (d->seek_dir + delta) & 15;
    }
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
    g.rope_render_adapter.set_image = live_rope_set_image;
    g.rope_render_adapter.user = 0;
    /* Seed renderer-visible symbols from the exact initial position-table images. */
    for (i = 0u; i < 4u; ++i) {
        unsigned ch;
        for (ch = 0u; ch < WM_FIX39_ROPE_CHANNEL_COUNT; ++ch) {
            WmRopeRuntimeChannel *rc = &g.ropes[i].channel[ch];
            if (rc->first_object_exists)
                live_rope_set_image(0, (WmRopeBank)i, (WmRopeChannel)ch,
                                    WM_FIX39_ROPE_HALF_FIRST, rc->first_image_symbol);
            if (rc->second_object_exists)
                live_rope_set_image(0, (WmRopeBank)i, (WmRopeChannel)ch,
                                    WM_FIX39_ROPE_HALF_SECOND, rc->second_image_symbol);
        }
    }
    g.status.rope_renderer_ready = true;

    wm_arcade_combat_runtime_init(&g.combat_runtime);
    memset(&g.react1_callbacks, 0, sizeof(g.react1_callbacks));
    g.react1_callbacks.change_anim = live_react_anim;
    g.react1_callbacks.play_sound = live_react_sound;
    g.react1_callbacks.get_health = live_get_health;
    g.react1_callbacks.victim_has_live_teammates = live_no_teammates;
    g.react1_callbacks.rndper_hi = live_rndper_hi;
    g.react1_callbacks.collisions_off = live_react_collisions_off;
    g.react1_callbacks.user = &g.rng;
    wm_arcade_react1_context_init(&g.react1_context, &g.react1_callbacks);

    memset(&g.react_callbacks, 0, sizeof(g.react_callbacks));
    g.react_callbacks.good_run_hit = live_good_run_hit;
    g.react_callbacks.reaction = live_reaction_dispatch;
    g.react_callbacks.adjust_health = live_adjust_health;
    g.react_callbacks.user = &g.react1_context;
    g.react_bridge.runtime = &g.combat_runtime;
    g.react_bridge.callbacks = &g.react_callbacks;
    memset(&g.combat_callbacks, 0, sizeof(g.combat_callbacks));
    g.combat_callbacks.wrestler_hit = live_wrestler_hit;
    memset(&g.special_callbacks, 0, sizeof(g.special_callbacks));
    g.special_callbacks.react = &g.react_callbacks;
    g.special_callbacks.react1 = &g.react1_context;
    init_source_anim_services();
    wm_arcade_special_lists_init(&g.special_lists);
    for (i = 0u; i < WM_FIX39_SPECIAL_SLOTS; ++i)
        wm_arcade_special_obj_init(&g.special_pool[i]);
    memset(g.ringout, 0, sizeof(g.ringout));
    g.ring_out_on = false;
    g.allow_offscreen = 0u;
    memset(&g.secret_state, 0, sizeof(g.secret_state));
    memset(&g.hiscore_backend, 0, sizeof(g.hiscore_backend));
    g.hiscore_backend_bound = false;

    /* These are deliberately false until the corresponding direct source
       service/adapter is supplied.  They prevent "wired" from meaning guessed. */
    g.status.movement_integrator_ready = true;
    g.status.animation_backend_ready = true;
    g.status.collision_boxes_ready = false;
    g.status.camera_onscreen_inputs_ready = false; /* becomes true with presenter/platform pose */
    g.status.ring_line_services_ready = true;
    g.status.secret_input_scheduler_ready = true;
    g.status.health_service_ready = true;
    g.status.audio_label_service_ready = false;
    g.status.special_spawn_command_service_ready = true;
    g.status.ringout_operator_state_ready = true;
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
        int p1_wrestler = wm_fix39_frontend_to_arcade_roster(frontend_p1);
        int p2_wrestler = wm_fix39_frontend_to_arcade_roster(frontend_p2);
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
    memset(g.presenter_pose, 0, sizeof(g.presenter_pose));
    memset(g.presenter_attack, 0, sizeof(g.presenter_attack));
    g.status.collision_boxes_ready = false;
    g.actor_ptrs[0] = &g.actors[0];
    g.actor_ptrs[1] = &g.actors[1];
    g.actors[0].smart_target = &g.actors[1];
    g.actors[1].smart_target = &g.actors[0];
    refresh_distances();
    memset(g.ringout, 0, sizeof(g.ringout));
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        g.ringout[i].ring_time = 0;
        live_ringout_refresh(i);
    }
    g.allow_offscreen = 0u;

    /* Combat2CG: animation bytecode/tables are streamed from DragonFS.
       Drop the previous match cache only at a match boundary, never while a
       live runtime still owns a program pointer. */
    wm_source_anim_program_cache_reset();
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {
        wm_source_anim_runtime_init(&g.source_anim[i]);
        wm_source_anim_runtime_init(&g.source_torso[i]);
        wm_source_anim_runtime_bind(&g.source_anim[i],&g.source_anim_services);
        wm_source_anim_runtime_bind(&g.source_torso[i],&g.source_anim_services);
        wm_source_anim_runtime_set_secondary(&g.source_torso[i], true);
    }
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {
        wm_source_anim_runtime_init(&g.source_anim[i]);
        wm_source_anim_runtime_init(&g.source_torso[i]);
    }
    init_source_character_animation(&g.actors[0]);
    init_source_character_animation(&g.actors[1]);
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i)
        if (!wm_source_anim_runtime_frame(&g.source_anim[i]))
            (void)live_start_source_anim(&g.actors[i],live_default_stand_label(&g.actors[i]),false);
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i)
        if (!wm_source_anim_runtime_frame(&g.source_anim[i]))
            (void)live_start_source_anim(&g.actors[i],live_default_stand_label(&g.actors[i]),false);

    g.old_p1_buttons = 0u;
    g.old_p1_stick = 0u;
    g.match_cpu_vs_cpu = false;
    /* WRESTLE.ASM::init_scroller, normal 1v1/1v3 case. */
    g.camera.worldtlx_fp16 = (WM_RING_X_CENTER - 200) << 16;
    g.camera.worldtly_fp16 = -(27 << 16);
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
    g.status.combat_checkhit_ticks = 0u;
    g.status.combat_attack_boxes_built = 0u;
    g.status.combat_x_overlap_ticks = 0u;
    g.status.combat_y_overlap_ticks = 0u;
    g.status.combat_z_overlap_ticks = 0u;
    g.status.combat_full_overlap_ticks = 0u;
    g.status.combat_full_overlap_rejected = 0u;
    g.status.combat_accepted_hits = 0u;
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

    /* WRESTLE.ASM loop: ARE_WE_IN_RING is refreshed before the movement
       services.  The exact line service is now source-backed. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i)
        g.actors[i].in_ring = wm_ring_inring_field(
            (int16_t)g.actors[i].x_int, (int16_t)g.actors[i].z_int);

    /* WRESTLE.ASM wrestler_main exact control-order seam:
       set_collision_boxes/confine precede update_newfacing; update_newfacing and
       closest-position state MUST precede drone_main.  Earlier revisions ran
       DRONE first, so AI consumed stale facing/distance and could continually
       seek away from its opponent. */
    live_source_face_opponents();
    refresh_distances();

    /* WRESTLE.ASM::wrestler_main calls update_newfacing before drone_main.
       Keep the translated actor facing/world direction current before DRONE
       chooses seek/action input and before character move dispatch consumes it. */
    live_source_face_opponents();
    refresh_distances();

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


    /* WRESTLE.ASM count_button_presses: live per-wrestler counters consumed
       by animation/native sequence logic. */
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) source_count_button_presses(&g.actors[i]);

    /* ARE_WE_IN_RING / ring-out timing and health are already ported; keep
       their persistent per-player RING_TIME state live every match tick. */
    live_ringout_tick();

    /* The already-ported keep_onscreen service is now in the live source
       order. Its camera input now comes from translated DISPLAY.ASM WORLDTLX;
       other platforms can still call
       wm_fix39_keep_onscreen_before_velocity with their exact DISPLAY state. */
    live_keep_onscreen_from_camera();
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        wm_arcade_wrestler_veladd(&g.actors[i], false, false);
        wm_arcade_wrestler_friction(&g.actors[i]);
    }
    /* ANIM.ASM::animate_wrestler: primary and secondary animation state
       advance here, before move_wrestler, exactly as WRESTLE.ASM orders it. */
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {
        wm_source_anim_runtime_tick(&g.source_anim[i],&g.actors[i]);
        wm_source_anim_runtime_tick(&g.source_torso[i],&g.actors[i]);
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
        wm_rope_runtime_tick(&g.ropes[i], &g.rope_render_adapter);
        ++g.status.rope_process_ticks;
    }

    /* ANIM.ASM owns attack windows through ANI_ATTACK_ON/OFF.  The current
       WIMP frame contributes only exact IANI3 hurt-box geometry. */
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {
        wm_arcade_actor_t *a=&g.actors[i];
        const char *frame=wm_source_anim_runtime_frame(&g.source_anim[i]);
        const wm_source_sprite *spr=frame?wm_character_sprite_find((uint8_t)a->wrestler_num,frame):0;
        wm_arcade_frame_box_t fb;
        if(spr && wm_arcade_wimp_frame_box_from_sprite(spr,&fb)){g.frame_box[i]=fb;g.frame_box_valid[i]=true;}
        else g.frame_box_valid[i]=false;
    }
    /* WRESTLE.ASM update_links: break a one-way stale attachment. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        wm_arcade_actor_t *a = &g.actors[i];
        if (a->attach_proc && a->attach_proc->attach_proc != a)
            a->attach_proc = 0;
    }

    /* WRESTLE.ASM master_keep_attached after overlap_collision. */
    for (i=0u;i<WM_FIX39_ACTOR_COUNT;++i) {
        wm_arcade_actor_t *a=&g.actors[i];
        if (a->anim_mode & WM_ARCADE_MODE_KEEPATTACHED)
            (void)wm_arcade_master_keep_attached(a);
    }

    /* WRESTLE.ASM set_wrestler_xflip, skipped only for MODE_NOAUTOFLIP. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        wm_arcade_actor_t *a = &g.actors[i];
        if (!(a->anim_mode & WM_ARCADE_MODE_NOAUTOFLIP)) {
            if (a->facing_dir & WM_MOVE_RIGHT) a->obj_control &= (uint16_t)~WM_OBJ_FLIPH;
            else a->obj_control |= WM_OBJ_FLIPH;
        }
    }

    /* WRESTLE.ASM wrestler_main countdown tail.  GETUP_TIME is recovery
       state only; ANI_WAITROLL/ANI_GETUP_WAIT and ANI_CHANGEANIM own the actual
       get-up animation.  Do not force PLYRMODE/stand from this loop. */
    for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
        wm_arcade_actor_t *a = &g.actors[i];
        if (a->delay_butns > 0) --a->delay_butns;
        if (a->safe_time > 0) --a->safe_time;
        if (a->delay_meter > 0) --a->delay_meter;
        if (a->immobilize_time > 0) --a->immobilize_time;
        if (a->walk_fast > 0) --a->walk_fast;
        if (a->getup_time > 0) {
            /* Source rejects a newly-set recovery while DELAY_METER is active. */
            if (a->delay_meter > 0) a->getup_time = 0;
            else {
                --a->getup_time;
                if (a->getup_time > 0) {
                    uint16_t pressed=(uint16_t)(a->but_val_down|a->stick_val_down);
                    uint32_t sf=a->status_flags;
                    bool press_last=(sf&WM_STATUS_PRESS_LAST)!=0;
                    if(pressed)sf|=WM_STATUS_PRESS_LAST;else sf&=~WM_STATUS_PRESS_LAST;
                    a->status_flags=sf;
                    if(pressed||press_last){a->getup_time-=3;if(a->getup_time<0)a->getup_time=0;}
                }
            }
            if(a->getup_time==0){a->dizzy=0;a->stars_flag=0;a->delay_butns=40;}
        }
    }

    /* SPECIAL.ASM process state and COLLIS.ASM object collisions are already
       directly ported. Tick any live source-spawned objects every match tick,
       then include them in the same collision pass as wrestlers. */
    for (i = 0u; i < WM_FIX39_SPECIAL_SLOTS; ++i) {
        if (g.special_pool[i].in_list) {
            wm_arcade_special_tick_source_state(&g.special_pool[i]);
            wm_arcade_special_velocity_add(&g.special_pool[i]);
            wm_arcade_special_standard_bounce(&g.special_pool[i]);
            ++g.status.special_process_ticks;
        }
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
        /* WRESTLE.ASM overlap_collision runs after move_wrestler/update_links
           and a fresh set_collision_boxes.  Resolve both source actors in the
           same ordered pass; do not synthesize boxes when source IANI3 is absent. */
        (void)wm_arcade_resolve_overlap(&g.actors[0], &g.actors[1]);
        (void)wm_arcade_resolve_overlap(&g.actors[1], &g.actors[0]);
        g.combat_runtime.pcnt = g.status.pcnt;
        g.combat_runtime.round_tickcount = g.status.round_tickcount;
        {
            int hit;
            bool had_full_overlap = false;
            for (i = 0u; i < WM_FIX39_ACTOR_COUNT; ++i) {
                if (g.actors[i].anim_mode & WM_ARCADE_MODE_CHECKHIT) {
                    size_t vi = i ^ 1u;
                    wm_arcade_actor_t *attacker = &g.actors[i];
                    wm_arcade_actor_t *victim = &g.actors[vi];
                    bool xo, yo, zo;
                    ++g.status.combat_checkhit_ticks;
                    wm_arcade_set_attack_box(attacker);
                    ++g.status.combat_attack_boxes_built;
                    xo = !(attacker->attack_box.x1 > victim->hurt_box.x2 ||
                           attacker->attack_box.x2 < victim->hurt_box.x1);
                    yo = !(attacker->attack_box.y1 > victim->hurt_box.y2 ||
                           attacker->attack_box.y2 < victim->hurt_box.y1);
                    zo = !(attacker->attack_box.z1 > victim->hurt_box.z2 ||
                           attacker->attack_box.z2 < victim->hurt_box.z1);
                    if (xo) ++g.status.combat_x_overlap_ticks;
                    if (yo) ++g.status.combat_y_overlap_ticks;
                    if (zo) ++g.status.combat_z_overlap_ticks;
                    if (xo && yo && zo) {
                        ++g.status.combat_full_overlap_ticks;
                        had_full_overlap = true;
                    }
                }
            }
            hit = wm_arcade_check_wrestler_collisions(
                g.actor_ptrs, WM_FIX39_ACTOR_COUNT,
                g.status.round_tickcount, &g.combat_callbacks);
            if (hit) ++g.status.combat_accepted_hits;
            else if (had_full_overlap) ++g.status.combat_full_overlap_rejected;
            {
                wm_arcade_special_collision_result_t sp = wm_arcade_object_collisions(
                    &g.special_lists, g.actor_ptrs, WM_FIX39_ACTOR_COUNT,
                    &g.combat_runtime, &g.special_callbacks);
                if (sp.wrestler_hits > 0)
                    g.status.combat_accepted_hits += (uint32_t)sp.wrestler_hits;
            }
        }
        ++g.status.combat_collision_ticks;
    }

    /* Remaining genuine source seams are now limited to data/interpreters not
       yet ported: native ANI_CODE routines outside ANIM.ASM and platform presentation/
       audio backends. Ported ring-out, keep-onscreen, SPECIAL process/collision,
       health and reaction services above are all on the live tick path. */

    /* WRESTLE.ASM main loop calls scroll_world after the match work and
       before incrementing round_tickcount. */
    live_scroll_world_attract();

    g.combat_runtime.pcnt = g.status.pcnt;
    ++g.status.pcnt;
    ++g.status.round_tickcount;
}

const wm_arcade_actor_t *wm_fix39_actor(size_t index)
{
    if (index >= WM_FIX39_ACTOR_COUNT) return 0;
    return &g.actors[index];
}

const char *wm_fix39_actor_source_frame(size_t index)
{
    if(!g.status.initialized)wm_fix39_runtime_init();
    if(index>=WM_FIX39_ACTOR_COUNT)return 0;
    return wm_source_anim_runtime_frame(&g.source_anim[index]);
}
const char *wm_fix39_actor_source_torso_frame(size_t index)
{
    if(!g.status.initialized)wm_fix39_runtime_init();
    if(index>=WM_FIX39_ACTOR_COUNT)return 0;
    return wm_source_anim_runtime_frame(&g.source_torso[index]);
}

const char *wm_fix39_actor_source_anim(size_t index)
{
    if(!g.status.initialized)wm_fix39_runtime_init();
    if(index>=WM_FIX39_ACTOR_COUNT)return 0;
    return wm_source_anim_runtime_label(&g.source_anim[index]);
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

void wm_fix39_match_sync_presenter_pose(size_t index, int32_t x, int32_t z,
                                        bool flip_x, bool blocking)
{
    /* Compatibility/diagnostic API only. Presentation must never mutate
       WRESTLE.ASM actor position, facing, blocking, or player mode. */
    if (index >= WM_FIX39_ACTOR_COUNT) return;
    g.presenter_pose[index].valid = true;
    g.presenter_pose[index].x = x;
    g.presenter_pose[index].z = z;
    g.presenter_pose[index].flip_x = flip_x;
    g.presenter_pose[index].blocking = blocking;
}

void wm_fix39_match_bind_source_frame_attack(size_t index, uint8_t roster_id,
                                             const char *source_frame)
{
    wm_arcade_actor_t *a; wm_arcade_attack_on_z_args_t zargs; wm_arcade_attack_on_args_t args; bool uses_z=false;
    if(index>=WM_FIX39_ACTOR_COUNT)return;
    a=&g.actors[index];
    if(wm_arcade_character_attack_for_source_frame(roster_id,source_frame,&uses_z,&zargs,&args)){if(uses_z)wm_arcade_ani_attack_on_z(a,&zargs);else wm_arcade_ani_attack_on(a,&args);}
    else if(a->anim_mode&WM_ARCADE_MODE_CHECKHIT)wm_arcade_ani_attack_off(a,g.status.round_tickcount);
}

void wm_fix39_match_bind_bret_source_frame_attack(size_t index, const char *source_frame) {
    wm_fix39_match_bind_source_frame_attack(index, 0u, source_frame);
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


bool wm_fix39_match_spawn_special(size_t owner_index,
                                  wm_arcade_special_kind_t kind)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (!g.status.match_started || owner_index >= WM_FIX39_ACTOR_COUNT)
        return false;
    return live_spawn_special(&g.actors[owner_index], kind);
}

void wm_fix39_match_set_ringout_enabled(bool enabled)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    g.ring_out_on = enabled;
    g.status.ringout_operator_state_ready = true;
}

void wm_fix39_secret_begin(uint32_t tsec_ticks)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    wm_attract_secret_begin(&g.secret_state, tsec_ticks);
    g.status.secret_input_scheduler_ready = true;
}

bool wm_fix39_secret_tick(bool has_button, WmAttractSecretButton button)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    return wm_attract_secret_tick(&g.secret_state, has_button, button);
}

void wm_fix39_hiscore_bind_persistence(const WmHsSaveBackend *backend)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    memset(&g.hiscore_backend, 0, sizeof(g.hiscore_backend));
    if (backend != 0) {
        g.hiscore_backend = *backend;
        g.hiscore_backend_bound = backend->read != 0 && backend->write != 0;
    } else {
        g.hiscore_backend_bound = false;
    }
    g.status.hiscore_persistence_ready = g.hiscore_backend_bound;
}

WmHsLoadResult wm_fix39_hiscore_load_bound(uint32_t adjusted_reset_value)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (!g.hiscore_backend_bound) return WM_HS_LOAD_IO_ERROR;
    return wm_hs_save_read(&g.hiscore, &g.hiscore_backend, adjusted_reset_value);
}

bool wm_fix39_hiscore_save_bound(void)
{
    if (!g.status.initialized) wm_fix39_runtime_init();
    if (!g.hiscore_backend_bound) return false;
    return wm_hs_save_write(&g.hiscore, &g.hiscore_backend);
}

const WmFix39Status *wm_fix39_status(void)
{
    return &g.status;
}
