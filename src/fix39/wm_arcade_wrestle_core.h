#ifndef WM_ARCADE_WRESTLE_CORE_H
#define WM_ARCADE_WRESTLE_CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#ifndef WM_ARCADE_TSEC
#define WM_ARCADE_TSEC 53u
#endif
typedef struct wm_arcade_auto_pin_env { bool in_finish_move; bool finish_completed; bool royal_rumble; bool anyone_bucking; } wm_arcade_auto_pin_env_t;
void wm_arcade_update_links(wm_arcade_actor_t *a);
void wm_arcade_set_wrestler_xflip(wm_arcade_actor_t *a);
void wm_arcade_count_button_presses(wm_arcade_actor_t *a);
int wm_arcade_auto_pin_check(wm_arcade_actor_t *a, wm_arcade_actor_t *closest, const wm_arcade_auto_pin_env_t *env);
int wm_arcade_can_pin(wm_arcade_actor_t *a, wm_arcade_actor_t **actors, size_t actor_count);
void wm_arcade_hit_nearest_for_pin(wm_arcade_actor_t *a, wm_arcade_actor_t **actors, size_t actor_count);
void wm_arcade_wrestler_countdown_tail(wm_arcade_actor_t *a, bool match_time_zero);
void wm_arcade_reset_wrestle2_state(wm_arcade_actor_t *a);
#endif
