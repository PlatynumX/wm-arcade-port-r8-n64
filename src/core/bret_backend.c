#include "wm/bret_backend.h"
#include "wm/anim_frame_commands.h"
#include "wm/arcade/wm_arcade_start_run.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_mode_dead.h"
#include "wm/bret_visuals.h"
#include <string.h>

void wm_bret_backend_init(wm_bret_backend_actor *bva) {
    if (!bva) return;
    memset(bva, 0, sizeof(*bva));
}

/* BRET.ASM:2897 hrt_leg_anims_table, transcribed value-for-value.
   leg_table[move_compass][facing_compass], both wm_convert_facing() 0-7
   results, matching WRESTLE.ASM::change_walk_anim's move_compass*8+
   facing_compass addressing (WRESTLE.ASM:5000-5014). */
static const wm_visual_sequence *const leg_table[8][8] = {
    /* MOVE=UP */
    { &wm_bret_walk1_f2_anim, &wm_bret_walk1_f2_anim, &wm_bret_walk1_f4_anim, &wm_bret_walk1_f4_anim,
      &wm_bret_walk1_f4_anim, &wm_bret_walk1_f4_anim, &wm_bret_walk1_f2_anim, &wm_bret_walk1_f2_anim },
    /* MOVE=UP-RIGHT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f4_anim,
      &wm_bret_walk8_f4_anim, &wm_bret_walk8_f4_anim, &wm_bret_walk4_f2_anim, &wm_bret_walk4_f2_anim },
    /* MOVE=RIGHT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk4_f4_anim,
      &wm_bret_walk4_f4_anim, &wm_bret_walk8_f4_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk6_f2_anim },
    /* MOVE=DOWN-RIGHT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk8_f2_anim, &wm_bret_walk4_f4_anim, &wm_bret_walk4_f4_anim,
      &wm_bret_walk2_f4_anim, &wm_bret_walk6_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk6_f2_anim },
    /* MOVE=DOWN */
    { &wm_bret_walk5_f2_anim, &wm_bret_walk5_f2_anim, &wm_bret_walk5_f4_anim, &wm_bret_walk5_f4_anim,
      &wm_bret_walk5_f4_anim, &wm_bret_walk5_f4_anim, &wm_bret_walk5_f2_anim, &wm_bret_walk5_f2_anim },
    /* MOVE=DOWN-LEFT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk6_f4_anim,
      &wm_bret_walk2_f4_anim, &wm_bret_walk4_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk8_f2_anim },
    /* MOVE=LEFT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk8_f4_anim,
      &wm_bret_walk4_f4_anim, &wm_bret_walk4_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim },
    /* MOVE=UP-LEFT */
    { &wm_bret_walk2_f2_anim, &wm_bret_walk4_f2_anim, &wm_bret_walk6_f2_anim, &wm_bret_walk8_f4_anim,
      &wm_bret_walk6_f4_anim, &wm_bret_walk2_f4_anim, &wm_bret_walk2_f2_anim, &wm_bret_walk2_f2_anim },
};

const wm_visual_sequence *wm_bret_leg_anim(int move_compass, int facing_compass) {
    if (move_compass < 0 || move_compass > 7 || facing_compass < 0 || facing_compass > 7)
        return NULL;
    return leg_table[move_compass][facing_compass];
}

/*
 * BRET.ASM:2871 hrt_rotate_anims_table[old FACING_DIR diag][new
 * NEW_FACING_DIR diag] (the "TURNS (STANDS)" block), transcribed
 * value-for-value including its real SUBR aliasing (WRESTLE.ASM's own
 * table literally reuses the reverse-rotation's address, e.g.
 * hrt_6_to_8_turn_anim IS hrt_4_to_2_turn_anim, not merely equivalent --
 * confirmed by reading HRTSEQ1.ASM directly, not guessed): diag2's row is
 * diag1's row reversed, and diag3's row is diag0's reversed, because
 * turning e.g. down-right<->down-left is the mirror of up-right<->up-left.
 * Played by set_rotate_anim/change_anim1 for the idle (#zip) facing-change
 * case -- see wm_bret_backend_execute_walk's own comment for how the two
 * diag indices are derived (old FACING_DIR, not old MOVE_DIR).
 */
static const wm_visual_sequence *const rotate_table[4][4] = {
    /* old = UP/UP_RIGHT (diag0) */
    { &wm_bret_stand2_anim,     &wm_bret_2_to_4_turn_anim, &wm_bret_2_to_6_turn_anim, &wm_bret_2_to_8_turn_anim },
    /* old = RIGHT/DOWN_RIGHT (diag1) */
    { &wm_bret_4_to_2_turn_anim, &wm_bret_stand4_anim,     &wm_bret_4_to_6_turn_anim, &wm_bret_4_to_8_turn_anim },
    /* old = DOWN/DOWN_LEFT (diag2) == hrt_stand6_anim's row, HRTSEQ1.ASM:75 alias of stand4 */
    { &wm_bret_4_to_8_turn_anim, &wm_bret_4_to_6_turn_anim, &wm_bret_stand4_anim,     &wm_bret_4_to_2_turn_anim },
    /* old = LEFT/UP_LEFT (diag3) == hrt_stand8_anim's row, HRTSEQ1.ASM:50 alias of stand2 */
    { &wm_bret_2_to_8_turn_anim, &wm_bret_2_to_6_turn_anim, &wm_bret_2_to_4_turn_anim, &wm_bret_stand2_anim },
};

const wm_visual_sequence *wm_bret_rotate_anim(int old_facing_compass, int new_facing_compass) {
    int old_diag, new_diag;
    if (old_facing_compass < 0 || old_facing_compass > 7) return NULL;
    if (new_facing_compass < 0 || new_facing_compass > 7) return NULL;
    old_diag = old_facing_compass >> 1;
    new_diag = new_facing_compass >> 1;
    return rotate_table[old_diag][new_diag];
}

static bool is_leg_turn_sequence(const wm_visual_sequence *seq) {
    return seq == &wm_bret_2_to_4_turn_anim || seq == &wm_bret_4_to_2_turn_anim ||
           seq == &wm_bret_4_to_6_turn_anim || seq == &wm_bret_2_to_8_turn_anim ||
           seq == &wm_bret_4_to_8_turn_anim || seq == &wm_bret_2_to_6_turn_anim;
}

/*
 * BRET.ASM:2981 hrt_torso_anims_table[FACING_DIR diag][NEW_FACING_DIR diag]
 * (the "TURNS (TORSOS)" block), same real aliasing pattern and reasoning as
 * rotate_table above (e.g. hrt_8_to_6_turn2_anim IS hrt_2_to_4_turn2_anim).
 * Played by change_walk_anim's torso half while actually walking. Unlike
 * the leg's turn anims, each of these 12 carries one or two real
 * ANI_SETFACING commands -- see torso_turn_setfacing below and
 * wm_bret_backend_tick for how FACING_DIR actually gets promoted mid-walk. */
static const wm_visual_sequence *const torso_table[4][4] = {
    { &wm_bret_torso2_anim,      &wm_bret_2_to_4_turn2_anim, &wm_bret_2_to_6_turn2_anim, &wm_bret_2_to_8_turn2_anim },
    { &wm_bret_4_to_2_turn2_anim, &wm_bret_torso4_anim,      &wm_bret_4_to_6_turn2_anim, &wm_bret_4_to_8_turn2_anim },
    { &wm_bret_4_to_8_turn2_anim, &wm_bret_4_to_6_turn2_anim, &wm_bret_torso4_anim,      &wm_bret_4_to_2_turn2_anim },
    { &wm_bret_2_to_8_turn2_anim, &wm_bret_2_to_6_turn2_anim, &wm_bret_2_to_4_turn2_anim, &wm_bret_torso2_anim },
};

const wm_visual_sequence *wm_bret_torso_anim(int facing_compass, int new_facing_compass) {
    int facing_diag, new_facing_diag;
    if (facing_compass < 0 || facing_compass > 7) return NULL;
    if (new_facing_compass < 0 || new_facing_compass > 7) return NULL;
    facing_diag = facing_compass >> 1;
    new_facing_diag = new_facing_compass >> 1;
    return torso_table[facing_diag][new_facing_diag];
}

/*
 * Shared shape for "this sequence fires an instant, once-per-crossing
 * command right before 1 or 2 specific 0-based frame indices" -- both
 * ANI_SETFACING in the torso turn2 sequences and ANI_XFLIP in the leg turn
 * sequences (both below) take this form, hand-traced the same way
 * attack_windows' active_frame_index is: the command falls right before
 * the WL frame line at that index. */
typedef struct {
    const wm_visual_sequence *seq;
    unsigned char indices[2];
    unsigned char count;
} wm_bret_frame_markers_t;

static bool frame_index_is_marked(const wm_bret_frame_markers_t *w, size_t frame_index) {
    unsigned char i;
    if (!w) return false;
    for (i = 0; i < w->count; ++i)
        if (w->indices[i] == frame_index) return true;
    return false;
}

/* HRTSEQ1.ASM:460-524: each turn2 sequence's real ANI_SETFACING command(s).
   ANI_SETFACING is ANIM.ASM's _ani_setfacing (ANIM.ASM:1242-1249): an
   unconditional FACING_DIR=NEW_FACING_DIR copy, fired live off whatever
   NEW_FACING_DIR is at that instant, not a value captured when the turn
   started. The two 4-frame sequences (4-to-8/6-to-2 and 2-to-6/8-to-4, the
   "opposite quadrant" 180-degree-ish turns) carry two such commands. */
