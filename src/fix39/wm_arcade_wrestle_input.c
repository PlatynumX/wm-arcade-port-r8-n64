#include "wm_arcade_wrestle_input.h"

#include <string.h>

static uint16_t xflip_relative(uint16_t joy)
{
    static const uint16_t xflip_table[16] = {
        0,
        WM_J_UP,
        WM_J_DOWN,
        0,
        WM_J_TOWARD,
        WM_J_UP_TOWARD,
        WM_J_DOWN_TOWARD,
        0,
        WM_J_AWAY,
        WM_J_UP_AWAY,
        WM_J_DOWN_AWAY,
        0, 0, 0, 0, 0
    };
    return xflip_table[joy & 0x0fu];
}

static void history_insert(wm_arcade_actor_t *actor,
                           uint16_t input,
                           uint16_t round_tick)
{
    size_t i;
    uint32_t packed;
    if (!actor) return;

    packed = ((uint32_t)round_tick << 16) | (uint32_t)input;
    for (i = 15u; i > 0u; --i)
        actor->wrest_joystat[i] = actor->wrest_joystat[i - 1u];
    actor->wrest_joystat[0] = packed;
}

void wm_arcade_wrestle_input_init(wm_arcade_actor_t *actor)
{
    if (!actor) return;
    memset(actor->wrest_joystat, 0, sizeof(actor->wrest_joystat));
    memset(actor->joy_dtime, 0, sizeof(actor->joy_dtime));
    actor->stick_rel_cur = 0u;
    actor->stick_rel_new = 0u;
    actor->auto_pin_countdown = 0u;
}

void wm_arcade_update_joystat(wm_arcade_actor_t *actor,
                              uint16_t round_tick,
                              bool halt)
{
    uint16_t joy;
    uint16_t rel;
    uint16_t real_lr;
    uint16_t packed_stick;
    uint16_t transitions;
    uint16_t down;
    uint16_t bit;
    unsigned i;

    if (!actor || halt) return;

    joy = (uint16_t)(actor->stick_val_cur & 0x0fu);
    real_lr = (uint16_t)((joy & (WM_MOVE_LEFT | WM_MOVE_RIGHT)) << 8);
    rel = joy;
    if ((actor->facing_dir & WM_MOVE_LEFT) != 0)
        rel = xflip_relative(joy);

    actor->stick_rel_cur = rel;
    transitions = (uint16_t)((actor->stick_val_up |
                              actor->stick_val_down) & 0x0fu);
    actor->stick_rel_new = transitions ? rel : 0u;
    packed_stick = (uint16_t)(real_lr | rel);

    if (transitions != 0u && packed_stick != 0u)
        history_insert(actor, packed_stick, round_tick);

    down = (uint16_t)(actor->but_val_down & WM_BTN_ATTACK_MASK);
    bit = WM_BTN_PUNCH;
    for (i = 0u; i < 5u; ++i, bit = (uint16_t)(bit << 1)) {
        if ((down & bit) != 0u) {
            uint16_t entry = (uint16_t)((bit << 4) | packed_stick);
            history_insert(actor, entry, round_tick);
        }
    }
}

static uint16_t held_next(uint16_t old, bool held)
{
    return held ? (uint16_t)(old + 1u) : 0u;
}

void wm_arcade_update_joy_dtime(wm_arcade_actor_t *actor)
{
    uint16_t b;
    uint16_t j;
    if (!actor) return;

    b = actor->but_val_cur;
    j = actor->stick_val_cur;

    actor->joy_dtime[WM_DTIME_PUNCH] =
        held_next(actor->joy_dtime[WM_DTIME_PUNCH], (b & WM_BTN_PUNCH) != 0u);
    actor->joy_dtime[WM_DTIME_BLOCK] =
        held_next(actor->joy_dtime[WM_DTIME_BLOCK], (b & WM_BTN_BLOCK) != 0u);
    actor->joy_dtime[WM_DTIME_SPUNCH] =
        held_next(actor->joy_dtime[WM_DTIME_SPUNCH], (b & WM_BTN_SPUNCH) != 0u);
    actor->joy_dtime[WM_DTIME_KICK] =
        held_next(actor->joy_dtime[WM_DTIME_KICK], (b & WM_BTN_KICK) != 0u);
    actor->joy_dtime[WM_DTIME_SKICK] =
        held_next(actor->joy_dtime[WM_DTIME_SKICK], (b & WM_BTN_SKICK) != 0u);

    actor->joy_dtime[WM_DTIME_UP] =
        held_next(actor->joy_dtime[WM_DTIME_UP], (j & WM_MOVE_UP) != 0u);
    actor->joy_dtime[WM_DTIME_DOWN] =
        held_next(actor->joy_dtime[WM_DTIME_DOWN], (j & WM_MOVE_DOWN) != 0u);
    actor->joy_dtime[WM_DTIME_LEFT] =
        held_next(actor->joy_dtime[WM_DTIME_LEFT], (j & WM_MOVE_LEFT) != 0u);
    actor->joy_dtime[WM_DTIME_RIGHT] =
        held_next(actor->joy_dtime[WM_DTIME_RIGHT], (j & WM_MOVE_RIGHT) != 0u);
}

uint16_t wm_arcade_get_joy_dtime(const wm_arcade_actor_t *actor,
                                 wm_arcade_joy_dtime_index_t which)
{
    if (!actor || (unsigned)which >= WM_DTIME_COUNT) return 0u;
    return actor->joy_dtime[(unsigned)which];
}

uint16_t wm_arcade_get_button_dtime(const wm_arcade_actor_t *actor,
                                    uint16_t source_button_bit)
{
    if (!actor) return 0u;
    switch (source_button_bit) {
    case WM_BTN_PUNCH:
        return actor->joy_dtime[WM_DTIME_PUNCH];
    case WM_BTN_BLOCK:
        return actor->joy_dtime[WM_DTIME_BLOCK];
    case WM_BTN_SPUNCH:
        return actor->joy_dtime[WM_DTIME_SPUNCH];
    case WM_BTN_KICK:
        return actor->joy_dtime[WM_DTIME_KICK];
    case WM_BTN_SKICK:
        return actor->joy_dtime[WM_DTIME_SKICK];
    default:
        return 0u;
    }
}

bool wm_arcade_wrestle_pattern_match(
    const wm_arcade_actor_t *actor,
    const wm_arcade_input_step_t *steps,
    size_t step_count,
    uint16_t max_ticks,
    uint16_t round_tick)
{
    size_t si;
    size_t qi = 0u;
    unsigned masked_skips_left = 8u;
    uint16_t last_tick = 0u;
    uint16_t first_value;

    if (!actor || !steps || step_count == 0u) return false;

    if ((uint16_t)(actor->wrest_joystat[0] >> 16) != round_tick)
        return false;

    first_value = (uint16_t)actor->wrest_joystat[0];
    first_value = (uint16_t)(first_value &
                             (uint16_t)~steps[0].ignore_mask);
    if (first_value == 0u)
        return false;

    for (si = 0u; si < step_count; ++si) {
        uint16_t value = 0u;
        bool got = false;

        while (qi < 16u) {
            uint32_t entry = actor->wrest_joystat[qi++];
            last_tick = (uint16_t)(entry >> 16);
            value = (uint16_t)entry;
            value = (uint16_t)(value &
                               (uint16_t)~steps[si].ignore_mask);

            if (value == 0u && masked_skips_left != 0u) {
                --masked_skips_left;
                continue;
            }
            got = true;
            break;
        }

        if (!got || value != steps[si].value)
            return false;
    }

    return (uint16_t)(round_tick - last_tick) <= max_ticks;
}
