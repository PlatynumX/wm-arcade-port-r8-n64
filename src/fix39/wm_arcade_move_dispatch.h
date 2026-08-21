#ifndef WM_ARCADE_MOVE_DISPATCH_H
#define WM_ARCADE_MOVE_DISPATCH_H

#include "wm_arcade_combat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum wm_arcade_move_handler_id {
    WM_MOVE_HANDLER_BRET = 0,
    WM_MOVE_HANDLER_RAZOR = 1,
    WM_MOVE_HANDLER_TAKER = 2,
    WM_MOVE_HANDLER_YOKO = 3,
    WM_MOVE_HANDLER_SHAWN = 4,
    WM_MOVE_HANDLER_BAM = 5,
    WM_MOVE_HANDLER_DOINK = 6,
    WM_MOVE_HANDLER_SPARE = 7,
    WM_MOVE_HANDLER_LEX = 8
} wm_arcade_move_handler_id_t;

typedef enum wm_arcade_move_dispatch_result {
    WM_MOVE_DISPATCH_HALTED = 0,
    WM_MOVE_DISPATCH_SPECIAL = 1,
    WM_MOVE_DISPATCH_CHARACTER = 2,
    WM_MOVE_DISPATCH_SPARE = 3,
    WM_MOVE_DISPATCH_BAD_WRESTLER = -1
} wm_arcade_move_dispatch_result_t;

typedef struct wm_arcade_move_callbacks {
    void (*change_anim_special)(wm_arcade_actor_t *actor, uintptr_t anim_token, void *user);
    void (*auto_pin_check)(wm_arcade_actor_t *actor, void *user);
    void (*character_move)(wm_arcade_actor_t *actor, wm_arcade_move_handler_id_t which, void *user);
    void *user;
} wm_arcade_move_callbacks_t;

wm_arcade_move_dispatch_result_t wm_arcade_move_wrestler(
    wm_arcade_actor_t *actor,
    int halt,
    const wm_arcade_move_callbacks_t *callbacks);

/* WRESTLE.ASM convert_facing table, values 0..15. */
uint16_t wm_arcade_convert_facing(uint16_t binary_dir);

#ifdef __cplusplus
}
#endif
#endif
