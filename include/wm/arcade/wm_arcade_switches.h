/*
 * The cabinet's switch word, and every routine that reads it.
 *
 * MAIN.ASM:811 keeps a 64-bit latch in four parts -- current, previous,
 * newly-pressed and newly-released -- and WRESTLE2.ASM:2669 onward
 * slices individual sticks, buttons and start switches out of it by bit
 * offset. UTIL.ASM:2694 onward aggregates those across the two players.
 * None of that was translated: wm/human_input.h maps the N64 pad
 * straight onto the actor's cached fields and says so. This is the real
 * path, and it is what the AWARD.ASM powerup codes read.
 *
 * The latch is genuinely 64 bits wide and the reads genuinely cross the
 * halfway line: `but_offs2` is `20h-3`, so player 1's high buttons are a
 * 16-bit field starting at bit 0x1D and masked to bits 3 and 4 -- bits
 * 0x20 and 0x21 of the latch. Modelling it as two 32-bit words would
 * break that read.
 */
#ifndef WM_ARCADE_SWITCHES_H
#define WM_ARCADE_SWITCHES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WRESTLE2.ASM:2746, :2806-2807 and UTIL.ASM:2770 -- the bit offsets. */
#define WM_SW_MAX_PLAYERS 4
#define WM_SW_HUMAN_PLAYERS 2

extern const uint8_t wm_sw_joy_offs[WM_SW_MAX_PLAYERS];    /* 00,08,20,28 */
extern const uint8_t wm_sw_but_offs[WM_SW_MAX_PLAYERS];    /* 04,0c,24,2c */
extern const uint8_t wm_sw_but_offs2[WM_SW_HUMAN_PLAYERS]; /* 20h-3, 24h-3 */
extern const uint8_t wm_sw_start_offs[WM_SW_HUMAN_PLAYERS];/* 12h, 15h */

/* The masks each read applies after pulling its 16-bit field. */
#define WM_SW_STICK_MASK 0x0Fu      /* andi 01111b */
#define WM_SW_BUT_LOW_MASK 0x07u    /* andi 0111b  */
#define WM_SW_BUT_HIGH_MASK 0x18u   /* andi 011000b */
#define WM_SW_START_MASK 0x01u      /* andi 1 */

/*
 * MAIN.ASM:811. `switches_old` keeps last frame, and the two transition
 * words are derived, not sampled:
 *
 *     down = (old ^ new) & new        newly pressed
 *     up   = (new ^ old) & old        newly released
 */
typedef struct wm_switch_latch {
    uint64_t cur;
    uint64_t old;
    uint64_t down;
    uint64_t up;
} wm_switch_latch;

void wm_switch_latch_init(wm_switch_latch *sw);
void wm_switch_latch_update(wm_switch_latch *sw, uint64_t fswitch);

/* WRESTLE2.ASM:2700, :2719, :2737 -- players 0..3. */
uint16_t wm_get_stick_val_cur(const wm_switch_latch *sw, int player);
uint16_t wm_get_stick_val_down(const wm_switch_latch *sw, int player);
uint16_t wm_get_stick_val_up(const wm_switch_latch *sw, int player);

/* WRESTLE2.ASM:2669, :2757, :2788 -- players 0..1 only, because the
 * high two buttons come from but_offs2 and that table has two rows. */
uint16_t wm_get_but_val_cur(const wm_switch_latch *sw, int player);
uint16_t wm_get_but_val_down(const wm_switch_latch *sw, int player);
uint16_t wm_get_but_val_up(const wm_switch_latch *sw, int player);

/* UTIL.ASM:2750, :2765. */
uint16_t wm_get_start_cur(const wm_switch_latch *sw, int player);
uint16_t wm_get_start_down(const wm_switch_latch *sw, int player);

/*
 * UTIL.ASM's aggregators. The plain forms consult PSTATUS -- bit 0 is
 * "player 1 is in", bit 1 is "player 2" -- and skip a player who is
 * not. The `2` forms do not: they OR both players unconditionally,
 * which is how the credit screen reads a pad before anyone has paid.
 */
uint16_t wm_get_all_sticks_cur(const wm_switch_latch *sw, uint32_t pstatus);
uint16_t wm_get_all_sticks_cur2(const wm_switch_latch *sw);
uint16_t wm_get_all_sticks_down(const wm_switch_latch *sw, uint32_t pstatus);
uint16_t wm_get_all_sticks_down2(const wm_switch_latch *sw);
uint16_t wm_get_all_buttons_cur(const wm_switch_latch *sw, uint32_t pstatus);
uint16_t wm_get_all_buttons_cur2(const wm_switch_latch *sw);
uint16_t wm_get_all_buttons_down(const wm_switch_latch *sw, uint32_t pstatus);
uint16_t wm_get_all_buttons_down2(const wm_switch_latch *sw);
uint16_t wm_get_all_starts_cur(const wm_switch_latch *sw, uint32_t pstatus);
uint16_t wm_get_all_starts_down(const wm_switch_latch *sw, uint32_t pstatus);

/*
 * WRESTLE2.ASM's `#xflip_table`: turn an absolute stick reading into a
 * facing-relative one by swapping left and right. Up and down are
 * untouched, so it is exactly a swap of WM_MOVE_LEFT and WM_MOVE_RIGHT
 * -- the table is written out in full in the source and this
 * reproduces every one of its sixteen rows.
 */
uint16_t wm_stick_flip(uint16_t stick);

/*
 * read_switches (WRESTLE2.ASM:2820): one pass over the wrestlers that
 * caches all six readings plus two derived ones on each, "sacrificing
 * clarity for speed" in the source's own words.
 *
 * This is the per-wrestler body, decoupled from the actor struct.
 *
 *   immobilized  IMMOBILIZE_TIME > 0. Everything is zeroed, INCLUDING
 *                both relative values, and the derivation is skipped.
 *                Note it is strictly positive: a zero timer does not
 *                immobilize.
 *   drone        PLYR_TYPE != 0. The six readings come from the
 *                wrestler's own DRN_* fields instead of the latch.
 *   facing_right the FACING_DIR PLAYER_RIGHT_BIT test the relative
 *                values use. Worth knowing that the standalone
 *                wres_get_stick_rel_cur tests the SPRITE's FLIPH bit
 *                instead for the same purpose -- two different
 *                sources for one idea, both in the shipped code.
 */
typedef struct wm_switch_readings {
    uint16_t but_cur;
    uint16_t but_down;
    uint16_t but_up;
    uint16_t stick_cur;
    uint16_t stick_down;
    uint16_t stick_up;
    uint16_t stick_rel_cur;
    uint16_t stick_rel_new;
} wm_switch_readings;

/* The DRN_* six a drone supplies in place of the latch. */
typedef struct wm_drone_switches {
    uint16_t but;
    uint16_t but_down;
    uint16_t but_up;
    uint16_t joy;
    uint16_t joy_down;
    uint16_t joy_up;
} wm_drone_switches;

void wm_read_switches_one(wm_switch_readings *out,
                          const wm_switch_latch *sw,
                          int plyrnum,
                          bool is_drone,
                          const wm_drone_switches *drone,
                          int32_t immobilize_time,
                          bool facing_right);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_SWITCHES_H */
