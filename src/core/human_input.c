#include "wm/human_input.h"
#include "wm/arcade/wm_arcade_switches.h"
#include <string.h>

/* Matches wm/core/demo.c's MOVE_DEADZONE, the only other place in this tree
   that already turns wm_input_state's analog stick into discrete moves. */
#define WM_HUMAN_INPUT_DEADZONE 12

void wm_human_input_init(wm_human_input_state *hs) {
    if (!hs) return;
    memset(hs, 0, sizeof(*hs));
}

void wm_human_input_commit(wm_arcade_actor_t *actor,
                           wm_human_input_state *hs,
                           const wm_input_state *input) {
    uint16_t but = 0, joy = 0;
    uint16_t old_but, old_joy, but_x, joy_x;

    if (!actor || !hs) return;
    old_but = hs->but;
    old_joy = hs->joy;

    if (input) {
        if (input->light_punch) but |= WM_BTN_PUNCH;
        if (input->block) but |= WM_BTN_BLOCK;
        if (input->power_punch) but |= WM_BTN_SPUNCH;
        if (input->light_kick) but |= WM_BTN_KICK;
        if (input->power_kick) but |= WM_BTN_SKICK;

        if (input->stick_x > WM_HUMAN_INPUT_DEADZONE) joy |= WM_MOVE_RIGHT;
        else if (input->stick_x < -WM_HUMAN_INPUT_DEADZONE) joy |= WM_MOVE_LEFT;
        if (input->stick_y > WM_HUMAN_INPUT_DEADZONE) joy |= WM_MOVE_UP;
        else if (input->stick_y < -WM_HUMAN_INPUT_DEADZONE) joy |= WM_MOVE_DOWN;
    }

    hs->but = but;
    hs->joy = joy;

    but_x = (uint16_t)(but ^ old_but);
    actor->but_val_cur = but;
    actor->but_val_down = (uint16_t)(but_x & but);
    actor->but_val_up = (uint16_t)(but_x & old_but);

    joy_x = (uint16_t)(joy ^ old_joy);
    actor->stick_val_cur = joy;
    actor->stick_val_down = (uint16_t)(joy_x & joy);
    actor->stick_val_up = (uint16_t)(joy_x & old_joy);

    /*
     * read_switches' last step (WRESTLE2.ASM:2820, `#cont`): the two
     * RELATIVE readings, which every WAITSWITCH_DWN in the game reads
     * instead of the absolute ones. Left and right become away and
     * toward, so a special-move sequence is written once and works
     * from either side of the opponent.
     *
     * `facing_right` is the FACING_DIR PLAYER_RIGHT_BIT test, which is
     * WM_MOVE_RIGHT. And STICK_REL_NEW is gated on the stick having
     * actually moved this tick -- `or a1,a0 / jrz #no_stick` over the
     * up and down transitions -- so a held direction reads once, not
     * every tick. Without that gate a stick rotation would complete on
     * its first press.
     */
    actor->stick_rel_cur = (actor->facing_dir & WM_MOVE_RIGHT)
                         ? joy : wm_stick_flip(joy);
    actor->stick_rel_new =
        (actor->stick_val_up | actor->stick_val_down)
            ? actor->stick_rel_cur : 0u;
}