static const wm_bret_frame_markers_t torso_turn_setfacing[] = {
    { &wm_bret_2_to_4_turn2_anim, {1, 0}, 1 },
    { &wm_bret_4_to_2_turn2_anim, {1, 0}, 1 },
    { &wm_bret_4_to_6_turn2_anim, {1, 0}, 1 },
    { &wm_bret_2_to_8_turn2_anim, {1, 0}, 1 },
    { &wm_bret_4_to_8_turn2_anim, {1, 3}, 2 },
    { &wm_bret_2_to_6_turn2_anim, {1, 3}, 2 },
};
#define WM_BRET_TORSO_TURN_SETFACING_COUNT \
    (sizeof(torso_turn_setfacing) / sizeof(torso_turn_setfacing[0]))

static const wm_bret_frame_markers_t *find_torso_turn_setfacing(const wm_visual_sequence *seq) {
    size_t i;
    if (!seq) return NULL;
    for (i = 0; i < WM_BRET_TORSO_TURN_SETFACING_COUNT; ++i)
        if (torso_turn_setfacing[i].seq == seq) return &torso_turn_setfacing[i];
    return NULL;
}

/*
 * HRTSEQ1.ASM:390-454 ("TURNS (STANDS)", the leg's own rotate_table above):
 * the real ANI_XFLIP each carries. ANI_XFLIP is ANIM.ASM's _ani_xflip
 * (ANIM.ASM:939-947): OBJ_CONTROL ^= M_FLIPH (WM_OBJ_FLIPH here). Only 4 of
 * the 6 canonical bodies carry one at all -- 2_to_4/4_to_2 (adjacent-
 * quadrant turns, e.g. up-right<->right) never cross the sprite's own
 * left/right mirror line, so they have none; confirmed by reading each
 * header directly, not assumed. */
static const wm_bret_frame_markers_t leg_turn_xflip[] = {
    { &wm_bret_4_to_6_turn_anim, {1, 0}, 1 },
    { &wm_bret_2_to_8_turn_anim, {1, 0}, 1 },
    { &wm_bret_4_to_8_turn_anim, {3, 0}, 1 },
    { &wm_bret_2_to_6_turn_anim, {3, 0}, 1 },
};
#define WM_BRET_LEG_TURN_XFLIP_COUNT \
    (sizeof(leg_turn_xflip) / sizeof(leg_turn_xflip[0]))

static const wm_bret_frame_markers_t *find_leg_turn_xflip(const wm_visual_sequence *seq) {
    size_t i;
    if (!seq) return NULL;
    for (i = 0; i < WM_BRET_LEG_TURN_XFLIP_COUNT; ++i)
        if (leg_turn_xflip[i].seq == seq) return &leg_turn_xflip[i];
    return NULL;
}

const wm_visual_sequence *wm_bret_anim_sequence(wm_arcade_bret_anim_id_t id) {
    switch (id) {
        case WM_BRET_ANIM_STAND2: return &wm_bret_stand2_anim;
        case WM_BRET_ANIM_STAND4: return &wm_bret_stand4_anim;
        case WM_BRET_ANIM_TORSO2: return &wm_bret_torso2_anim;
        case WM_BRET_ANIM_TORSO4: return &wm_bret_torso4_anim;
        case WM_BRET_ANIM_PUNCH2: return &wm_bret_light_punch2_anim;
        case WM_BRET_ANIM_PUNCH4: return &wm_bret_light_punch4_anim;
        /* HRTSEQ2.ASM:618/677 -- BRET.ASM #spunch_slap's own FACE24 pair,
           which is what do_super_punch selects. Not hrt_4_super_punch_anim:
           that is WM_BRET_ANIM_SUPER_PUNCH4 below, #scrt_cut's supercut
           target, a different animation with a different attack box. */
        case WM_BRET_ANIM_SUPER_PUNCH2_2: return &wm_bret_super_punch2_2_anim;
        case WM_BRET_ANIM_SUPER_PUNCH2_4: return &wm_bret_super_punch2_4_anim;
        case WM_BRET_ANIM_SUPER_PUNCH4: return &wm_bret_power_punch_anim;
        case WM_BRET_ANIM_KICK2: return &wm_bret_light_kick2_anim;
        case WM_BRET_ANIM_KICK4: return &wm_bret_light_kick4_anim;
        /* HRTSEQ2.ASM:1334-1335: hrt_4_super_kick_anim is a literal SUBR
           alias of hrt_2_super_kick_anim, same address -- not distinct
           artwork, so both ids resolve to the same extracted sequence. */
        case WM_BRET_ANIM_SUPER_KICK2: return &wm_bret_power_kick_anim;
        case WM_BRET_ANIM_SUPER_KICK4: return &wm_bret_power_kick_anim;
        /* HRTSEQ4.ASM:104 hrt_4_block_anim -- the only real block animation
           Bret has (its 2-facing twin is commented out in the source). */
        case WM_BRET_ANIM_BLOCK4: return &wm_bret_block4_anim;
        case WM_BRET_ANIM_BUTT2: return &wm_bret_butt2_anim;
        case WM_BRET_ANIM_BUTT4: return &wm_bret_butt4_anim;
        case WM_BRET_ANIM_KNEE2: return &wm_bret_knee2_anim;
        case WM_BRET_ANIM_KNEE4: return &wm_bret_knee4_anim;
        case WM_BRET_ANIM_UPPERCUT4: return &wm_bret_uppercut4_anim;
        case WM_BRET_ANIM_STOMP2: return &wm_bret_stomp2_anim;
        case WM_BRET_ANIM_STOMP4: return &wm_bret_stomp4_anim;
        case WM_BRET_ANIM_GROUND_PUNCH2: return &wm_bret_ground_punch2_anim;
        case WM_BRET_ANIM_GROUND_PUNCH4: return &wm_bret_ground_punch4_anim;
        case WM_BRET_ANIM_PUSH4: return &wm_bret_push4_anim;
        case WM_BRET_ANIM_JUMP_KICK4: return &wm_bret_jump_kick4_anim;
        case WM_BRET_ANIM_KNEE_FALL4: return &wm_bret_knee_fall4_anim;
        case WM_BRET_ANIM_KICK_TB: return &wm_bret_kick_tb_anim;
        case WM_BRET_ANIM_HEAD_HELD_STAND3: return &wm_bret_head_held_stand3_anim;
        case WM_BRET_ANIM_KNEE_TO_HEAD4: return &wm_bret_knee_to_head4_anim;
        case WM_BRET_ANIM_FAKE_HOLD3: return &wm_bret_fake_hold3_anim;
        case WM_BRET_ANIM_KNEES_TO_HEAD: return &wm_bret_knees_to_head_anim;
        case WM_BRET_ANIM_PIN2: return &wm_bret_pin2_anim;
        case WM_BRET_ANIM_PIN4: return &wm_bret_pin4_anim;
        case WM_BRET_ANIM_BUTTS2: return &wm_bret_butts2_anim;
        case WM_BRET_ANIM_BUTTS4: return &wm_bret_butts4_anim;
        case WM_BRET_ANIM_FLYING_KICK: return &wm_bret_flying_kick_anim;
        case WM_BRET_ANIM_TBUKL_LEAP: return &wm_bret_tbukl_leap_anim;
        case WM_BRET_ANIM_RUNNING_GROUND_PUNCH:
            return &wm_bret_running_ground_punch_anim;
        case WM_BRET_ANIM_COMBO_PUNCH: return &wm_bret_combo_punch_anim;
        case WM_BRET_ANIM_COMBO_KICK: return &wm_bret_combo_kick_anim;
        case WM_BRET_ANIM_FALL_BACK: return &wm_bret_fall_back_anim;
        case WM_BRET_ANIM_FACEDOWN_GETUP: return &wm_bret_facedown_getup_anim;
        case WM_BRET_ANIM_FACEUP_GETUP: return &wm_bret_faceup_getup_anim;
        case WM_BRET_ANIM_FACEUP_GETUP2_4: return &wm_bret_faceup_getup2_4_anim;
        case WM_BRET_ANIM_HITONGROUND_FACEDOWN:
            return &wm_bret_hitonground_facedown_anim;
        /* WRESTLE2.ASM:3443 start_run_anim has no WL frames of its own: it
           is a state-setup routine that ends by selecting the wrestler's
           own run animation out of #run_anims[WRESTLERNUM]. For Bret that
           is hrt_run_anim, already extracted, so the id resolves straight
           to it and wm_arcade_start_run does the state half. */
        case WM_BRET_ANIM_START_RUN: return &wm_bret_run_anim;
        default: return NULL;
    }
}

