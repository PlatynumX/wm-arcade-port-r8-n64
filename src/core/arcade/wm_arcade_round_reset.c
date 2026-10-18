/*
 * WRESTLE.ASM's between-round reset and the three small routines
 * around it -- see wm/arcade/wm_arcade_round_reset.h.
 */
#include "wm/arcade/wm_arcade_round_reset.h"
#include "wm/arcade/wm_arcade_combat_defs.h"
#include "wm/arcade/wmania_ring_geometry.h"

#include <stddef.h>

/*
 * WRESTLE.ASM:2887. `.word X,Z,face_dir,unused`, three rows a side.
 * Team 1's Z values are based at 1127 and team 2's at 1103 -- the
 * source's own constants, twenty-four apart, so the two sides do not
 * start on exactly the same depth line.
 */
const wm_round_start_t wm_round_team_starts[2][WM_ROUND_STARTS_PER_TEAM] = {
    {   /* #team1_starts */
        { WM_RING_X_CENTER - 85,  1127 + 93,  WM_MOVE_UP_RIGHT },
        { WM_RING_X_CENTER - 150, 1127 + 170, WM_MOVE_UP_RIGHT },
        { WM_RING_X_CENTER - 20,  1127 + 16,  WM_MOVE_DOWN_RIGHT },
    },
    {   /* #team2_starts */
        { WM_RING_X_CENTER + 85,  1103 + 93,  WM_MOVE_DOWN_LEFT },
        { WM_RING_X_CENTER + 150, 1103 + 170, WM_MOVE_UP_LEFT },
        { WM_RING_X_CENTER + 20,  1103 + 16,  WM_MOVE_DOWN_LEFT },
    },
};

int wm_round_start_index(const wm_arcade_actor_t *a,
                         wm_arcade_actor_t *const *actors, size_t count) {
    size_t i;
    int index = 0;

    if (!a || !actors) return -1;

    /* `#loop0`: walk process_ptrs, skip an inactive slot, stop on
       yourself, and count anyone on your own side you passed. */
    for (i = 0; i < count; ++i) {
        const wm_arcade_actor_t *o = actors[i];
        if (!o || !o->active) continue;
        if (o == a) return index;
        if (o->player_side == a->player_side) ++index;
    }
    return -1;                      /* not in the list at all */
}

void wm_round_reset_wrestler(wm_arcade_actor_t *a,
                             wm_arcade_actor_t *const *actors, size_t count,
                             uint32_t pcnt) {
    const wm_round_start_t *row;
    int index, side;
    int32_t ground, y;

    if (!a) return;

    a->delay_meter = WM_ROUND_DELAY_METER;

    index = wm_round_start_index(a, actors, count);
    if (index < 0 || index >= WM_ROUND_STARTS_PER_TEAM) index = 0;
    side = (a->player_side == 0) ? 0 : 1;   /* `TEST a0 / jrz #add` */
    row = &wm_round_team_starts[side][index];

    /* `move *a9+,a0 / sll 16,a0` -- the table is whole pixels and the
       position is 16.16. Y is cleared outright. */
    a->x_int = row->x;
    a->x_fixed = row->x << 16;
    a->z_int = row->z;
    a->z_fixed = row->z << 16;
    a->y_int = 0;
    a->y_fixed = 0;

    a->ground_y = WM_MAT_Y;

    /*
     * ";From veladd" -- the source inlines wrestler_veladd's ground
     * clamp here so the wrestler is standing ON the mat rather than
     * falling to it on the first tick. Y is zero and GROUND_Y is 62,
     * so the subtraction is negative, the branch is taken, the Y
     * velocity is zeroed and he lands on the mat exactly.
     */
    ground = (int32_t)WM_MAT_Y << 16;
    y = a->y_fixed - ground + a->y_vel;
    if (y < 0) {
        a->y_vel = 0;
        y = 0;
    }
    a->y_fixed = y + ground;
    a->y_int = a->y_fixed >> 16;

    a->facing_dir = row->facing;
    a->new_facing_dir = row->facing;

    a->player_mode = (uint16_t)WM_PMODE_NORMAL;   /* `clr a0` into PLYRMODE */
    a->plyr_dizzy = 0;
    a->anim_mode = 0;

    /* `clr a0 / move a0,*a13(INRING)`. Zero INRING is INSIDE the
       ring; this port's boolean reads the other way round. */
    a->in_ring = 1;

    a->x_vel = 0;
    a->y_vel = 0;
    a->z_vel = 0;

    /* `move @PCNT,a14 / move a14,*a13(FOOT_PCNT),W` -- a WORD store,
       so the stamp is truncated. */
    a->foot_pcnt = (uint16_t)pcnt;
}

