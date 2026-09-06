#include "wm/bret_backend.h"
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
        /* HRTSEQ3.ASM hrt_3_pile_driver_anim: the finish
           hrt_knees_to_head_anim hands off to when the player has kept
           mashing super kick through the last knee. */
        case WM_BRET_ANIM_PILE_DRIVER3: return &wm_bret_pile_driver3_anim;
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
 * The animation id whose sequence carries `label`, or -1.
 *
 * An ANI_CHANGEANIM or an ANI_IFBUTTONS names its target by source label,
 * which is what the program carries; the rest of the backend is keyed on
 * this port's own ids. Resolved by asking the id->sequence map itself
 * rather than from a second hand-written table, so a label can never drift
 * away from the sequence it names.
 */
static int anim_id_for_label(const char *label) {
    int id;
    if (!label) return -1;
    /* WRESTLE2.ASM:3443 start_run_anim is a state-setup routine with no WL
       frames of its own, so it has no sequence to find it by. */
    if (strcmp(label, "start_run_anim") == 0)
        return (int)WM_BRET_ANIM_START_RUN;
    for (id = 0; id <= (int)WM_BRET_ANIM_HITONGROUND_FACEDOWN; ++id) {
        const wm_visual_sequence *s =
            wm_bret_anim_sequence((wm_arcade_bret_anim_id_t)id);
        if (s && s->source_label && strcmp(s->source_label, label) == 0)
            return id;
    }
    return -1;
}


/*
 * The attack windows, the animation transitions, the mid-animation
 * ANI_SETPLYRMODE rows, the per-id header mode bits and the "this attack
 * sets facing on start" list all used to live here, each of them a table
 * keyed on (animation id, flat frame index).
 *
 * They are gone because the program subsumes every one of them. An attack
 * box is an ANI_ATTACK_ON op sitting at the point in the op stream where
 * the source writes it; a mode word is an ANI_SETMODE op; a hand-off is an
 * ANI_CHANGEANIM. Running the ops applies each of them exactly where and
 * when ANIM.ASM does, on whichever branch the animation actually took --
 * which is what a frame-index key could not do, since the flat list it
 * indexed into was every branch spliced end to end. hrt_4_ground_punch_anim
 * is the clearest case: it writes its follow-up twice, once for the hit and
 * once for the miss, so the flat list carried three attack boxes where any
 * real playthrough fires two.
 */

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

