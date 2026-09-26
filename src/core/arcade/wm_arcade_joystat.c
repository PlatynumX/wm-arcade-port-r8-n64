#include "wm/arcade/wm_arcade_joystat.h"
#include <string.h>

void wm_arcade_joystat_init(wm_arcade_joystat_t *js) {
    if (!js) return;
    memset(js, 0, sizeof(*js));
}

static void joystat_insert(wm_arcade_joystat_t *js, uint16_t value, uint16_t round_tickcount) {
    int i;
    for (i = WM_JOYSTAT_DEPTH - 1; i > 0; --i) {
        js->entries[i] = js->entries[i - 1];
    }
    js->entries[0].value = value;
    js->entries[0].tickcount = round_tickcount;
}

/* WRESTLE.ASM:4650 #xflip_table -- swaps only the LEFT/RIGHT bits of a raw
   0-15 stick value, preserving UP/DOWN, so index i's bit layout is
   [bit0=UP,bit1=DOWN,bit2=LEFT,bit3=RIGHT] exactly like WM_MOVE_*. */
static const uint16_t wm_joystat_xflip_table[16] = {
    0,               WM_J_UP,         WM_J_DOWN,       0,
    WM_J_TOWARD,     WM_J_UP_TOWARD,  WM_J_DOWN_TOWARD, 0,
    WM_J_AWAY,       WM_J_UP_AWAY,    WM_J_DOWN_AWAY,  0,
    0, 0, 0, 0
};

void wm_arcade_joystat_update(wm_arcade_joystat_t *js,
                              const wm_arcade_actor_t *actor,
                              uint16_t round_tickcount) {
    uint16_t abs_lr, rel_dir, combined;
    unsigned btn;

    if (!js || !actor) return;

    abs_lr = (uint16_t)((actor->stick_val_cur & (WM_MOVE_LEFT | WM_MOVE_RIGHT)) << 8);
    rel_dir = (actor->facing_dir & WM_MOVE_LEFT)
        ? wm_joystat_xflip_table[actor->stick_val_cur & 0x0Fu]
        : (uint16_t)(actor->stick_val_cur & 0x0Fu);
    combined = (uint16_t)(abs_lr | rel_dir);

    /* WRESTLE.ASM:4587-4599: a fresh UP/DOWN stick edge, with some
       direction currently held, inserts one entry. */
    if ((actor->stick_val_up | actor->stick_val_down) != 0 && combined != 0) {
        joystat_insert(js, combined, round_tickcount);
    }

    /* WRESTLE.ASM:4601-4615: one entry per newly-pressed button this tick,
       each tagged with its own button bit plus the same directional
       value, regardless of whether that direction is "fresh". */
    for (btn = 0; btn < 5; ++btn) {
        uint16_t but_bit = (uint16_t)(1u << btn);
        if (actor->but_val_down & but_bit) {
            joystat_insert(js, (uint16_t)((but_bit << 4) | combined), round_tickcount);
        }
    }
}

static uint16_t joystat_elapsed(uint16_t now, uint16_t then) {
    return (uint16_t)(now - then);
}

bool wm_arcade_joystat_matches(const wm_arcade_joystat_t *js,
                               uint16_t round_tickcount,
                               const wm_arcade_bret_sequence_step_t *steps,
                               uint16_t step_count,
                               uint16_t max_ticks) {
    unsigned idx;
    int skip_budget = 8;
    uint16_t oldest_ts;
    unsigned s;

    if (!js || !steps || step_count == 0) return false;

    /* WRESTLE.ASM's own "special check" on the first (trigger) entry:
       must be the queue's current head exactly, no skip tolerance. */
    if ((uint16_t)(js->entries[0].value & (uint16_t)~steps[0].ignore_mask) != steps[0].value) {
        return false;
    }
    oldest_ts = js->entries[0].tickcount;
    idx = 1;

    for (s = 1; s < step_count; ++s) {
        uint16_t mask = steps[s].ignore_mask;
        uint16_t want = steps[s].value;

        while (idx < WM_JOYSTAT_DEPTH &&
               (uint16_t)(js->entries[idx].value & (uint16_t)~mask) == 0 &&
               skip_budget > 0) {
            --skip_budget;
            ++idx;
        }
        if (idx >= WM_JOYSTAT_DEPTH) return false;
        if ((uint16_t)(js->entries[idx].value & (uint16_t)~mask) != want) return false;

        oldest_ts = js->entries[idx].tickcount;
        ++idx;
    }

    return joystat_elapsed(round_tickcount, oldest_ts) <= max_ticks;
}
