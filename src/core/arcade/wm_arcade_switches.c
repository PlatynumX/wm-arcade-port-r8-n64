/*
 * The cabinet's switch word: MAIN.ASM's latch, WRESTLE2.ASM's slicing
 * and UTIL.ASM's aggregators.
 */
#include "wm/arcade/wm_arcade_switches.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

#include <string.h>

const uint8_t wm_sw_joy_offs[WM_SW_MAX_PLAYERS] = { 0x00, 0x08, 0x20, 0x28 };
const uint8_t wm_sw_but_offs[WM_SW_MAX_PLAYERS] = { 0x04, 0x0C, 0x24, 0x2C };
/* `but_offs2 .word 20h-3,24h-3` -- three bits low so the 011000b mask
 * lands on latch bits 0x20/0x21 and 0x24/0x25. */
const uint8_t wm_sw_but_offs2[WM_SW_HUMAN_PLAYERS] = { 0x20 - 3, 0x24 - 3 };
const uint8_t wm_sw_start_offs[WM_SW_HUMAN_PLAYERS] = { 0x12, 0x15 };

void wm_switch_latch_init(wm_switch_latch *sw)
{
    if (sw) {
        memset(sw, 0, sizeof(*sw));
    }
}

void wm_switch_latch_update(wm_switch_latch *sw, uint64_t fswitch)
{
    uint64_t old;

    if (!sw) {
        return;
    }
    old = sw->cur;
    sw->old = old;
    sw->cur = fswitch;
    /* `xor a1,a2 / and a1,a2` and then `xor a0,a1 / and a0,a1`. */
    sw->down = (old ^ fswitch) & fswitch;
    sw->up = (fswitch ^ old) & old;
}

/* `move *a0,a0` reads a sixteen-bit field at a bit address; the caller's
 * `andi` then keeps the bits it wants. */
static uint16_t field(uint64_t word, unsigned bit, uint16_t mask)
{
    return (uint16_t)(((word >> bit) & 0xFFFFu) & mask);
}

static uint16_t stick_of(uint64_t word, int player)
{
    if (player < 0 || player >= WM_SW_MAX_PLAYERS) {
        return 0;
    }
    return field(word, wm_sw_joy_offs[player], WM_SW_STICK_MASK);
}

static uint16_t buttons_of(uint64_t word, int player)
{
    uint16_t low;
    uint16_t high;

    if (player < 0 || player >= WM_SW_HUMAN_PLAYERS) {
        return 0;
    }
    high = field(word, wm_sw_but_offs2[player], WM_SW_BUT_HIGH_MASK);
    low = field(word, wm_sw_but_offs[player], WM_SW_BUT_LOW_MASK);
    return (uint16_t)(low | high);
}

uint16_t wm_get_stick_val_cur(const wm_switch_latch *sw, int player)
{
    return sw ? stick_of(sw->cur, player) : 0;
}

uint16_t wm_get_stick_val_down(const wm_switch_latch *sw, int player)
{
    return sw ? stick_of(sw->down, player) : 0;
}

uint16_t wm_get_stick_val_up(const wm_switch_latch *sw, int player)
{
    return sw ? stick_of(sw->up, player) : 0;
}

uint16_t wm_get_but_val_cur(const wm_switch_latch *sw, int player)
{
    return sw ? buttons_of(sw->cur, player) : 0;
}

uint16_t wm_get_but_val_down(const wm_switch_latch *sw, int player)
{
    return sw ? buttons_of(sw->down, player) : 0;
}

uint16_t wm_get_but_val_up(const wm_switch_latch *sw, int player)
{
    return sw ? buttons_of(sw->up, player) : 0;
}

uint16_t wm_get_start_cur(const wm_switch_latch *sw, int player)
{
    if (!sw || player < 0 || player >= WM_SW_HUMAN_PLAYERS) {
        return 0;
    }
    return field(sw->cur, wm_sw_start_offs[player], WM_SW_START_MASK);
}

uint16_t wm_get_start_down(const wm_switch_latch *sw, int player)
{
    if (!sw || player < 0 || player >= WM_SW_HUMAN_PLAYERS) {
        return 0;
    }
    return field(sw->down, wm_sw_start_offs[player], WM_SW_START_MASK);
}

/* ------------------------------------------------------------------ */
/* UTIL.ASM's aggregators                                              */

typedef uint16_t (*read_one_fn)(const wm_switch_latch *, int);

