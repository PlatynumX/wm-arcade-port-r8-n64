#ifndef WMANIA_ATTRACT_LIVE_H
#define WMANIA_ATTRACT_LIVE_H

#include "wmania_attract_core.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GAME.EQU source timebase used by ATTRACT.ASM SLEEP expressions. */
#define WM_FIX39_ATTRACT_TSEC 60u

typedef enum {
    WM_ATTRACT_LIVE_IDLE = 0,
    WM_ATTRACT_LIVE_WAIT_EXTERNAL,
    WM_ATTRACT_LIVE_PREWAIT,
    WM_ATTRACT_LIVE_WAIT_BUTTON,
    WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_SETTLE,
    WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_WAIT,
    WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_SETTLE,
    WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_WAIT,
    WM_ATTRACT_LIVE_AAMA_SETUP,
    WM_ATTRACT_LIVE_AAMA_SETTLE,
    WM_ATTRACT_LIVE_AAMA_WAIT,

    /* ATTRACT.ASM::show_time_date.  QUERY is READ_DIP/GetTime, PRESENT is
       GENERIC_DISPLAY plus the source JAM_STR/string construction. */
    WM_ATTRACT_LIVE_TIME_DATE_QUERY,
    WM_ATTRACT_LIVE_TIME_DATE_PREWAIT,
    WM_ATTRACT_LIVE_TIME_DATE_PRESENT,
    WM_ATTRACT_LIVE_TIME_DATE_MIN_DISPLAY,
    WM_ATTRACT_LIVE_TIME_DATE_WAIT,

    /* ATTRACT.ASM::show_operatormsg. QUERY checks CUSTOM_MESSAGE. DAN_SETUP
       covers the blocking dan_test path (2 + 1 + 32 source ticks), PRESENT
       prints the CMOS rows, CLEANUP is scrn_scaleout + WIPEOUT. */
    WM_ATTRACT_LIVE_OPERATOR_QUERY,
    WM_ATTRACT_LIVE_OPERATOR_DAN_SETUP,
    WM_ATTRACT_LIVE_OPERATOR_PRESENT,
    WM_ATTRACT_LIVE_OPERATOR_PREWAIT,
    WM_ATTRACT_LIVE_OPERATOR_WAIT,
    WM_ATTRACT_LIVE_OPERATOR_CLEANUP,

    WM_ATTRACT_LIVE_DONE
} WmAttractLivePhase;

typedef enum {
    /* DCS, Sports and Title already have source-backed frontend tickers. */
    WM_ATTRACT_OWNER_EXISTING_FRONTEND = 0,
    /* V11 has a source timing/state implementation; platform presentation may
       opt in once its exact renderer/data adapter is bound. */
    WM_ATTRACT_OWNER_FIX39_LIVE,
    /* A named source dependency still blocks execution. */
    WM_ATTRACT_OWNER_PENDING_DEPENDENCY
} WmAttractOwner;

typedef struct {
    bool active;
    bool done;
    bool waiting_external;
    bool button_enabled;
    WmAttractScreen screen;
    WmAttractLivePhase phase;
    uint32_t phase_ticks;
    uint32_t total_ticks;
    uint8_t page;
    uint8_t hint_index;
    uint8_t wrestler_index;
} WmAttractLive;

/* Which screens have enough ATTRACT.ASM control-flow/timing translated for a
 * live runner. This does NOT claim the N64 renderer for that screen exists. */
WmAttractOwner wm_attract_live_owner(WmAttractScreen screen);

void wm_attract_live_reset(WmAttractLive *live);
bool wm_attract_live_begin(WmAttractLive *live, const WmAttractStep *step);

/*
 * Some source routines call external/shared services.  `available` means the
 * real source/platform dependency exists and completed.  false is meaningful
 * only for source-optional calls (operator message absent, time/date DIP off)
 * and completes that attract step exactly as the source would.
 */
bool wm_attract_live_signal_external_result(WmAttractLive *live,
                                            bool available);

/* Convenience for mandatory external source effects (OPEN_SCREEN_LINE,
 * BLOW_0_TO_1, GENERIC_DISPLAY setup, scrn_scaleout/WIPEOUT, etc.). */
bool wm_attract_live_signal_external_complete(WmAttractLive *live);

/* Advance one source tick. Returns true when the source routine is complete. */
bool wm_attract_live_tick(WmAttractLive *live, bool any_button);

#ifdef __cplusplus
}
#endif

#endif