/*
 * hrt_4_block_anim's own ANI_WAITRELEASE,PLAYER_BLOCK_BIT sits between its
 * second and third frames (HRTSEQ4.ASM:104):
 *
 *     ANI_SETPLYRMODE,MODE_BLOCK
 *     WL 3,H4BK3A+FR1                       <- frame 0
 *   #4block:
 *     WL 3,H4BK3A+FR2                       <- frame 1
 *     ANI_SETMODE,...|MODE_FRICTION
 *     ANI_WAITRELEASE,PLAYER_BLOCK_BIT      <- parks here
 *     ANI_SETMODE,MODE_NOAUTOFLIP
 *     ANI_SETFACING
 *     WL 3,H4BK3A+FR1                       <- frame 2
 *     ANI_SETPLYRMODE,MODE_NORMAL
 *     ANI_END
 *
 * So the animation holds on frame 1 for exactly as long as the block button
 * is held, then plays its last frame and hands the wrestler back to
 * MODE_NORMAL. That hold is what actually keeps a blocking wrestler in
 * WM_PMODE_BLOCK, which is in turn what makes wm_arcade_try_attack_hit's own
 * hit_blocker check and mode_block's 160-tick push counter reachable at all.
 */
#define WM_BRET_BLOCK_WAITRELEASE_FRAME ((size_t)1)

static bool block_holding_waitrelease(const wm_bret_backend_actor *bva,
                                      const wm_arcade_actor_t *actor) {
    if (!bva || !actor) return false;
    if (bva->visual.sequence != &wm_bret_block4_anim) return false;
    if (bva->visual.ended) return false;
    if (bva->visual.frame_index != WM_BRET_BLOCK_WAITRELEASE_FRAME) return false;
    return (actor->but_val_cur & WM_BTN_BLOCK) != 0;
}

/* Returns true iff this call actually (re)started the sequence -- callers
   use that to gate one-shot "instant command processed when a new
   animation starts" side effects (MODE_UNINT, ANI_SETFACING below) so they
   fire once per real selection, not every tick a caller happens to pass
   the same id/sequence again. */
static bool start_if_new(wm_visual_state *state, const wm_visual_sequence *seq) {
    if (!state || !seq) return false;
    if (state->sequence != seq || state->ended) {
        wm_visual_start(state, seq);
        return true;
    }
    return false;
}

/*
 * Real ANIM.ASM ATTACK_ON(_Z) args for the attack ids wm_bret_anim_sequence
 * maps: each attack's ANI_ATTACK_ON(_Z) command falls right before the WL
 * frame line at active_frame_index (0-based) in that id's
 * wm_visual_sequence. The first six were hand-traced against HRTSEQ2.ASM
 * (see wm_bret_backend_tick's comment); everything after them was traced
 * with tools/wlattack.py, which reproduces all six of those by hand-traced
 * index and operand exactly.
 *
 * One row is one ANI_ATTACK_ON(_Z)/ANI_ATTACK_OFF pulse, NOT one animation:
 * an animation with several real pulses gets several rows, matched on
 * (id, frame). hrt_2/4_stomp_anim fire two apiece and
 * hrt_2/4_ground_punch_anim three, each a genuinely separate attack box in
 * the source, so collapsing them to one window per id would silently drop
 * real hits.
 */
typedef struct {
    wm_arcade_bret_anim_id_t id;
    size_t active_frame_index;
    bool use_z;
    wm_arcade_attack_on_args_t args;
    wm_arcade_attack_on_z_args_t z_args;
} wm_bret_attack_window_t;

