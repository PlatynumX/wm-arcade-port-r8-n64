#ifndef WM_ARCADE_GETUP_PROCESS_H
#define WM_ARCADE_GETUP_PROCESS_H
#include <stdbool.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#include "wm/arcade_sound.h"
#define WM_GETUP_SIZE 80
#define WM_GETUP_DELAY_AFTER_SLIDE (18*60)
typedef struct { bool active; int32_t initial_getup; int32_t display_value; } wm_arcade_getup_process;
void wm_arcade_getup_process_init(wm_arcade_getup_process *p);
void wm_arcade_getup_process_attach(wm_arcade_getup_process *p, wm_arcade_actor_t *a, wm_arcade_sound *sound);
void wm_arcade_getup_process_tick(wm_arcade_getup_process *p, wm_arcade_actor_t *a);
void wm_arcade_getup_process_ditch(wm_arcade_getup_process *p, wm_arcade_actor_t *a);
#endif
