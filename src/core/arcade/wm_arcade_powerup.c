/*
 * AWARD.ASM's powerup codes.
 * See include/wm/arcade/wm_arcade_powerup.h.
 */
#include "wm/arcade/wm_arcade_powerup.h"

#include <string.h>

/*
 * AWARD.ASM:1967-2180, in the order player_powerup_checker CREATEs
 * them. Two are not what they look like:
 *
 *   drone_meters has no sequence at all -- all three of its
 *   PUPWAITSWITCHes are commented out -- so wherever it runs it simply
 *   turns itself on. Its caller gates it instead: PSTATUS != 3 (not a
 *   two-player game) and NUM_OPPS == 1.
 *
 *   no_ring has a complete four-step sequence and is never started:
 *   its CREATE is commented out, "Disable until we get blimp module".
 */
const wm_powerup_code wm_powerup_codes[] = {
    { "no_block", WM_PU_BLOCKING_OFF, 3,
      { WM_PUP_BLOCK, WM_PUP_BLOCK, WM_PUP_BLOCK, 0, 0 }, true, true },
    { "move_names", WM_PU_MOVE_NAMES_ON, 4,
      { WM_PUP_PUNCH, WM_PUP_PUNCH, WM_PUP_PUNCH, WM_PUP_PUNCH, 0 },
      false, true },
    { "drone_meters", WM_PU_D_METERS_ON, 0,
      { 0, 0, 0, 0, 0 }, false, true },
    { "hyper_match", WM_PU_HYPER_MATCH_ON, 4,
      { WM_PUP_LEFT, WM_PUP_PUNCH, WM_PUP_PUNCH, WM_PUP_BLOCK, 0 },
      false, true },
    { "combos", WM_PU_COMBOS_ON, 3,
      { WM_PUP_RIGHT, WM_PUP_PUNCH, WM_PUP_SUPERP, 0, 0 }, false, true },
    { "ring_out", WM_PU_RING_OUTS_ON, 4,
      { WM_PUP_DOWN, WM_PUP_PUNCH, WM_PUP_KICK, WM_PUP_BLOCK, 0 },
      false, true },
    { "buddy_mode", WM_PU_BUDDY_MODE, 5,
      { WM_PUP_BLOCK, WM_PUP_BLOCK, WM_PUP_BLOCK, WM_PUP_BLOCK,
        WM_PUP_BLOCK }, true, true },
    { "no_ring", WM_PU_NO_RING, 4,
      { WM_PUP_UP, WM_PUP_RIGHT, WM_PUP_DOWN, WM_PUP_LEFT, 0 },
      false, false }
};

const int wm_powerup_code_count =
    (int)(sizeof(wm_powerup_codes) / sizeof(wm_powerup_codes[0]));

void wm_powerup_attempt_start(wm_powerup_attempt *att,
                              const wm_powerup_code *code)
{
    if (!att) {
        return;
    }
    memset(att, 0, sizeof(*att));
    att->code = code;
    att->timer = 0;             /* `clr a11` -- the first press waits forever */
}

bool wm_powerup_attempt_tick(wm_powerup_attempt *att, int32_t switches)
{
    if (!att || !att->code || att->dead) {
        return false;
    }
    if (att->at >= att->code->steps) {
        return false;           /* already entered */
    }
    /* `SLEEPK 1 / dec a11 / jrz FAILADDR`: the decrement is every tick
     * and comes before anything is read. From zero it wraps negative,
     * which is exactly why the opening press has no deadline. */
    --att->timer;
    if (att->timer == 0) {
        att->dead = true;
        return false;
    }
    if (switches == 0) {
        return false;           /* `jrz lp?` */
    }
    if (switches != att->code->step[att->at]) {
        return false;           /* `cmpi / jrnz lp?` -- no reset */
    }
    ++att->at;
    if (att->at == 1) {
        att->timer = WM_PUP_WINDOW;     /* `movi TSEC*2,a11`, once */
    }
    return att->at >= att->code->steps;
}

bool wm_powerup_attempt_done(const wm_powerup_attempt *att)
{
    return att && att->code && att->at >= att->code->steps;
}

void wm_powerup_reset(wm_powerup_flags *f)
{
    if (f) {
        memset(f, 0, sizeof(*f));
    }
}

void wm_get_powerups(wm_powerup_flags *f)
{
    uint32_t p1;
    uint32_t p2;
    uint32_t both;

    if (!f) {
        return;
    }
    p1 = f->p_request[0];
    p2 = f->p_request[1];

    both = (p1 & p2) & (uint32_t)WM_PU_BOTH_P_MASK;
    p1 = (p1 & ~(uint32_t)WM_PU_BOTH_P_MASK) | both;
    p2 = (p2 & ~(uint32_t)WM_PU_BOTH_P_MASK) | both;
    f->p_request[0] = p1;
    f->p_request[1] = p2;

    /* The both-players options are read off player 1's reconciled set,
     * which after the merge is the same as player 2's for those bits. */
    f->blocking_off = (p1 & WM_PU_BLOCKING_OFF) ? WM_BLOCKING_OFF_VALUE : 0;
    f->instant_combos_on = (int32_t)(p1 & WM_PU_COMBOS_ON);
    f->ring_out_on = (int32_t)(p1 & WM_PU_RING_OUTS_ON);
    f->no_ring_on = (int32_t)(p1 & WM_PU_NO_RING);
    f->hyper_speed_on = (p1 & WM_PU_HYPER_MATCH_ON) ? 1 : 0;
    /* `or a9,a10` then `move a10,a8` -- either player is enough. */
    f->drone_meters_on = (int32_t)((p1 | p2) & WM_PU_D_METERS_ON);
}
