#include "wm_arcade_move_dispatch.h"

#include <stddef.h>

wm_arcade_move_dispatch_result_t wm_arcade_move_wrestler(
    wm_arcade_actor_t *a,
    int halt,
    const wm_arcade_move_callbacks_t *cb)
{
    if (!a) return WM_MOVE_DISPATCH_BAD_WRESTLER;
    if (halt) return WM_MOVE_DISPATCH_HALTED;

    /* WRESTLE.ASM: queued SPECIAL_MOVE_ADDR wins before auto-pin and move_xxx. */
    if (a->special_move_addr != (uintptr_t)0) {
        uintptr_t tok = a->special_move_addr;
        if (cb && cb->change_anim_special) cb->change_anim_special(a, tok, cb->user);
        a->special_move_addr = (uintptr_t)0;
        return WM_MOVE_DISPATCH_SPECIAL;
    }

    if (cb && cb->auto_pin_check) cb->auto_pin_check(a, cb->user);

    if (a->wrestler_num < 0 || a->wrestler_num > 8)
        return WM_MOVE_DISPATCH_BAD_WRESTLER;
    if (a->wrestler_num == 7)
        return WM_MOVE_DISPATCH_SPARE;

    if (cb && cb->character_move)
        cb->character_move(a, (wm_arcade_move_handler_id_t)a->wrestler_num, cb->user);
    return WM_MOVE_DISPATCH_CHARACTER;
}

uint16_t wm_arcade_convert_facing(uint16_t d)
{
    static const uint16_t table[16] = {
        0,0,4,0,6,7,5,0,2,1,3,0,0,0,0,0
    };
    return table[d & 0x0fu];
}
