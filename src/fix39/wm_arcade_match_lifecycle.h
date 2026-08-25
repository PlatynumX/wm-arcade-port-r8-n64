#ifndef WM_ARCADE_MATCH_LIFECYCLE_H
#define WM_ARCADE_MATCH_LIFECYCLE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "wm_arcade_combat.h"
#ifndef WM_ARCADE_TSEC
#define WM_ARCADE_TSEC 53u
#endif
typedef enum wm_arcade_match_event {
    WM_MATCH_EVENT_NONE=0,
    WM_MATCH_EVENT_TIMER_NORMAL=1,
    WM_MATCH_EVENT_TIMER_RED=2,
    WM_MATCH_EVENT_TIMER_WARNING=3,
    WM_MATCH_EVENT_CLEAR_REDUCE_BOG=4,
    WM_MATCH_EVENT_WAKE_CROWD=5,
    WM_MATCH_EVENT_PIN_PROMPT=6,
    WM_MATCH_EVENT_ANNOUNCE_ROUND_WINNER=7
} wm_arcade_match_event_t;
typedef void (*wm_arcade_match_event_fn)(wm_arcade_match_event_t event, int dead_team, void *user);
typedef struct wm_arcade_match_lifecycle_config {
    uint8_t adjust_speed; /* ADJSPEED 1..5; BADCHK default is 3. */
    uint8_t pstatus;
    uint8_t num_opps;
    bool royal_rumble;
    bool final_match;
    bool eight_on_one;
    int32_t wrestler_count;
    wm_arcade_match_event_fn event;
    void *user;
} wm_arcade_match_lifecycle_config;
typedef struct {
    uint8_t tens,ones;
    uint16_t fraction,step,startup_delay,pin_timeout;
    uint32_t last_dead_pcnt;
    bool halt,attract_wrap,round_award_pending;
    wm_arcade_match_lifecycle_config config;
} wm_arcade_match_lifecycle;
void wm_arcade_match_lifecycle_init(wm_arcade_match_lifecycle *m,bool attract_wrap);
void wm_arcade_match_lifecycle_configure(wm_arcade_match_lifecycle *m,const wm_arcade_match_lifecycle_config *config);
void wm_arcade_match_lifecycle_tick(wm_arcade_match_lifecycle *m,wm_arcade_actor_t **actors,size_t actor_count,uint32_t pcnt);
bool wm_arcade_match_clock_zero(const wm_arcade_match_lifecycle *m);
unsigned wm_arcade_get_live_bits(wm_arcade_actor_t **actors,size_t actor_count);
uint16_t wm_arcade_match_timer_step(const wm_arcade_match_lifecycle_config *config);
#endif
