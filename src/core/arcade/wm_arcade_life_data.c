/*
 * LIFEBAR.ASM's life_data table.
 */
#include "wm/arcade/wm_arcade_life_data.h"

#include <string.h>

static bool in_range(const wm_life_data *ld, int plyrnum)
{
    return ld && plyrnum >= 0 && plyrnum < WM_LIFE_NUM_WRES;
}

void wm_life_init_match(wm_life_data *ld)
{
    int i;

    if (!ld) {
        return;
    }
    for (i = 0; i < WM_LIFE_NUM_WRES; ++i) {
        ld->row[i].life = WM_LIFE_MAX;
        ld->row[i].turbo = WM_TURBO_MAX;
        /* `clr a4` then `move a4,*a1(PLT_CLIFE)` -- the displayed bar
         * starts EMPTY and inc_life runs it up. */
        ld->row[i].clife = 0;
        ld->row[i].combo_size = 0;
    }
}

void wm_life_init_wrestler(wm_life_data *ld, int plyrnum)
{
    if (!in_range(ld, plyrnum)) {
        return;
    }
    ld->row[plyrnum].life = WM_LIFE_MAX;
    ld->row[plyrnum].turbo = WM_TURBO_MAX;
    /* `move a2,*a1(PLT_CLIFE)` with a2 still LIFE_MAX -- this one
     * snaps, because it is replacing a wrestler who just died. */
    ld->row[plyrnum].clife = WM_LIFE_MAX;
    ld->row[plyrnum].combo_size = 0;
}

void wm_life_init_round(wm_life_data *ld)
{
    int i;

    if (!ld) {
        return;
    }
    for (i = 0; i < WM_LIFE_NUM_WRES; ++i) {
        ld->row[i].life = WM_LIFE_MAX;
        ld->row[i].clife = WM_LIFE_MAX;
        /* Neither combo_size nor turbo is touched here. */
    }
}

bool wm_life_inc_life(wm_life_data *ld, int plyrnum)
{
    int32_t life;
    int32_t clife;

    if (!in_range(ld, plyrnum)) {
        return false;
    }
    life = ld->row[plyrnum].life;
    clife = ld->row[plyrnum].clife;

    if (clife == life) {
        return false;
    }
    if (clife < life) {
        clife += WM_LIFEBAR_DELTAVEE;
        if (clife > life) {
            clife = life;       /* `#inrange` -- overshot it */
        }
    } else {
        clife -= WM_LIFEBAR_DELTAVEE;
        if (clife < life) {
            clife = life;
        }
    }
    ld->row[plyrnum].clife = clife;
    return clife != life;
}

int32_t wm_life_get_health(const wm_life_data *ld, int plyrnum)
{
    return in_range(ld, plyrnum) ? ld->row[plyrnum].life : 0;
}

void wm_life_clear(wm_life_data *ld, int plyrnum)
{
    if (in_range(ld, plyrnum)) {
        /* clear_lifebar writes PLT_LIFE only. */
        ld->row[plyrnum].life = 0;
    }
}

bool wm_life_turbo_is_ever_read(void)
{
    /*
     * Searched the whole source: PLT_TURBO appears five times -- its
     * own `equ`, and two write pairs in init_life_data and
     * init_wres_life_data. Nothing reads it, and there is no turbo bar
     * on screen. The answer is no, and this exists so that fact is
     * assertable rather than a comment nobody checks.
     */
    return false;
}
