#ifndef WM_ARCADE_GETUP_PROCESS_H
#define WM_ARCADE_GETUP_PROCESS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#include "wm/arcade_sound.h"
#define WM_GETUP_SIZE 80
#define WM_GETUP_DELAY_AFTER_SLIDE (18*60)
#define WM_GETUP_DUFUS_TICKS 120

typedef enum wm_arcade_getup_phase { WM_GETUP_INACTIVE=0, WM_GETUP_INITIAL_SLEEP, WM_GETUP_OFFSCREEN, WM_GETUP_ONSCREEN } wm_arcade_getup_phase;
typedef struct {
    bool active;
    wm_arcade_getup_phase phase;
    uint16_t initial_sleep;
    uint16_t offscreen_counter;
    int32_t initial_getup;
    int32_t display_value;
    int32_t dufus_start_getup;
    uint16_t dufus_countdown;
    bool dufus_message_pending;
    bool drone_meters_on;
    uint8_t num_opps;
    bool royal_rumble;
    wm_arcade_sound *sound;
    wm_arcade_actor_t **actors;
    size_t actor_count;
} wm_arcade_getup_process;
void wm_arcade_getup_process_init(wm_arcade_getup_process *p);
void wm_arcade_getup_process_begin(wm_arcade_getup_process *p,wm_arcade_actor_t *a,wm_arcade_actor_t **actors,size_t actor_count,bool drone_meters_on,uint8_t num_opps,bool royal_rumble,wm_arcade_sound *sound);
/* Compatibility name: source process creation, not an immediate meter display. */
void wm_arcade_getup_process_attach(wm_arcade_getup_process *p,wm_arcade_actor_t *a,wm_arcade_sound *sound);
void wm_arcade_getup_process_tick(wm_arcade_getup_process *p,wm_arcade_actor_t *a);
void wm_arcade_getup_process_ditch(wm_arcade_getup_process *p,wm_arcade_actor_t *a);
void wm_arcade_getup_process_kill(wm_arcade_getup_process *p,wm_arcade_actor_t *a);
void wm_arcade_inc_getup_time(wm_arcade_actor_t *a,int32_t ticks);
#endif
