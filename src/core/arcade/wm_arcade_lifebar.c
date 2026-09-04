#include "wm/arcade/wm_arcade_lifebar.h"

void wm_arcade_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                             wm_arcade_actor_t *damage_source,
                             bool attract_mode, uint32_t pcnt,
                             int32_t *dam_mult) {
    int32_t life;
    int32_t d = delta;

    if (!victim) return;

    if (damage_source && damage_source->combo_count != 0) {
        /* LIFEBAR.ASM:1429-1447: "doing a combo" damage entirely replaces
           the original hit's delta. */
        int32_t magnitude = 10 - damage_source->combo_count;
        if (magnitude < 4) magnitude = 4;
        d = -magnitude;
        if (dam_mult) *dam_mult = 0;
    } else if (dam_mult && *dam_mult != 0) {
        /* LIFEBAR.ASM:1449-1465: damage *= (1+DAM_MULT)/2, e.g. DAM_MULT=2
           -> x1.5, DAM_MULT=4 -> x2.5. sra 1 floors toward -infinity for
           negative values, matching C's >> on a negative signed int here. */
        d = d * (1 + *dam_mult);
        d >>= 1;
        *dam_mult = 0;
    }

    if (d < 0) {
        /* LIFEBAR.ASM:1471-1521 damage_mod_table lookup ("unless we're
           adding life"). wm_match's actors[] never holds more than the
           fixed pair wm_match_start_attract/selected create, so the
           source's active-drone count (is_8_on_1/buddy_mode_on/
           royal_rumble, none of which exist in this port, or else a count
           of process slots beyond the first two) always resolves to the
           table's first row here -- and that row's drone/player columns
           are both _85PCT, so no PLYR_TYPE branch is needed either. */
        d = (d * WM_ARCADE_DAMAGE_MOD_85PCT) >> 8;
    }

    life = (int32_t)victim->life + d;

    if (life <= 0) {
        if (life > -10 && d <= -20) {
            /* LIFEBAR.ASM:1561-1569 fudge. */
            life = 5;
        } else {
            /* LIFEBAR.ASM:1575-1581. */
            life = attract_mode ? WM_ARCADE_LIFE_MAX : 0;
            if (life == 0) {
                /* LIFEBAR.ASM:1659-1670: genuinely dying -- a live combo
                   on the attacker defers it instead. */
                if (damage_source && damage_source->combo_count != 0) {
                    life = 1;
                    victim->i_will_die = 1;
                } else {
                    /* LIFEBAR.ASM:1723-1725 SETMODE DEAD + wres_collis_off. */
                    victim->player_mode = WM_PMODE_DEAD;
                    wm_arcade_wrestler_collisions_off(victim);
                }
            }
        }
    } else if (life > WM_ARCADE_LIFE_MAX) {
        life = WM_ARCADE_LIFE_MAX;
    }

    victim->life = life;

    /* LIFEBAR.ASM:1593-1595, unconditional on every call. */
    victim->last_damage = (uint16_t)pcnt;
}
