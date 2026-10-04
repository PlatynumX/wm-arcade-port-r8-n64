/*
 * LIFEBAR.ASM's `life_data` table -- the per-wrestler life, displayed
 * life, turbo and combo-bar values, and the three routines that reset
 * them.
 *
 * The port already carries an actor `life` field and adjust_health;
 * what was missing is the table beside it, and in particular PLT_CLIFE
 * -- the *displayed* life, which chases the real one two pixels a tick
 * (`#lifebar_deltavee equ 2`). That chase is why a hit drains the bar
 * smoothly instead of snapping, and it is also why the bar fills up at
 * the start of a match: init_life_data leaves CLIFE at zero while LIFE
 * is already full, so the very first thing inc_life does is run it up.
 *
 * The three resets differ, and the differences are the content:
 *
 *   init_life_data       whole match. LIFE full, CLIFE ZERO, combo 0,
 *                        turbo full. The bar animates up from empty.
 *   init_wres_life_data  one wrestler, on a change_wrestler swap. LIFE
 *                        full, CLIFE FULL -- it snaps, because the new
 *                        wrestler is replacing a dead one mid-match --
 *                        combo 0, turbo full.
 *   init_rnd_life_data   round two onwards, everyone. LIFE and CLIFE
 *                        both full, and it does NOT touch combo or
 *                        turbo, unlike the other two.
 */
#ifndef WM_ARCADE_LIFE_DATA_H
#define WM_ARCADE_LIFE_DATA_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GAME.EQU:438 -- the table has one row per wrestler process slot. */
#define WM_LIFE_NUM_WRES 7

/* LIFEBAR.ASM:135-136 */
#define WM_LIFE_MAX 163                 /* green pixels in the life bar */
#define WM_TURBO_MAX (84 << 8)          /* 0x5400 */

/* LIFEBAR.ASM:2022 `#lifebar_deltavee equ 2  ;pixels per tick of motion` */
#define WM_LIFEBAR_DELTAVEE 2

/*
 * One row. The source's PLT_ fields are 16-bit words (`UHW`), and the
 * arithmetic below is done in the register width, so these are signed
 * 32-bit here and the routines clamp exactly where the source does.
 */
typedef struct wm_life_row {
    int32_t life;           /* PLT_LIFE   -- the real value */
    int32_t clife;          /* PLT_CLIFE  -- what the bar is showing */
    int32_t turbo;          /* PLT_TURBO */
    int32_t combo_size;     /* PLT_COMBO_SIZE */
} wm_life_row;

typedef struct wm_life_data {
    wm_life_row row[WM_LIFE_NUM_WRES];
} wm_life_data;

/* init_life_data (LIFEBAR.ASM:166): every wrestler, at match start.
 * CLIFE starts at zero so the bars fill in. */
void wm_life_init_match(wm_life_data *ld);

/* init_wres_life_data (LIFEBAR.ASM:199): one wrestler, on a swap.
 * CLIFE snaps to full. The name-object update it also does is display
 * and is not here. */
void wm_life_init_wrestler(wm_life_data *ld, int plyrnum);

/* init_rnd_life_data (LIFEBAR.ASM:238): everyone, at the start of round
 * two and later. Life and displayed life only -- combo and turbo are
 * deliberately left alone. The lifebar palette swap it also does now
 * has a real home in wm/arcade/wm_arcade_pal.h and belongs to the
 * caller. */
void wm_life_init_round(wm_life_data *ld);

/*
 * inc_life (LIFEBAR.ASM:2024): step the displayed value one tick toward
 * the real one, by WM_LIFEBAR_DELTAVEE, without overshooting. Returns
 * true while it still has ground to cover.
 */
bool wm_life_inc_life(wm_life_data *ld, int plyrnum);

/* get_health (LIFEBAR.ASM): the real life value, which is what
 * AWARD.ASM's comeback check compares against 80% of LIFE_MAX. */
int32_t wm_life_get_health(const wm_life_data *ld, int plyrnum);

/* clear_lifebar (LIFEBAR.ASM:5309) zeroes PLT_LIFE only; the displayed
 * value then drains to meet it. */
void wm_life_clear(wm_life_data *ld, int plyrnum);

/*
 * PLT_TURBO is written by two of the three resets and read by nothing
 * at all -- there is no other reference to it anywhere in the source.
 * It is kept because the resets write it, and because a field the
 * shipped game maintains and never consults is worth being able to see.
 */
bool wm_life_turbo_is_ever_read(void);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_LIFE_DATA_H */
