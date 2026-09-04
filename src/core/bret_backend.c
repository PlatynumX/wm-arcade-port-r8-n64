#include "wm/bret_backend.h"
#include "wm/arcade/wm_arcade_anim_combat.h"
#include "wm/arcade/wm_arcade_lifebar.h"
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
        case WM_BRET_ANIM_SUPER_PUNCH2_4: return &wm_bret_power_punch_anim;
        case WM_BRET_ANIM_KICK2: return &wm_bret_light_kick2_anim;
        case WM_BRET_ANIM_KICK4: return &wm_bret_light_kick4_anim;
        /* HRTSEQ2.ASM:1334-1335: hrt_4_super_kick_anim is a literal SUBR
           alias of hrt_2_super_kick_anim, same address -- not distinct
           artwork, so both ids resolve to the same extracted sequence. */
        case WM_BRET_ANIM_SUPER_KICK2: return &wm_bret_power_kick_anim;
        case WM_BRET_ANIM_SUPER_KICK4: return &wm_bret_power_kick_anim;
        default: return NULL;
    }
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
 * Real ANIM.ASM ATTACK_ON(_Z) args for the 6 attack ids wm_bret_anim_sequence
 * maps, hand-traced against HRTSEQ2.ASM (see wm_bret_backend_tick's comment):
 * each attack's ANI_ATTACK_ON(_Z) command falls right before the WL frame
 * line at active_frame_index (0-based) in that id's wm_visual_sequence.
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
    /* HRTSEQ2.ASM:247 ANI_ATTACK_ON,AMODE_UPRCUT,-6,40,64,90 */
    { WM_BRET_ANIM_SUPER_PUNCH2_4, 5, false,
      { WM_AMODE_UPRCUT, -6, 40, 64, 90 }, {0,0,0,0,0,0,0} },
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
};
#define WM_BRET_ATTACK_WINDOW_COUNT \
    (sizeof(attack_windows) / sizeof(attack_windows[0]))

static const wm_bret_attack_window_t *find_attack_window(wm_arcade_bret_anim_id_t id) {
    size_t i;
    for (i = 0; i < WM_BRET_ATTACK_WINDOW_COUNT; ++i)
        if (attack_windows[i].id == id) return &attack_windows[i];
    return NULL;
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
 * its header directly, not assumed from the other 5. */
static bool attack_sets_facing_on_start(wm_arcade_bret_anim_id_t id) {
    switch (id) {
        case WM_BRET_ANIM_PUNCH2:
        case WM_BRET_ANIM_PUNCH4:
        case WM_BRET_ANIM_SUPER_PUNCH2_4:
        case WM_BRET_ANIM_KICK2:
        case WM_BRET_ANIM_SUPER_KICK2:
        case WM_BRET_ANIM_SUPER_KICK4:
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
    bool restarted;
    if (!bva) return;
    restarted = start_if_new(&bva->visual, wm_bret_anim_sequence(id));
    bva->current_id = id;
    if (!actor || !restarted) return;

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
    if (find_attack_window(id))
        actor->anim_mode |= (uint16_t)(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);

    /* attack_sets_facing_on_start's own comment: 5 of the 6 wired attacks
       also carry a real, instant ANI_SETFACING right at their own start
       (before hrt_4_kick_anim's real exception). Gated on `restarted`, not
       every call, since the source command fires exactly once, the instant
       the animation is selected -- not continuously for as long as it
       plays. */
    if (attack_sets_facing_on_start(id))
        actor->facing_dir = actor->new_facing_dir;
}

void wm_bret_backend_change_torso_anim(wm_arcade_actor_t *actor,
                                       wm_arcade_bret_anim_id_t id, void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    (void)actor;
    if (!bva) return;
    start_if_new(&bva->torso_visual, wm_bret_anim_sequence(id));
}

/* wm_arcade_bret_callbacks_t.adjust_health body: mode_normal's own
   I_WILL_DIE self-death call (BRET.ASM:1341-1343, "movi -10,a0 ... calla
   adjust_health") reads WHOHITME itself for the shared routine's
   combo-revival check, so this needs no opponent parameter beyond what the
   actor already carries. */
static void wm_bret_backend_adjust_health(wm_arcade_actor_t *actor, int delta,
                                          void *user) {
    wm_bret_backend_actor *bva = (wm_bret_backend_actor *)user;
    if (!actor) return;
    /* No wm_arcade_combat_runtime_t reachable from this self-death path
       (see wm_arcade_adjust_health's own comment): DAM_MULT tracking is
       skipped here, not guessed at. */
    wm_arcade_adjust_health(actor, (int16_t)delta, actor->who_hit_me,
                            bva ? bva->attract_mode : false,
                            bva ? bva->pcnt : 0, NULL);
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

wm_arcade_bret_callbacks_t wm_bret_backend_callbacks(wm_bret_backend_actor *bva) {
    wm_arcade_bret_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.change_anim = wm_bret_backend_change_anim;
    cb.change_torso_anim = wm_bret_backend_change_torso_anim;
    cb.execute_walk = wm_bret_backend_execute_walk;
    cb.adjust_health = wm_bret_backend_adjust_health;
    cb.user = bva;
    return cb;
}

void wm_bret_backend_tick(wm_bret_backend_actor *bva, wm_arcade_actor_t *actor,
                          uint16_t round_tickcount) {
    const wm_bret_attack_window_t *w;
    bool at_active_frame;
    size_t leg_old_frame_index, torso_old_frame_index;

    if (!bva) return;
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

    w = find_attack_window(bva->current_id);
    at_active_frame = w && bva->visual.sequence == wm_bret_anim_sequence(bva->current_id) &&
                      bva->visual.frame_index == w->active_frame_index;

    if (at_active_frame) {
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
    if (w && bva->visual.sequence == wm_bret_anim_sequence(bva->current_id) && bva->visual.ended)
        actor->anim_mode &= (uint16_t)~(WM_MODE_UNINT | WM_MODE_NOAUTOFLIP);
}

void wm_bret_backend_tick_position(wm_arcade_actor_t *actor) {
    wm_integrate_position(actor);
}