static const wm_bret_attack_window_t attack_windows[] = {
    /* HRTSEQ2.ASM:204 ANI_ATTACK_ON_Z, AMODE_PUNCH,30,91,-45,50,15,45 */
    { WM_BRET_ANIM_PUNCH2, 5, true, {0,0,0,0,0},
      { WM_AMODE_PUNCH, 30, 91, -45, 50, 15, 45 } },
    /* HRTSEQ2.ASM:322 ANI_ATTACK_ON_Z, AMODE_PUNCH,30,91,0,50,15,45 */
    { WM_BRET_ANIM_PUNCH4, 5, true, {0,0,0,0,0},
      { WM_AMODE_PUNCH, 30, 91, 0, 50, 15, 45 } },
    /* HRTSEQ2.ASM:247 hrt_4_super_punch_anim,
       ANI_ATTACK_ON,AMODE_UPRCUT,-6,40,64,90 -- #scrt_cut's supercut. */
    { WM_BRET_ANIM_SUPER_PUNCH4, 5, false,
      { WM_AMODE_UPRCUT, -6, 40, 64, 90 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM:618/677 hrt_2/4_super_punch2_anim,
       ANI_ATTACK_ON,AMODE_URN,19,75,35,24 -- the ordinary super punch.
       Same box numbers as the headbutt, a different AMODE, and a
       different frame index from the supercut above. */
    { WM_BRET_ANIM_SUPER_PUNCH2_2, 3, false,
      { WM_AMODE_URN, 19, 75, 35, 24 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_SUPER_PUNCH2_4, 3, false,
      { WM_AMODE_URN, 19, 75, 35, 24 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM:1074 ANI_ATTACK_ON,AMODE_KICK,23,73,50,17 */
    { WM_BRET_ANIM_KICK2, 5, false,
      { WM_AMODE_KICK, 23, 73, 50, 17 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM:1240 ANI_ATTACK_ON,AMODE_KICK,23,73,50,17 */
    { WM_BRET_ANIM_KICK4, 5, false,
      { WM_AMODE_KICK, 23, 73, 50, 17 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM:1357 ANI_ATTACK_ON,AMODE_SUPER_KICK,5,54,70,34 -- shared
       body (HRTSEQ2.ASM:1334-1335 SUBR alias), so SUPER_KICK4 gets the
       identical hand-traced window, not a separate guess. */
    { WM_BRET_ANIM_SUPER_KICK2, 4, false,
      { WM_AMODE_SUPER_KICK, 5, 54, 70, 34 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_SUPER_KICK4, 4, false,
      { WM_AMODE_SUPER_KICK, 5, 54, 70, 34 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM hrt_2/4_butt_anim: ANI_ATTACK_ON,AMODE_HDBUTT,19,75,35,24
       -- the close-range headbutt do_punch selects (identical window in
       both facing banks). */
    { WM_BRET_ANIM_BUTT2, 5, false,
      { WM_AMODE_HDBUTT, 19, 75, 35, 24 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_BUTT4, 5, false,
      { WM_AMODE_HDBUTT, 19, 75, 35, 24 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM hrt_2/4_knee_anim: ANI_ATTACK_ON,AMODE_KNEE,11,44,51,49
       -- the close-range knee do_kick selects. */
    { WM_BRET_ANIM_KNEE2, 4, false,
      { WM_AMODE_KNEE, 11, 44, 51, 49 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_KNEE4, 4, false,
      { WM_AMODE_KNEE, 11, 44, 51, 49 }, {0,0,0,0,0,0,0} },
    /* HRTSEQ2.ASM hrt_4_uppercut_anim: ANI_ATTACK_ON,AMODE_UPRCUT,-6,22,64,100.
       Distinct from SUPER_PUNCH4's own -6,40,64,90 window. */
    { WM_BRET_ANIM_UPPERCUT4, 5, false,
      { WM_AMODE_UPRCUT, -6, 22, 64, 100 }, {0,0,0,0,0,0,0} },
    /* hrt_2_stomp_anim: two real pulses, the first an AMODE_HITCHECK probe
       and the second the AMODE_STOMP2 hit itself, with an ANI_ATTACK_OFF
       and an ANI_STARTATTACK,AT_STOMP,5 between them. Both are
       ANI_ATTACK_ON_Z, so both carry real z offset/depth. */
    { WM_BRET_ANIM_STOMP2, 4, true, {0,0,0,0,0},
      { WM_AMODE_HITCHECK, 7, -10, -40, 28, 31, 50 } },
    { WM_BRET_ANIM_STOMP2, 7, true, {0,0,0,0,0},
      { WM_AMODE_STOMP2, 7, -10, -40, 28, 31, 50 } },
    /* hrt_4_stomp_anim: same two-pulse shape, its own real 4-facing box. */
    { WM_BRET_ANIM_STOMP4, 4, true, {0,0,0,0,0},
      { WM_AMODE_HITCHECK, 7, -12, -10, 29, 35, 50 } },
    { WM_BRET_ANIM_STOMP4, 7, true, {0,0,0,0,0},
      { WM_AMODE_STOMP2, 7, -12, -10, 29, 35, 50 } },
    /* hrt_2_ground_punch_anim: three real pulses -- an AMODE_HITCHECK probe
       and then two separate AMODE_LBOWDROP2 elbow drops (the second after
       its own ANI_STARTATTACK,AT_LBDROP,14). The first row's x offset is
       written `5-10` in the source, an assembler expression, i.e. -5, and
       is deliberately not "simplified" to the 5 the other two use. */
    { WM_BRET_ANIM_GROUND_PUNCH2, 2, true, {0,0,0,0,0},
      { WM_AMODE_HITCHECK, 5 - 10, -8, -40, 32, 32, 50 } },
    { WM_BRET_ANIM_GROUND_PUNCH2, 5, true, {0,0,0,0,0},
      { WM_AMODE_LBOWDROP2, 5, -8, -40, 32, 32, 50 } },
    { WM_BRET_ANIM_GROUND_PUNCH2, 8, true, {0,0,0,0,0},
      { WM_AMODE_LBOWDROP2, 5, -8, -40, 32, 32, 50 } },
    /* hrt_4_ground_punch_anim: same three-pulse shape, its own real box,
       and here the probe's x offset really is a plain 5. */
    { WM_BRET_ANIM_GROUND_PUNCH4, 2, true, {0,0,0,0,0},
      { WM_AMODE_HITCHECK, 5, -6, -10, 36, 30, 50 } },
    { WM_BRET_ANIM_GROUND_PUNCH4, 5, true, {0,0,0,0,0},
      { WM_AMODE_LBOWDROP2, 5, -6, -10, 36, 30, 50 } },
    { WM_BRET_ANIM_GROUND_PUNCH4, 8, true, {0,0,0,0,0},
      { WM_AMODE_LBOWDROP2, 5, -6, -10, 36, 30, 50 } },
    /* hrt_4_push_anim: ANI_ATTACK_ON,AMODE_PUSH,11,83,70,20 -- the shove
       mode_block uses to break a blocking opponent off, and the same one
       its own down-input branch selects. */
    { WM_BRET_ANIM_PUSH4, 3, false,
      { WM_AMODE_PUSH, 11, 83, 70, 20 }, {0,0,0,0,0,0,0} },
    /* hrt_4_jump_kick_anim: ANI_ATTACK_ON,AMODE_FLYKICK,15,69,64,38 --
       wm_arcade_bret_fire_secret's own jump-kick target, which until now
       had no frame data and so only got the one-tick MODE_UNINT stopgap. */
    { WM_BRET_ANIM_JUMP_KICK4, 4, false,
      { WM_AMODE_FLYKICK, 15, 69, 64, 38 }, {0,0,0,0,0,0,0} },
    /* hrt_4_knee_fall_anim: ANI_ATTACK_ON,AMODE_BIGKNEE,11,44,51,49 --
       same box as the ordinary knee, a different AMODE. */
    { WM_BRET_ANIM_KNEE_FALL4, 2, false,
      { WM_AMODE_BIGKNEE, 11, 44, 51, 49 }, {0,0,0,0,0,0,0} },
    /* hrt_kick_TB_anim: ANI_ATTACK_ON,AMODE_SPINKICK,5,54,70,34, and the
       one wired attack whose ANI_ATTACK_OFF is two frames later rather
       than one (frames 2 and 3 are both inside the window). */
    { WM_BRET_ANIM_KICK_TB, 2, false,
      { WM_AMODE_SPINKICK, 5, 54, 70, 34 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_KICK_TB, 3, false,
      { WM_AMODE_SPINKICK, 5, 54, 70, 34 }, {0,0,0,0,0,0,0} },
    /* hrt_4_knee_to_head_anim: ANI_ATTACK_ON,AMODE_KNEE,11,44,51,49 at
       frame 2 of the chained 8-frame stream (1 frame before chaining, and
       the window sat past its end). */
    { WM_BRET_ANIM_KNEE_TO_HEAD4, 2, false,
      { WM_AMODE_KNEE, 11, 44, 51, 49 }, {0,0,0,0,0,0,0} },
    /* hrt_knees_to_head_anim: two real pulses with different boxes. The
       first sits INSIDE the routine's ANI_SET_RPTCOUNT,3 span (frames
       1..5), so the visual runtime's loop genuinely re-fires it once per
       pass, exactly as the source's own ANI_ATTACK_ON does inside its
       loop body; the second is after the loop. */
    { WM_BRET_ANIM_KNEES_TO_HEAD, 3, false,
      { WM_AMODE_HEADKNEES, 4, 34, 70, 54 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_KNEES_TO_HEAD, 8, false,
      { WM_AMODE_HEADKNEES, 4, 54, 70, 34 }, {0,0,0,0,0,0,0} },
    /* hrt_2/4_butts_anim: AMODE_HDBUTT_STAY, and the window sits inside
       the routine's ANI_SET_RPTCOUNT,3 span, so three headbutts land per
       playthrough from this one row. */
    { WM_BRET_ANIM_BUTTS2, 3, false,
      { WM_AMODE_HDBUTT_STAY, 19, 75, 35, 24 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_BUTTS4, 3, false,
      { WM_AMODE_HDBUTT_STAY, 19, 75, 35, 24 }, {0,0,0,0,0,0,0} },
    /* hrt_flying_kick_anim -- a different box from JUMP_KICK4's own
       AMODE_FLYKICK,15,69,64,38. */
    { WM_BRET_ANIM_FLYING_KICK, 4, false,
      { WM_AMODE_FLYKICK, -3, 26, 61, 21 }, {0,0,0,0,0,0,0} },
    /* hrt_tbukl_leap_anim writes TWO ANI_ATTACK_ON_Z back to back here,
       its own comment saying "attack box dimensions depends on opp mode":
       the second is reached by an ANI_IFOPPMODE,MODE_ONGROUND branch this
       port does not evaluate. The fall-through default (y offset -1+5) is
       wired; the grounded-opponent variant (-1+15) deliberately is not,
       rather than guessed at. */
    { WM_BRET_ANIM_TBUKL_LEAP, 4, true, {0,0,0,0,0},
      { WM_AMODE_BSTOMP, 0, -1 + 5, -10, 36, 52, 70 } },
    { WM_BRET_ANIM_RUNNING_GROUND_PUNCH, 5, false,
      { WM_AMODE_BUTTSTOMP, -50, -6, 36, 23 }, {0,0,0,0,0,0,0} },
    /* hrt_combo_punch_anim / hrt_combo_kick_anim: three real pulses each,
       one per combo hit. */
    { WM_BRET_ANIM_COMBO_PUNCH, 3, true, {0,0,0,0,0},
      { WM_AMODE_PUNCH, 30, 51, 0, 80, 45, 45 } },
    { WM_BRET_ANIM_COMBO_PUNCH, 12, true, {0,0,0,0,0},
      { WM_AMODE_PUNCH, 30, 51, 0, 80, 45, 45 } },
    { WM_BRET_ANIM_COMBO_PUNCH, 21, true, {0,0,0,0,0},
      { WM_AMODE_PUNCH, 30, 51, 0, 80, 45, 45 } },
    { WM_BRET_ANIM_COMBO_KICK, 3, false,
      { WM_AMODE_KICK, 23, 53, 50, 27 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_COMBO_KICK, 12, false,
      { WM_AMODE_KICK, 23, 53, 50, 27 }, {0,0,0,0,0,0,0} },
    { WM_BRET_ANIM_COMBO_KICK, 21, false,
      { WM_AMODE_KICK, 23, 53, 50, 27 }, {0,0,0,0,0,0,0} },
};
#define WM_BRET_ATTACK_WINDOW_COUNT \
    (sizeof(attack_windows) / sizeof(attack_windows[0]))

/*
 * ANIM.ASM's ANI_CHANGEANIM (:1301) where it genuinely terminates a
 * routine: _ani_changeanim overwrites OANIPC AND OANIBASE with the target
 * and never returns, so the animation does not end -- it becomes another
 * one. The source writes exactly that shape with the `.word ANI_END` after
 * it commented out.
 *
 * Only unconditional transitions are here. tools/wlanim.py reports a
 * next_label only when nothing after the ANI_CHANGEANIM ends the routine
 * any other way; where a real ANI_END follows (hrt_2_butts_anim's
 * button-mash #ex path, hrt_facedown_getup_anim's free-toss branch) the
 * transition is one exit among several and is deliberately NOT taken here,
 * because taking it would need the condition this port does not evaluate.
 */
typedef struct {
    wm_arcade_bret_anim_id_t from;
    wm_arcade_bret_anim_id_t to;
} wm_bret_anim_transition_t;

static const wm_bret_anim_transition_t anim_transitions[] = {
    { WM_BRET_ANIM_FALL_BACK,            WM_BRET_ANIM_FACEUP_GETUP },
    { WM_BRET_ANIM_FLYING_KICK,          WM_BRET_ANIM_FACEDOWN_GETUP },
    { WM_BRET_ANIM_TBUKL_LEAP,           WM_BRET_ANIM_HITONGROUND_FACEDOWN },
    { WM_BRET_ANIM_RUNNING_GROUND_PUNCH, WM_BRET_ANIM_FACEUP_GETUP2_4 },
    /* hrt_hitonground_facedown_anim is itself a transition target that
       transitions again, so the chain really is leap -> hit the ground ->
       get up. */
    { WM_BRET_ANIM_HITONGROUND_FACEDOWN, WM_BRET_ANIM_FACEUP_GETUP },
};
#define WM_BRET_ANIM_TRANSITION_COUNT \
    (sizeof(anim_transitions) / sizeof(anim_transitions[0]))

static bool find_anim_transition(wm_arcade_bret_anim_id_t from,
                                 wm_arcade_bret_anim_id_t *to) {
    size_t i;
    for (i = 0; i < WM_BRET_ANIM_TRANSITION_COUNT; ++i)
        if (anim_transitions[i].from == from) {
            *to = anim_transitions[i].to;
            return true;
        }
    return false;
}

/*
 * ANIM.ASM's ANI_SETPLYRMODE, when it falls partway through an animation
 * rather than in its header. Keyed on (id, frame) exactly like the attack
 * windows, and applied by wm_bret_backend_tick when the animation reaches
 * that frame. A row at frame 0 is applied by wm_bret_backend_change_anim
 * instead, since the header commands are instant on selection.
 */
typedef struct {
    wm_arcade_bret_anim_id_t id;
    size_t frame_index;
    uint16_t player_mode;
} wm_bret_plyrmode_change_t;

static const wm_bret_plyrmode_change_t plyrmode_changes[] = {
    /* hrt_kick_TB_anim: ANI_SETPLYRMODE,MODE_INAIR2 then MODE_NORMAL. */
    { WM_BRET_ANIM_KICK_TB, 0, WM_PMODE_INAIR2 },
    { WM_BRET_ANIM_KICK_TB, 4, WM_PMODE_NORMAL },
    /* hrt_4_push_anim, hrt_3_head_held_stand_anim and hrt_3_fake_hold_anim
       all lead with ANI_SETPLYRMODE,MODE_NORMAL before their own frame 0 --
       for the head-held stand that is the whole point of the animation,
       since it is what actually releases mode_headhold. */
    { WM_BRET_ANIM_PUSH4, 0, WM_PMODE_NORMAL },
    { WM_BRET_ANIM_HEAD_HELD_STAND3, 0, WM_PMODE_NORMAL },
    { WM_BRET_ANIM_FAKE_HOLD3, 0, WM_PMODE_NORMAL },
    /* hrt_2/4_stomp_anim's own header. */
    { WM_BRET_ANIM_STOMP2, 0, WM_PMODE_NORMAL },
    { WM_BRET_ANIM_STOMP4, 0, WM_PMODE_NORMAL },
    /* hrt_flying_kick_anim: NORMAL on entry, ONGROUND once he lands. */
    { WM_BRET_ANIM_FLYING_KICK, 0, WM_PMODE_NORMAL },
    { WM_BRET_ANIM_FLYING_KICK, 7, WM_PMODE_ONGROUND },
    /* hrt_tbukl_leap_anim ends up on the ground too. */
    { WM_BRET_ANIM_TBUKL_LEAP, 6, WM_PMODE_ONGROUND },
    /* hrt_running_ground_punch_anim: NORMAL, then airborne for the drop. */
    { WM_BRET_ANIM_RUNNING_GROUND_PUNCH, 0, WM_PMODE_NORMAL },
    { WM_BRET_ANIM_RUNNING_GROUND_PUNCH, 2, WM_PMODE_INAIR },
    { WM_BRET_ANIM_RUNNING_GROUND_PUNCH, 7, WM_PMODE_ONGROUND },
    /* hrt_fall_back_anim's own landing. */
    { WM_BRET_ANIM_FALL_BACK, 11, WM_PMODE_ONGROUND },
    /* The getups' own headers put him back on his feet. */
    { WM_BRET_ANIM_FACEDOWN_GETUP, 0, WM_PMODE_NORMAL },
    { WM_BRET_ANIM_FACEUP_GETUP, 0, WM_PMODE_NORMAL },
    { WM_BRET_ANIM_FACEUP_GETUP2_4, 0, WM_PMODE_NORMAL },
};
#define WM_BRET_PLYRMODE_CHANGE_COUNT \
    (sizeof(plyrmode_changes) / sizeof(plyrmode_changes[0]))

static const wm_bret_plyrmode_change_t *find_plyrmode_change(
        wm_arcade_bret_anim_id_t id, size_t frame_index) {
    size_t i;
    for (i = 0; i < WM_BRET_PLYRMODE_CHANGE_COUNT; ++i)
        if (plyrmode_changes[i].id == id &&
            plyrmode_changes[i].frame_index == frame_index)
            return &plyrmode_changes[i];
    return NULL;
}

/*
 * Extra ANI_SETMODE bits a wired animation's own HRTSEQ header carries on
 * top of MODE_UNINT|MODE_NOAUTOFLIP, read with tools/wlattack.py rather
 * than assumed. Most attacks carry only those two; these carry more, and
 * every bit here is genuinely consumed somewhere in the port
 * (WM_MODE_OVERLAP by wm_arcade_combat.c's wrestler-overlap test,
 * WM_MODE_NOCONFINE by wm_arcade_confine_wrestler's own early-out,
 * WM_MODE_NOGRAVITY and WM_MODE_NOCOLLIS by the react/collision paths).
 */
static uint16_t anim_header_mode_bits(wm_arcade_bret_anim_id_t id) {
    switch (id) {
        case WM_BRET_ANIM_STOMP2:
        case WM_BRET_ANIM_STOMP4:
        case WM_BRET_ANIM_GROUND_PUNCH2:
        case WM_BRET_ANIM_GROUND_PUNCH4:
        case WM_BRET_ANIM_FAKE_HOLD3:
        case WM_BRET_ANIM_PIN2:
        case WM_BRET_ANIM_PIN4:
        case WM_BRET_ANIM_RUNNING_GROUND_PUNCH:
            return (uint16_t)WM_MODE_OVERLAP;
        /* hrt_facedown_getup_anim / hrt_faceup_getup_anim /
           hrt_4_faceup_getup2_anim: ...|MODE_NOCOLLIS */
        case WM_BRET_ANIM_FACEDOWN_GETUP:
        case WM_BRET_ANIM_FACEUP_GETUP:
        case WM_BRET_ANIM_FACEUP_GETUP2_4:
            return (uint16_t)WM_MODE_NOCOLLIS;
        /* hrt_fall_back_anim: ...|MODE_OVERLAP|MODE_NOCOLLIS */
        case WM_BRET_ANIM_FALL_BACK:
            return (uint16_t)(WM_MODE_OVERLAP | WM_MODE_NOCOLLIS);
        /* hrt_tbukl_leap_anim:
           ...|MODE_OVERLAP|MODE_NOCONFINE|MODE_NOGRAVITY */
        case WM_BRET_ANIM_TBUKL_LEAP:
            return (uint16_t)(WM_MODE_OVERLAP | WM_MODE_NOCONFINE |
                              WM_MODE_NOGRAVITY);
        default:
            return 0;
    }
}

/* Any pulse at all for this id -- i.e. "this animation is one of the wired
   attacks", which is what the MODE_UNINT header treatment keys off. */
static bool anim_has_attack_window(wm_arcade_bret_anim_id_t id) {
    size_t i;
    for (i = 0; i < WM_BRET_ATTACK_WINDOW_COUNT; ++i)
        if (attack_windows[i].id == id) return true;
    return false;
}

/* The one pulse (if any) whose ANI_ATTACK_ON falls at this exact frame. */
static const wm_bret_attack_window_t *find_attack_window_at(wm_arcade_bret_anim_id_t id,
                                                            size_t frame_index) {
    size_t i;
    for (i = 0; i < WM_BRET_ATTACK_WINDOW_COUNT; ++i)
        if (attack_windows[i].id == id &&
            attack_windows[i].active_frame_index == frame_index)
            return &attack_windows[i];
    return NULL;
}

/* Wired animations whose own HRTSEQ header leads with
   ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP. That is every animation with an
   attack window, plus the ones that carry the header without ever
   attacking -- hrt_3_head_held_stand_anim is the first of those. (Block is
   deliberately not here: hrt_4_block_anim's own header does more, and has
   its own block below.) */
static bool anim_header_sets_uninit(wm_arcade_bret_anim_id_t id) {
    if (anim_has_attack_window(id)) return true;
    return id == WM_BRET_ANIM_HEAD_HELD_STAND3 ||
           id == WM_BRET_ANIM_FAKE_HOLD3 ||
           id == WM_BRET_ANIM_PIN2 || id == WM_BRET_ANIM_PIN4 ||
           id == WM_BRET_ANIM_FALL_BACK ||
           id == WM_BRET_ANIM_FACEDOWN_GETUP ||
           id == WM_BRET_ANIM_FACEUP_GETUP ||
           id == WM_BRET_ANIM_FACEUP_GETUP2_4 ||
           id == WM_BRET_ANIM_HITONGROUND_FACEDOWN ||
           id == WM_BRET_ANIM_START_RUN;
}

/*
 * HRTSEQ2.ASM: 5 of the 6 wired attacks lead with a real, instant
 * ANI_SETFACING (right after ANI_ZEROVELS, before any WL frame -- same
 * "processed synchronously when the animation starts" reasoning as
 * MODE_UNINT above): hrt_2_punch_anim:187, hrt_4_punch_anim:307,
 * hrt_4_super_punch_anim, hrt_2_kick_anim:1059, and
 * hrt_2_super_kick_anim/hrt_4_super_kick_anim:1342 (after ANI_STARTATTACK
 * there, but still before any WL frame -- same "instant, on start"
 * timing; SUPER_KICK2 and SUPER_KICK4 share this one body). hrt_4_kick_anim
 * is the one real exception: no ANI_SETFACING at all, verified by reading
 * its header directly, not assumed from the other 5.
 *
 * hrt_4_jump_kick_anim (HRTSEQ2.ASM:1265), hrt_hiptoss_anim (HRTSEQ3.ASM:
 * 445) and hrt_2/4_grabfling_anim (HRTSEQ2.ASM:2421/2433) -- three more of
 * wm_arcade_bret_fire_secret's own real secret-move targets -- carry the
 * identical instant ANI_SETFACING too, read directly from their own
 * headers the same way. */
static bool attack_sets_facing_on_start(wm_arcade_bret_anim_id_t id) {
    switch (id) {
        case WM_BRET_ANIM_PUNCH2:
        case WM_BRET_ANIM_PUNCH4:
        /* hrt_4_super_punch_anim and both hrt_2/4_super_punch2_anim carry
           the same instant ANI_SETFACING in their own headers. */
        case WM_BRET_ANIM_SUPER_PUNCH4:
        case WM_BRET_ANIM_SUPER_PUNCH2_2:
        case WM_BRET_ANIM_SUPER_PUNCH2_4:
        case WM_BRET_ANIM_KICK2:
        case WM_BRET_ANIM_SUPER_KICK2:
        case WM_BRET_ANIM_SUPER_KICK4:
        case WM_BRET_ANIM_JUMP_KICK4:
        case WM_BRET_ANIM_HIPTOSS:
        case WM_BRET_ANIM_GRABFLING_FACE24:
        case WM_BRET_ANIM_BUTT2:
        case WM_BRET_ANIM_BUTT4:
        case WM_BRET_ANIM_KNEE2:
        case WM_BRET_ANIM_KNEE4:
        case WM_BRET_ANIM_UPPERCUT4:
        /* push4, jump_kick4 and kick_TB carry the same instant
           ANI_SETFACING in their own headers; knee_fall4 and
           head_held_stand3 genuinely do not. */
        case WM_BRET_ANIM_PUSH4:
        case WM_BRET_ANIM_KICK_TB:
        case WM_BRET_ANIM_BUTTS2:
        case WM_BRET_ANIM_BUTTS4:
        case WM_BRET_ANIM_FLYING_KICK:
        case WM_BRET_ANIM_TBUKL_LEAP:
        case WM_BRET_ANIM_COMBO_KICK:
            return true;
        default:
            return false;
    }
}

/*
 * wm_arcade_bret_fire_secret's remaining secret-move targets that have no
 * real extracted wm_visual_sequence data yet (see wm_bret_anim_sequence),
 * but whose own real HRTSEQ header does lead with `ANI_SETMODE,MODE_UNINT
 * |MODE_NOAUTOFLIP` exactly like the attacks find_attack_window covers --
 * read directly from each header, not assumed: hrt_4_jump_kick_anim
 * (HRTSEQ2.ASM:1265), hrt_running_ddt_anim (HRTSEQ3.ASM:1614),
 * hrt_hh_2_ddt_anim (HRTSEQ3.ASM:1288), hrt_hiptoss_anim (HRTSEQ3.ASM:445),
 * hrt_2/4_grabfling_anim (HRTSEQ2.ASM:2421/2433), hrt_rake_face_anim
 * (HRTSEQ3.ASM:2465), hrt_3_head_hold_anim/hrt_3_head_hold2_anim/
 * hrt_3_fake_hold_anim (HRTSEQ3.ASM:1126/1105/1089).
 *
 * Without frame data there is no wm_visual_state to time a real end from
 * (the mechanism find_attack_window's own ids use, see
 * wm_bret_backend_tick), so wm_bret_backend_change_anim clears this back
 * off itself, same tick, via bva->pending_uninit_clear rather than leaving
 * it set indefinitely -- a deliberate, documented simplification (real
 * per-frame duration data would be needed to protect these for their own
 * actual length, matching the source, rather than just this one tick). */
static bool secret_move_sets_mode_uninit(wm_arcade_bret_anim_id_t id) {
    switch (id) {
        /* WM_BRET_ANIM_JUMP_KICK4 used to be on this list. It now has real
           extracted frame data and a real attack window, so it gets the
           properly-timed MODE_UNINT the wired attacks get and no longer
           needs (or wants) the one-tick stopgap below. */
        case WM_BRET_ANIM_RUNNING_DDT:
        case WM_BRET_ANIM_HH_DDT2:
        case WM_BRET_ANIM_HIPTOSS:
        case WM_BRET_ANIM_GRABFLING_FACE24:
        case WM_BRET_ANIM_RAKE_FACE:
        case WM_BRET_ANIM_HEAD_HOLD2_3:
        case WM_BRET_ANIM_HEAD_HOLD3:
            return true;
        default:
            return false;
    }
}

wm_arcade_frame_box_t wm_bret_hurt_box_for_frame(const char *source_frame) {
    wm_arcade_frame_box_t box;
    const wm_bret_frame_geometry_t *geo;

    box.iani3x = 0;
    box.iani3y = 0;
    box.iani3z = 0;
    box.iani3id = 0;

    geo = source_frame ? wm_bret_frame_geometry_find(source_frame) : NULL;
    if (!geo) return box;

    box.iani3x = -(int32_t)geo->xani;
    box.iani3y = -(int32_t)geo->yani;
    box.iani3z = (int32_t)geo->width;
    box.iani3id = (int32_t)geo->height;
    return box;
}

void wm_bret_backend_change_anim(wm_arcade_actor_t *actor,
                                 wm_arcade_bret_anim_id_t id, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    const wm_visual_sequence *seq;
    bool is_new_selection;
    if (!bva) return;

    /* start_if_new's own "restarted" answer only exists when this id has
       real extracted frame data to time an end from. Ids without any
       (secret_move_sets_mode_uninit's own list) still need to tell a
       genuinely new selection from a repeated call with the same id
       already in progress -- current_id changing is the only signal
       available without real timing data. */
    seq = wm_bret_anim_sequence(id);
    is_new_selection = seq ? start_if_new(&bva->visual, seq)
                           : (bva->current_id != id);
    bva->current_id = id;
    if (!actor || !is_new_selection) return;

    /* HRTSEQ2.ASM's own attack headers (hrt_2_punch_anim etc., HRTSEQ2.ASM:
       184/303/...) all lead with `ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP`.
       ANI_SETMODE is a zero-tick "instant" command the interpreter
       processes synchronously the moment a new animation starts (same
       reasoning already established for the turn sequences' own
       MODE_INTURN), so BRET.ASM mode_normal's own `if (ANIMODE&MODE_UNINT)
       return` sees it the instant an attack is selected, before
       execute_walk would otherwise run this same tick and stomp the
       just-started attack with an idle-turn/walk-cycle reselection.
       Cleared back to MODE_NORMAL when the attack animation naturally
       ends (matching that same header's `ANI_SETMODE,MODE_NORMAL` right
       before its own ANI_END) -- see wm_bret_backend_tick. */
    /* start_run_anim's own ANI_CODE #setup_run (WRESTLE2.ASM:3452): pick a
       run direction, clear the getup/run timers, face that way and enter
       MODE RUNNING. Its header's ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP is
       covered by anim_header_sets_uninit below. */
    if (id == WM_BRET_ANIM_START_RUN)
        wm_arcade_start_run(actor);

    if (anim_header_sets_uninit(id)) {
        actor->anim_mode |= (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
        /* hrt_2/4_stomp_anim and hrt_2/4_ground_punch_anim are the only
           wired attacks whose own ANI_SETMODE adds MODE_OVERLAP -- real,
           and genuinely consumed here by wm_arcade_combat.c's
           wrestler-overlap test, which is why they get it and the others
           deliberately do not. */
        actor->anim_mode |= anim_header_mode_bits(id);
        /* ANIM.ASM's inline motion commands for this animation's own frame
           0 -- its header. These used to be approximated by zeroing x and z
           velocity for every attack; the real per-animation list is
           generated by tools/wlcommands.py and includes the y velocity,
           offsets and friction the approximation missed. */
        if (seq) {
            const char *become =
                wm_anim_apply_frame_commands(actor, seq->source_label, 0);
            if (become) bva->pending_become = become;
        }

        /* An ANI_SETPLYRMODE in this animation's own header (frame 0) is
           instant on selection, same as the mode bits above -- see
           plyrmode_changes. */
        {
            const wm_bret_plyrmode_change_t *pm = find_plyrmode_change(id, 0);
            if (pm && actor->player_mode != WM_PMODE_DEAD)
                actor->player_mode = pm->player_mode;
        }
    } else if (secret_move_sets_mode_uninit(id)) {
        /* secret_move_sets_mode_uninit's own comment: no frame data means
           no real way to time this back off, so it's cleared this same
           tick instead (wm_bret_backend_tick) rather than left set. */
        actor->anim_mode |= (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
        bva->pending_uninit_clear = true;
    }

    /* attack_sets_facing_on_start's own comment: 5 of the 6 wired attacks
       (plus 3 more secret-move targets) also carry a real, instant
       ANI_SETFACING right at their own start (before hrt_4_kick_anim's
       real exception). Gated on `is_new_selection`, not every call, since
       the source command fires exactly once, the instant the animation is
       selected -- not continuously for as long as it plays. */
    if (attack_sets_facing_on_start(id))
        actor->facing_dir = actor->new_facing_dir;

    /* hrt_4_block_anim's own instant header commands, all before its first
       WL frame: ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP, ANI_ZEROVELS,
       ANI_SETFACING, and -- the one that matters for gameplay --
       ANI_SETPLYRMODE,MODE_BLOCK. MODE_BLOCK is never set by BRET.ASM
       itself (his do_block only selects this animation); it is the
       animation that puts him in the mode, which is why he could never
       actually block until this was wired. */
    if (id == WM_BRET_ANIM_BLOCK4) {
        actor->anim_mode |= (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
        actor->x_vel = 0;
        actor->z_vel = 0;
        actor->facing_dir = actor->new_facing_dir;
        actor->player_mode = WM_PMODE_BLOCK;
    }
}

void wm_bret_backend_change_torso_anim(wm_arcade_actor_t *actor,
                                       wm_arcade_bret_anim_id_t id, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    (void)actor;
    if (!bva) return;
    start_if_new(&bva->torso_visual, wm_bret_anim_sequence(id));
}

/* wm_arcade_adjust_health's death_anim bridge: the only id it ever passes
   is WM_R1_ANIM_FALL_BACK (see wm/arcade/wm_arcade_lifebar.h), which is the
   real hrt_fall_back_anim -- already wired as WM_BRET_ANIM_FALL_BACK by
   Bret's own I_WILL_DIE self-death case (wm_arcade_bret.c). */
static void wm_bret_backend_death_change_anim(wm_arcade_actor_t *actor,
                                              wm_arcade_react1_anim_group_t anim,
                                              void *user) {
    if (anim != WM_R1_ANIM_FALL_BACK) return;
    wm_bret_backend_change_anim(actor, WM_BRET_ANIM_FALL_BACK, user);
}

/* wm_arcade_bret_callbacks_t.adjust_health body: mode_normal's own
   I_WILL_DIE self-death call (BRET.ASM:1341-1343, "movi -10,a0 ... calla
   adjust_health") reads WHOHITME itself for the shared routine's
   combo-revival check, so this needs no opponent parameter beyond what the
   actor already carries. */
static void wm_bret_backend_adjust_health(wm_arcade_actor_t *actor, int delta,
                                          void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    wm_arcade_death_anim_callback_t death_anim;
    if (!actor) return;
    death_anim.change_anim = wm_bret_backend_death_change_anim;
    death_anim.user = bva;
    /* No wm_arcade_combat_runtime_t reachable from this self-death path
       (see wm_arcade_adjust_health's own comment): DAM_MULT tracking is
       skipped here, not guessed at. */
    wm_arcade_adjust_health(actor, (int16_t)delta, actor->who_hit_me,
                            bva ? bva->attract_mode : false,
                            bva ? bva->pcnt : 0, NULL, &death_anim);
}

void wm_bret_backend_execute_walk(wm_arcade_actor_t *actor, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    int32_t old_facing_dir;
    bool leg_inturn, torso_inturn;

    if (!actor || !bva) return;

    /* WRESTLE.ASM::execute_walk's own top-of-function gate (WRESTLE.ASM:
       5222-5252): "if our INTURN bit is set, we're doing a turn and we
       shouldn't do anything here -- treat it like UNINT." The source checks
       ANIMODE (leg) and ANIMODE2 (torso) independently; this port has no
       actor-level ANIMODE2 field (only Bret's torso track exists at all),
       so it's derived from bva's own visual state instead: a turn/turn2
       sequence is exactly the set that sets MODE_INTURN when played
       (rotate_table/torso_table's off-diagonal cells, never their
       diagonal/stand entries). This freeze isn't cosmetic: without it,
       set_rotate_anim's instant FACING_DIR=NEW_FACING_DIR copy (below)
       would immediately re-equalize old/new FACING_DIR on the very next
       idle tick and truncate every turn anim to a single tick; for the
       two-ANI_SETFACING torso turns it would cut the sequence off before
       its second FACING_DIR promotion ever fires. */
    leg_inturn = is_leg_turn_sequence(bva->visual.sequence) && !bva->visual.ended;
    torso_inturn = find_torso_turn_setfacing(bva->torso_visual.sequence) && !bva->torso_visual.ended;
    if (leg_inturn || torso_inturn) {
        if (actor->move_dir == 0) {
            actor->x_vel = 0;
            actor->z_vel = 0;
        }
        return;
    }

    old_facing_dir = actor->facing_dir;
    wm_execute_walk(actor, bva->opponent, wm_bret_velocity_table);

    if (actor->move_dir != 0) {
        int move_compass, facing_compass;

        /* WRESTLE.ASM::change_walk_anim's leg half (WRESTLE.ASM:5000-5014):
           reselects hrt_leg_anims_table[MOVE_DIR][FACING_DIR] every tick.
           FACING_DIR is real (wm_arcade_update_newfacing + wm_execute_walk's
           WM_MOVE_ZIP catch-up, see wm/arcade/wm_arcade_closest.h and
           wm/movement.h) -- change_walk_anim's leg half itself never writes
           FACING_DIR, so this doesn't either; it stays frozen at its last
           idle value while walking, exactly like the source. */
        move_compass = wm_convert_facing(actor->move_dir);
        facing_compass = wm_convert_facing(actor->facing_dir);
        start_if_new(&bva->visual, wm_bret_leg_anim(move_compass, facing_compass));

        /* WRESTLE.ASM::change_walk_anim's torso half (WRESTLE.ASM:4973-4997):
           reselects hrt_torso_anims_table[FACING_DIR][NEW_FACING_DIR], gated
           on MODE_UNINT only (unlike the leg half, which always runs once
           change_walk_anim is called at all) -- but change_walk_anim itself
           is only ever called from the 8 real-movement walk_table handlers,
           never from #zip, so this whole block is correctly nested under
           MOVE_DIR!=0, not run while idle. Both indices are now real and the
           table is fully wired (wm_bret_torso_anim), including the 12
           off-diagonal turn-transition entries. */
        if (!(actor->anim_mode & WM_MODE_UNINT)) {
            start_if_new(&bva->torso_visual,
                         wm_bret_torso_anim(facing_compass,
                                            wm_convert_facing(actor->new_facing_dir)));
        }
    } else {
        /* WRESTLE.ASM::set_rotate_anim (WRESTLE.ASM:5062-5088), called only
           from #zip/do_stance (WRESTLE.ASM:5286 `callr set_rotate_anim ;or
           stance`): selects the LEG turn/stand anim from (OLD FACING_DIR,
           NEW_FACING_DIR), using FACING_DIR as it was *before* the
           WM_MOVE_ZIP catch-up above already applied it -- exactly what the
           real set_rotate_anim reads before it overwrites FACING_DIR itself.
           No torso reselection happens here: change_walk_anim (which drives
           the torso) is never called from #zip in the source. */
        int old_facing_compass = wm_convert_facing(old_facing_dir);
        int new_facing_compass = wm_convert_facing(actor->new_facing_dir);
        start_if_new(&bva->visual, wm_bret_rotate_anim(old_facing_compass, new_facing_compass));
    }
}

/* wm_arcade_bret_callbacks_t.mode_dead body: DOINK.ASM's shared mode_dead
   (see wm/arcade/wm_arcade_mode_dead.h) needs nothing but the actor
   itself in this port's always-reached subset. */
static void wm_bret_backend_mode_dead(wm_arcade_actor_t *actor, void *user) {
    (void)user;
    wm_arcade_mode_dead(actor);
}

/* wm_arcade_bret_callbacks_t.check_secret_moves body: WRESTLE.ASM's real
   update_joystat + check_secret_moves (see wm/arcade/wm_arcade_joystat.h
   for the full derivation). update_joystat's own recording happens here,
   first, so this tick's input is guaranteed fresh by the time the
   patterns are scanned -- the source relies on its own per-process
   instruction ordering (update_joystat runs earlier in the same tick,
   before move_bret gets called) for the same guarantee. */
static void wm_bret_backend_check_secret_moves(wm_arcade_actor_t *actor,
                                               const wm_arcade_bret_secret_pattern_t *patterns,
                                               size_t count, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    wm_arcade_bret_callbacks_t cb;
    uint16_t now;
    uint16_t punch_dtime, powerp_dtime, powerk_dtime;
    size_t i;

    if (!actor || !bva) return;

    now = (uint16_t)bva->pcnt;
    wm_arcade_joystat_update(&bva->joystat, actor, now);

    /* wm_arcade_update_joy_dtime resets a button's dtime to 0 the instant
       BUT_VAL_CUR reads it as no longer held -- the exact tick a release
       is checked below. The real source's own update_joy_dtime and
       check_secret_moves run from two different points in the per-process
       loop, in an order that only makes sense if the dtime read on a
       release tick reflects the duration accumulated *through the
       previous tick*, not this tick's already-reset value -- so these are
       captured before updating, not after. */
    punch_dtime = actor->punch_dtime;
    powerp_dtime = actor->powerp_dtime;
    powerk_dtime = actor->powerk_dtime;
    wm_arcade_update_joy_dtime(actor);

    /* WRESTLE.ASM:4851-4862 top-of-function gates. */
    if (actor->immobilize_time) return;
    if (actor->player_mode == WM_PMODE_DIZZY || actor->player_mode == WM_PMODE_WAITANIM) return;
    if (actor->getup_time) return;

    cb = wm_bret_backend_callbacks(bva);

    /* hrt_charge_flying_kick/hrt_charge_face_rake (BRET.ASM:543/614): two
       real, independent persistent-process "hold the button, then release
       it" watchers -- checked every tick, same as bret_secret_moves' own
       first entry below, regardless of the joystick-history recognizer
       that follows. wm_arcade_bret_release_charge_flying_kick/
       release_charge_face_rake already carry their own real preconditions
       (charge>=100, GETUP_TIME, PLYRMODE, MODE_UNINT, ...); this just
       supplies the real BUT_VAL_UP edge and dtime they need. */
    if ((actor->but_val_up & WM_BTN_SKICK) &&
        wm_arcade_bret_release_charge_flying_kick(actor, bva->opponent, powerk_dtime, &cb)) {
        return;
    }
    if ((actor->but_val_up & WM_BTN_PUNCH) &&
        wm_arcade_bret_release_charge_face_rake(actor, punch_dtime, &cb)) {
        return;
    }

    /* WRESTLE.ASM's own charge_ddt "hold test": bret_secret_moves' real
       first entry (executable code, not a value/mask table -- see
       wm/arcade/wm_arcade_joystat.h's own comment), checked every tick and
       taking priority over the table scan below when it fires, exactly
       like the source's own "call a0 / jrc #done". */
    if (wm_arcade_bret_try_charge_ddt(actor, bva->opponent, powerp_dtime, &cb))
        return;

    /* "only check if newest entry in queue is fresh": nothing was
       recorded this exact tick, so no pattern can possibly be the one
       that just completed. */
    if (bva->joystat.entries[0].tickcount != now) return;

    /* WRESTLE.ASM's own zombie check (B_ZOMBIE) is skipped: WM_STATUS_ZOMBIE
       is never set on a Bret actor in this port (see wm/arcade/
       wm_arcade_mode_dead.h's own boundary), so it would never fire. */

    for (i = 0; i < count; ++i) {
        if (wm_arcade_joystat_matches(&bva->joystat, now, patterns[i].steps,
                                      patterns[i].step_count, patterns[i].max_ticks)) {
            wm_arcade_bret_fire_secret(actor, bva->opponent, patterns[i].id, now, &cb);
            return;
        }
    }
}

wm_arcade_bret_callbacks_t wm_bret_backend_callbacks(wm_bret_backend_actor *bva) {
    wm_arcade_bret_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.change_anim = wm_bret_backend_change_anim;
    cb.change_torso_anim = wm_bret_backend_change_torso_anim;
    cb.execute_walk = wm_bret_backend_execute_walk;
    cb.adjust_health = wm_bret_backend_adjust_health;
    cb.mode_dead = wm_bret_backend_mode_dead;
    cb.check_secret_moves = wm_bret_backend_check_secret_moves;
    cb.user = bva;
    return cb;
}

void wm_bret_backend_tick(wm_bret_backend_actor *bva, wm_arcade_actor_t *actor,
                          uint16_t round_tickcount) {
    const wm_bret_attack_window_t *w;
    bool at_current_anim;
    size_t leg_old_frame_index, torso_old_frame_index;

    if (!bva) return;
    /* hrt_4_block_anim's ANI_WAITRELEASE,PLAYER_BLOCK_BIT: park on its own
       frame 1 while the block button is still held, so the animation (and
       with it WM_PMODE_BLOCK) lasts exactly as long as the player holds
       block, instead of running straight through in nine ticks. */
    if (block_holding_waitrelease(bva, actor)) bva->visual.ticks_left = 2;
    leg_old_frame_index = bva->visual.frame_index;
    wm_visual_tick(&bva->visual);
    torso_old_frame_index = bva->torso_visual.frame_index;
    wm_visual_tick(&bva->torso_visual);

    /* HRTSEQ1.ASM's ANI_SETFACING (torso_turn_setfacing above) and
       ANI_XFLIP (leg_turn_xflip above): each fires exactly once per
       crossing into a marked frame, matching the real animation
       interpreter executing that command exactly once as it reaches that
       point in the sequence -- not every tick the frame is held. */
    if (actor && bva->torso_visual.frame_index != torso_old_frame_index &&
        frame_index_is_marked(find_torso_turn_setfacing(bva->torso_visual.sequence),
                              bva->torso_visual.frame_index)) {
        actor->facing_dir = actor->new_facing_dir;
    }
    if (actor && bva->visual.frame_index != leg_old_frame_index &&
        frame_index_is_marked(find_leg_turn_xflip(bva->visual.sequence),
                              bva->visual.frame_index)) {
        actor->obj_control = (uint16_t)(actor->obj_control ^ WM_OBJ_FLIPH);
    }

    if (!actor) return;

    {
        const wm_visual_frame *cur = wm_visual_current(&bva->visual);
        wm_arcade_frame_box_t box =
            wm_bret_hurt_box_for_frame(cur ? cur->source_frame : NULL);
        wm_arcade_set_hurt_box(actor, &box);
    }

    /* Matched on (id, frame) rather than id alone, so an animation with
       several real ANI_ATTACK_ON pulses fires each one at its own frame and
       the ANI_ATTACK_OFF between them still runs on the frames in
       between -- exactly the ON/OFF/ON shape the source writes out. */
    at_current_anim = bva->visual.sequence == wm_bret_anim_sequence(bva->current_id);
    w = at_current_anim ? find_attack_window_at(bva->current_id, bva->visual.frame_index)
                        : NULL;

    /* ANIM.ASM's inline motion commands at this frame: the velocity sets,
       zeroes, offsets and friction that make a wired attack actually move
       instead of playing its frames on the spot. Frame 0 is applied on
       selection by wm_bret_backend_change_anim, so it is skipped here
       rather than re-applied every tick the animation sits on frame 0. */
    if (at_current_anim && bva->visual.frame_index != 0 &&
        bva->visual.sequence &&
        bva->visual.frame_index != leg_old_frame_index) {
        const char *become = wm_anim_apply_frame_commands(
            actor, bva->visual.sequence->source_label, bva->visual.frame_index);
        if (become) bva->pending_become = become;
    }

    /* An ANI_IFBUTTONS whose buttons were all held: the animation becomes
       the named one. The only case in Bret's own set is start_run_anim, the
       run cancel out of an attack's opening frames -- pressing punch+kick
       mid-punch really does turn it into a run in the source. */
    if (actor && bva->pending_become) {
        const char *become = bva->pending_become;
        bva->pending_become = NULL;
        if (strcmp(become, "start_run_anim") == 0) {
            actor->anim_mode &= (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
            wm_bret_backend_change_anim(actor, WM_BRET_ANIM_START_RUN, bva);
        }
    }

    /* A mid-animation ANI_SETPLYRMODE (plyrmode_changes): applied the tick
       the animation reaches that frame. Frame 0 rows are the header's own,
       already applied by wm_bret_backend_change_anim on selection, so they
       are skipped here rather than re-applied every time the animation
       happens to sit on frame 0. */
    if (at_current_anim && bva->visual.frame_index != 0 &&
        actor->player_mode != WM_PMODE_DEAD) {
        const wm_bret_plyrmode_change_t *pm =
            find_plyrmode_change(bva->current_id, bva->visual.frame_index);
        if (pm) actor->player_mode = pm->player_mode;
    }

    if (w) {
        if (!bva->attack_active) {
            if (w->use_z) wm_arcade_ani_attack_on_z(actor, &w->z_args);
            else wm_arcade_ani_attack_on(actor, &w->args);
            bva->attack_active = true;
        }
    } else if (bva->attack_active) {
        wm_arcade_ani_attack_off(actor, round_tickcount);
        bva->attack_active = false;
    }

    /* The other half of wm_bret_backend_change_anim's MODE_UNINT: clears
       back to MODE_NORMAL once the attack animation naturally ends, same
       as its own HRTSEQ2.ASM header's trailing `ANI_SETMODE,MODE_NORMAL`. */
    if (at_current_anim && anim_header_sets_uninit(bva->current_id) && bva->visual.ended)
        actor->anim_mode &= (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP |
                                        anim_header_mode_bits(bva->current_id));

    /* secret_move_sets_mode_uninit's own ids have no frame data to time a
       real end from, so the WM_MODE_UNINT wm_bret_backend_change_anim set
       for one of them earlier this same tick (protecting it from being
       reselected by this tick's own player_mode dispatch continuation)
       gets cleared right back off here, rather than left set indefinitely
       -- see that function's own comment for why this is a deliberate,
       bounded simplification rather than the source's real per-animation
       duration. */
    if (bva->pending_uninit_clear) {
        actor->anim_mode &= (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
        bva->pending_uninit_clear = false;
    }

    /* ANI_CHANGEANIM's own hand-off: an animation that ends by BECOMING
       another does not stop -- the target starts, with its own header
       commands, attack windows and mode bits, exactly as if it had been
       selected. Driven through wm_bret_backend_change_anim so none of that
       is special-cased, and applied on the tick the source animation
       genuinely ends. */
    if (actor && at_current_anim && bva->visual.ended) {
        wm_arcade_bret_anim_id_t next_id;
        if (find_anim_transition(bva->current_id, &next_id)) {
            /* Clear the finished animation's own protection first, so the
               target's header sets its own rather than inheriting. */
            actor->anim_mode &= (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP |
                                            anim_header_mode_bits(bva->current_id));
            wm_bret_backend_change_anim(actor, next_id, bva);
        }
    }

    /* hrt_4_block_anim's own tail, once the WAITRELEASE hold above has let
       it run off its last frame: ANI_SETPLYRMODE,MODE_NORMAL followed by
       ANI_END, plus the matching clear of the MODE_UNINT|MODE_NOAUTOFLIP
       its header set. */
    if (actor && bva->visual.sequence == &wm_bret_block4_anim && bva->visual.ended) {
        if (actor->player_mode == WM_PMODE_BLOCK)
            actor->player_mode = WM_PMODE_NORMAL;
        actor->anim_mode &= (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
    }
}

void wm_bret_backend_tick_position(wm_arcade_actor_t *actor) {
    wm_integrate_position(actor);
}
