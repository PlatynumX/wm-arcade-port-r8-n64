#ifndef WM_ATTRACT_H
#define WM_ATTRACT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Active JSRP targets in ATTRACT.ASM::attract_mode plus the conditional tail.
   Names intentionally mirror the original routine labels. */
typedef enum {
    WM_ATTRACT_SHOW_HSTD = 0,
    WM_ATTRACT_DCS_LOGO,
    WM_ATTRACT_SHOW_SPORTS_LOGO,
    WM_ATTRACT_SHOW_GAMEPLAY,
    WM_ATTRACT_CREDITSCREEN,
    WM_ATTRACT_SHOW_TITLE,
    WM_ATTRACT_DO_HINTS,
    WM_ATTRACT_SHOW_GEN_TIPS,
    WM_ATTRACT_SHOW_BIOS,
    WM_ATTRACT_SHOW_BIOS_TIPS,
    WM_ATTRACT_SHOW_OPERATORMSG,
    WM_ATTRACT_SHOW_TIME_DATE,
    WM_ATTRACT_SHOW_COPYRIGHT,
    WM_ATTRACT_AAMA_MESSAGE,
    WM_ATTRACT_CALL_COUNT
} wm_attract_call;

/* Generated directly from the active JSRP calls between #loop and RemapIO. */
extern const wm_attract_call wm_source_attract_loop[];
extern const size_t wm_source_attract_loop_count;

const char *wm_attract_call_name(wm_attract_call call);

/* Port-completeness is deliberately separate from executability.  A PARTIAL
   routine follows original source control/timing but may omit source effects
   that are not translated yet.  HARNESS_ONLY code exists only for bring-up and
   must never be entered by the normal arcade program flow. */
typedef enum {
    WM_PORT_NOT_STARTED = 0,
    WM_PORT_PARTIAL_SOURCE,
    WM_PORT_EXACT_SOURCE,
    WM_PORT_HARNESS_ONLY
} wm_port_status;

wm_port_status wm_attract_call_port_status(wm_attract_call call);
const char *wm_port_status_name(wm_port_status status);

/* DCS_LOGO phases are split at the original SLEEP/button boundaries.  The
   graphics backend may inspect these without owning the timing/state machine. */
typedef enum {
    WM_DCS_STATIC = 0,             /* source object visible: SLEEP 42h */
    WM_DCS_ROT_SETUP,              /* SLEEP 1 before M_NODISP */
    WM_DCS_ROT_UNSKIPPABLE,        /* ADD_PIXEL_ROT, first SLEEP 60 */
    WM_DCS_ROT_SKIPPABLE,          /* 254-60 one-tick button loop */
    WM_DCS_STATIC_RETURN,          /* original object redisplayed, SLEEP 10 */
    WM_DCS_BURST_FLASH,            /* ADD_PIXEL_VEL + three white flashes */
    WM_DCS_BURST_WAIT,             /* SLEEPK 30 */
    WM_DCS_BURST_SKIPPABLE         /* wait_on_butn, 100 ticks */
} wm_dcs_phase;


#endif