wm_arcade_frame_box_t wm_hurt_box_for_frame(const char *source_frame) {
    wm_arcade_frame_box_t box;
    const wm_frame_geometry_t *geo;

    box.iani3x = 0;
    box.iani3y = 0;
    box.iani3z = 0;
    box.iani3id = 0;

    geo = source_frame ? wm_frame_geometry_find(source_frame) : NULL;
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

    /*
     * If this animation has a generated ANIM.ASM program, it drives: the
     * program's own header ops are the mode bits, the player mode, the
     * facing set, the velocities and the attack boxes below, so running
     * both would apply every one of them twice. wm_bret_backend_tick takes
     * the same fork.
     */
    {
        const wm_anim_program *prog =
            seq ? wm_anim_program_find(seq->source_label) : NULL;
        if (prog) {
            /* An animation the program is taking over from another still
               owns whatever attack box the previous one left on; the new
               program's own ATTACK_ON/OFF ops take it from here. */
            if (bva->attack_active) {
                wm_arcade_ani_attack_off(actor, bva->round_tickcount);
                bva->attack_active = false;
            }
            bva->anim_env.opponent = bva->opponent;
            wm_anim_exec_start(&bva->prog, prog, actor,
                               bva->round_tickcount, &bva->anim_env);
            return;
        }
        bva->prog.program = NULL;
        bva->prog.ended = true;
    }

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

    /*
     * What is left here is only the ids the program path cannot take:
     * WM_BRET_ANIM_START_RUN, whose source routine is state setup with no
     * WL frames of its own, and the secret-move targets that have no
     * extracted animation at all. Every id that does have one is
     * program-driven above, where its own header ops carry these same
     * writes.
     */
    if (id == WM_BRET_ANIM_START_RUN) {
        /* WRESTLE2.ASM:3443 start_run_anim's own ANI_CODE #setup_run
           (:3452): pick a run direction, clear the getup/run timers, face
           that way and enter MODE RUNNING, behind the header's own
           ANI_SETMODE,MODE_UNINT|MODE_NOAUTOFLIP. */
        wm_arcade_start_run(actor);
        actor->anim_mode |= (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
    } else if (secret_move_sets_mode_uninit(id)) {
        /* secret_move_sets_mode_uninit's own comment: no frame data means
           no real way to time this back off, so it's cleared this same
           tick instead (wm_bret_backend_tick) rather than left set. */
        actor->anim_mode |= (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
        bva->pending_uninit_clear = true;
        /* Three of those secret-move targets carry a real, instant
           ANI_SETFACING at their own start. Gated on `is_new_selection`,
           not every call, since the source command fires exactly once, the
           instant the animation is selected. */
        if (id == WM_BRET_ANIM_HIPTOSS ||
            id == WM_BRET_ANIM_GRABFLING_FACE24)
            actor->facing_dir = actor->new_facing_dir;
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

/*
 * One tick of a program-driven animation.
 *
 * Everything the side tables used to supply is an op here: ANI_ATTACK_ON /
 * ANI_ATTACK_ON_Z / ANI_ATTACK_OFF turn the attack box on and off at the
 * point in the sequence the source turns it on and off, ANI_SETMODE and
 * ANI_SETPLYRMODE write the modes, and the velocity/offset/friction
 * commands move the wrestler -- all of them run by the interpreter as it
 * reaches them, on whichever branch the animation actually took.
 */
static void wm_bret_backend_tick_program(wm_bret_backend_actor *bva,
                                         wm_arcade_actor_t *actor,
                                         uint16_t round_tickcount) {
    const char *frame;

    wm_anim_exec_tick(&bva->prog, actor, round_tickcount);

    frame = wm_anim_exec_frame(&bva->prog);
    if (frame) {
        wm_arcade_frame_box_t box = wm_hurt_box_for_frame(frame);
        wm_arcade_set_hurt_box(actor, &box);
    }

    if (!bva->prog.ended) return;

    /*
     * ANI_CHANGEANIM's hand-off, and ANI_IFBUTTONS' cancel: an animation
     * that ends by BECOMING another does not stop -- the target starts with
     * its own header, exactly as if it had been selected. The MODE_UNINT
     * the finished animation set is cleared first so the target's own
     * header sets its own rather than inheriting it; a target with no
     * program of its own falls back to the flat path, which needs that
     * clear for the same reason.
     */
    if (bva->prog.become) {
        int next = anim_id_for_label(bva->prog.become);
        bva->prog.become = NULL;
        bva->prog.program = NULL;
        actor->anim_mode &= (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
        if (next >= 0) {
            wm_bret_backend_change_anim(actor,
                                        (wm_arcade_bret_anim_id_t)next, bva);
            return;
        }
        /* A label this port has no animation for: the wrestler is left
           interruptible rather than stuck in a program that has ended. */
        return;
    }

    /* The animation ran to its own ANI_END. Its trailing ANI_SETMODE has
       already written whatever ANIMODE it leaves behind (HRTSEQ2's attack
       headers end on MODE_NORMAL, which is an absolute write of 0), so
       there is nothing here to clear by hand. */
    bva->prog.program = NULL;
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
    size_t leg_old_frame_index, torso_old_frame_index;

    if (!bva) return;
    bva->round_tickcount = round_tickcount;
    /* hrt_4_block_anim's ANI_WAITRELEASE,PLAYER_BLOCK_BIT: park on its own
       frame 1 while the block button is still held, so the animation (and
       with it WM_PMODE_BLOCK) lasts exactly as long as the player holds
       block, instead of running straight through in nine ticks. */
    if (block_holding_waitrelease(bva, actor)) {
        /* Both tracks are held: the flat one because callers still read
           its frame index, the program one because it is what actually
           runs the animation. hrt_4_block_anim's program is unbranching
           and shares the flat list's three frames, so they stay in step
           and the same frame-1 test answers for both. */
        bva->visual.ticks_left = 2;
        if (bva->prog.program) bva->prog.ticks_left = 2;
    }
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

    /*
     * A program-driven animation runs from its ops and nothing below
     * applies to it: the attack boxes, the mode and player-mode writes,
     * the motion commands and the end are all in the op stream, so the
     * side tables would only apply each of them a second time -- and on
     * the wrong frame whenever a branch has taken the animation down a
     * path the flat list cannot express. The flat track keeps ticking
     * alongside purely so callers reading `visual` still see which
     * sequence is playing.
     */
    if (bva->prog.program) {
        wm_bret_backend_tick_program(bva, actor, round_tickcount);
        return;
    }

    {
        const wm_visual_frame *cur = wm_visual_current(&bva->visual);
        wm_arcade_frame_box_t box =
            wm_hurt_box_for_frame(cur ? cur->source_frame : NULL);
        wm_arcade_set_hurt_box(actor, &box);
    }

    /* What reaches here is a walk, turn or stand cycle -- an unbranching
       frame loop with no commands in it -- or one of the secret-move ids
       that has no extracted animation at all. There is no attack box, no
       mode write and no hand-off among them; everything that had one is
       program-driven above. */

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
}

void wm_bret_backend_tick_position(wm_arcade_actor_t *actor) {
    wm_integrate_position(actor);
}