void wm_round_reset_wrestler2(wm_arcade_actor_t *a) {
    if (!a) return;

    a->immobilize_time = WM_ROUND_IMMOBILIZE;

    a->getup_time = 0;
    a->special_move_addr = 0;
    a->last_hit_time = 0;
    a->last_headhold = 0;
    a->last_spunch = 0;
    a->last_skick = 0;
    a->consecutive_hits = 0;
    a->last_fling = 0;
    a->last_hiptoss = 0;
    a->last_damage = 0;

    /* `andi SF_RESET_MASK,a14` -- keep TEMP_PAL and DID_BUCKOFF, drop
       everything else. */
    a->status_flags &= (uint32_t)WM_SF_RESET_MASK;

    /* "just in case they were KO'd last round". */
    a->ptime = 1;
}

void wm_round_update_links(wm_arcade_actor_t *a) {
    if (!a || !a->attach_proc) return;          /* `jrz #exit` */
    /* `move *a1(ATTACH_PROC),a0,L / cmp a0,a13 / jreq #exit`. */
    if (a->attach_proc->attach_proc == a) return;
    a->attach_proc = NULL;
}

void wm_round_init_scroller(int32_t *worldtlx, int32_t *worldtly,
                            int32_t num_opps) {
    if (worldtlx) *worldtlx = (int32_t)(WM_RING_X_CENTER - 200) << 16;
    if (worldtly)
        *worldtly = (num_opps == 2) ? WM_SCROLL_START_Y_1V2
                                    : WM_SCROLL_START_Y_DEFAULT;
}

int wm_round_most_damage(const int32_t *damage_given, size_t count,
                         const int32_t *plyr_type, int32_t *out_percent) {
    int32_t per_player[2] = { 0, 0 };
    int32_t total = 0;
    int32_t best;
    int who;
    size_t i;

    if (out_percent) *out_percent = 0;
    if (!damage_given) return 0;

    /* `#find_damage_lp`: drones contribute nothing and are not even
       added to the total, so the percentage is of the HUMANS' damage. */
    for (i = 0; i < count; ++i) {
        int32_t num;
        if (plyr_type && plyr_type[i] != 0) continue;
        total += damage_given[i];
        num = (int32_t)i;                 /* `move *a9(PLYRNUM),a10` */
        if (num >= 0 && num < 2) per_player[num] += damage_given[i];
    }
    if (total == 0) return 0;             /* the source would divide by 0 */

    /* `cmp a8,a9 / jrlt #p1_most` -- player 1 only on strictly more. */
    if (per_player[1] < per_player[0]) {
        who = 1;
        best = per_player[0];
    } else {
        who = 2;
        best = per_player[1];
    }

    /* `mpyu 100 / divu total`, unsigned and truncating. */
    if (out_percent)
        *out_percent = (int32_t)(((uint32_t)best * 100u) / (uint32_t)total);
    return who;
}

uint16_t wm_round_loser_snd(int32_t pstatus, int32_t match_winner,
                            const int32_t *old_winstreak, WmRng *rng) {
    /* SOUND.EQU's three. The four the source keeps commented out
       around them are not here: they are not in the shipped table. */
    static const uint16_t SPEECH[3] = {
        WM_SND_SOMEHOW_I_DONT_THINK,
        WM_SND_L_BACK_TO_SANDBOX,
        WM_SND_ARE_YOU_TOUGH_ENOUGH,
    };
    int loser;
    uint32_t pick;

    if (pstatus != 3) return 0;              /* two humans only */
    if (!old_winstreak) return 0;

    /* `NOT A1 / ANDI 3,A1 / DEC A1` -- the winner's index turned into
       the loser's. 1 -> 1, 2 -> 0; anything else falls outside. */
    loser = (int)((((uint32_t)~(uint32_t)match_winner) & 3u)) - 1;
    if (loser < 0 || loser > 1) return 0;

    if (old_winstreak[loser] == 0) return 0; /* no streak to break */

    pick = wm_rng_rndrng0(rng, 2);
    if (pick > 2) pick = 2;
    return SPEECH[pick];
}

bool wm_round_maybe_do_flashes(int32_t worldtly, int32_t reduce_bog) {
    if (reduce_bog != 0) return false;       /* `jrnz #die` */
    /* `CMPI [>ff97,0],A0 / JRGT #top` -- signed, and it keeps polling
       while the camera is BELOW the line. */
    if (worldtly > WM_FLASHES_Y_LIMIT) return false;
    return true;
}