static uint16_t gated(const wm_switch_latch *sw, uint32_t pstatus,
                      read_one_fn read)
{
    uint16_t v = 0;
    /* `btst 0,a2` then `btst 1,a2` -- a player who has not started is
     * simply not asked. */
    if (pstatus & 0x1u) {
        v = (uint16_t)(v | read(sw, 0));
    }
    if (pstatus & 0x2u) {
        v = (uint16_t)(v | read(sw, 1));
    }
    return v;
}

static uint16_t ungated(const wm_switch_latch *sw, read_one_fn read)
{
    return (uint16_t)(read(sw, 0) | read(sw, 1));
}

uint16_t wm_get_all_sticks_cur(const wm_switch_latch *sw, uint32_t pstatus)
{
    return gated(sw, pstatus, wm_get_stick_val_cur);
}

uint16_t wm_get_all_sticks_cur2(const wm_switch_latch *sw)
{
    return ungated(sw, wm_get_stick_val_cur);
}

uint16_t wm_get_all_sticks_down(const wm_switch_latch *sw, uint32_t pstatus)
{
    return gated(sw, pstatus, wm_get_stick_val_down);
}

uint16_t wm_get_all_sticks_down2(const wm_switch_latch *sw)
{
    return ungated(sw, wm_get_stick_val_down);
}

uint16_t wm_get_all_buttons_cur(const wm_switch_latch *sw, uint32_t pstatus)
{
    return gated(sw, pstatus, wm_get_but_val_cur);
}

uint16_t wm_get_all_buttons_cur2(const wm_switch_latch *sw)
{
    return ungated(sw, wm_get_but_val_cur);
}

uint16_t wm_get_all_buttons_down(const wm_switch_latch *sw, uint32_t pstatus)
{
    return gated(sw, pstatus, wm_get_but_val_down);
}

uint16_t wm_get_all_buttons_down2(const wm_switch_latch *sw)
{
    return ungated(sw, wm_get_but_val_down);
}

uint16_t wm_get_all_starts_cur(const wm_switch_latch *sw, uint32_t pstatus)
{
    return gated(sw, pstatus, wm_get_start_cur);
}

uint16_t wm_get_all_starts_down(const wm_switch_latch *sw, uint32_t pstatus)
{
    return gated(sw, pstatus, wm_get_start_down);
}

/* ------------------------------------------------------------------ */

uint16_t wm_stick_flip(uint16_t stick)
{
    /*
     * `#xflip_table` is written out as sixteen .word rows and amounts
     * to swapping bits 2 and 3 -- WM_MOVE_LEFT and WM_MOVE_RIGHT --
     * with up and down left alone. Anything above four bits is not a
     * stick reading and the table could not index it.
     */
    static const uint8_t xflip[16] = {
        0x0, 0x1, 0x2, 0x3, 0x8, 0x9, 0xA, 0xB,
        0x4, 0x5, 0x6, 0x7, 0xC, 0xD, 0xE, 0xF
    };
    return xflip[stick & 0x0Fu];
}

void wm_read_switches_one(wm_switch_readings *out,
                          const wm_switch_latch *sw,
                          int plyrnum,
                          bool is_drone,
                          const wm_drone_switches *drone,
                          int32_t immobilize_time,
                          bool facing_right)
{
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(*out));

    /* `move *a10(IMMOBILIZE_TIME),a14 / jrp #immob` -- strictly
     * positive. Everything is zeroed and the relative derivation below
     * is skipped entirely, so a frozen wrestler's cached stick_rel
     * values go to zero too. */
    if (immobilize_time > 0) {
        return;
    }

    if (is_drone) {
        if (drone) {
            out->but_cur = drone->but;
            out->but_down = drone->but_down;
            out->but_up = drone->but_up;
            out->stick_cur = drone->joy;
            out->stick_down = drone->joy_down;
            out->stick_up = drone->joy_up;
        }
    } else {
        out->but_cur = wm_get_but_val_cur(sw, plyrnum);
        out->but_down = wm_get_but_val_down(sw, plyrnum);
        out->but_up = wm_get_but_val_up(sw, plyrnum);
        out->stick_cur = wm_get_stick_val_cur(sw, plyrnum);
        out->stick_down = wm_get_stick_val_down(sw, plyrnum);
        out->stick_up = wm_get_stick_val_up(sw, plyrnum);
    }

    /* Both derivations run for a drone as well -- `#cont` is below the
     * drone branch, not inside the human one. */
    out->stick_rel_cur = facing_right ? out->stick_cur
                                      : wm_stick_flip(out->stick_cur);
    /* `or a1,a0 / jrz #no_stick` -- with no transition this frame the
     * relative-new value is zero, not the last one. */
    if ((out->stick_up | out->stick_down) != 0u) {
        out->stick_rel_new = out->stick_rel_cur;
    }
}
