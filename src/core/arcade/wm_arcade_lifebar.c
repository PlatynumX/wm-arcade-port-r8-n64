#include "wm/arcade/wm_arcade_lifebar.h"

void wm_arcade_adjust_health(wm_arcade_actor_t *victim, int16_t delta,
                             wm_arcade_actor_t *damage_source,
                             bool attract_mode, uint32_t pcnt) {
    int32_t life;

    if (!victim) return;

    life = (int32_t)victim->life + delta;

    if (life <= 0) {
        if (life > -10 && delta <= -20) {
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
