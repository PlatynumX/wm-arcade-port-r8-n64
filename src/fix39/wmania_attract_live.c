#include "wmania_attract_live.h"

#include "wmania_attract_data.h"
#include "wmania_attract_time.h"
#include "wmania_hiscore_present.h"

#include <string.h>

static bool phase_waits_external(WmAttractLivePhase phase)
{
    switch (phase) {
        case WM_ATTRACT_LIVE_WAIT_EXTERNAL:
        case WM_ATTRACT_LIVE_TIME_DATE_QUERY:
        case WM_ATTRACT_LIVE_TIME_DATE_PRESENT:
        case WM_ATTRACT_LIVE_OPERATOR_QUERY:
        case WM_ATTRACT_LIVE_OPERATOR_PRESENT:
        case WM_ATTRACT_LIVE_OPERATOR_CLEANUP:
            return true;
        default:
            return false;
    }
}

static void enter_phase(WmAttractLive *live,
                        WmAttractLivePhase phase,
                        uint32_t ticks,
                        bool button_enabled)
{
    live->phase = phase;
    live->phase_ticks = ticks;
    live->button_enabled = button_enabled;
    live->waiting_external = phase_waits_external(phase);
}

WmAttractOwner wm_attract_live_owner(WmAttractScreen screen)
{
    switch (screen) {
        case WM_FIX39_ATTRACT_DCS_LOGO:
        case WM_FIX39_ATTRACT_SPORTS_LOGO:
        case WM_FIX39_ATTRACT_TITLE:
            return WM_ATTRACT_OWNER_EXISTING_FRONTEND;

        case WM_FIX39_ATTRACT_DESIGNER_HINT:
        case WM_FIX39_ATTRACT_GENERAL_TIPS:
        case WM_FIX39_ATTRACT_OPERATOR_MESSAGE:
        case WM_FIX39_ATTRACT_TIME_DATE:
        case WM_FIX39_ATTRACT_COPYRIGHT:
        case WM_FIX39_ATTRACT_AAMA:
        case WM_FIX39_ATTRACT_HISCORES:
            return WM_ATTRACT_OWNER_FIX39_LIVE;

        /* These still have a specific source/platform dependency in V12g. */
        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1:
        case WM_FIX39_ATTRACT_CREDITS_1:
        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_2:
        case WM_FIX39_ATTRACT_CREDITS_2:
        case WM_FIX39_ATTRACT_BIO:
        case WM_FIX39_ATTRACT_BIO_TIPS:
        case WM_FIX39_ATTRACT_EVEN_LOOP_CREDITS:
            return WM_ATTRACT_OWNER_PENDING_DEPENDENCY;
    }
    return WM_ATTRACT_OWNER_PENDING_DEPENDENCY;
}

void wm_attract_live_reset(WmAttractLive *live)
{
    if (live) memset(live, 0, sizeof(*live));
}

bool wm_attract_live_begin(WmAttractLive *live, const WmAttractStep *step)
{
    if (!live || !step || wm_attract_live_owner(step->screen) != WM_ATTRACT_OWNER_FIX39_LIVE)
        return false;

    wm_attract_live_reset(live);
    live->active = true;
    live->screen = step->screen;
    live->hint_index = step->hint_index;
    live->wrestler_index = step->wrestler_index;

    switch (step->screen) {
        case WM_FIX39_ATTRACT_HISCORES:
            /* ATTRACT.ASM::show_hstd begins with the Intercontinental table.
               The renderer owns the source backdrop/table objects; this live
               state owns page order, scrolling cadence and button skips. */
            live->page = (uint8_t)WM_HS_PRESENT_INTER;
            live->hiscore_start_rank = 1u;
            live->hiscore_rows_since_hold = 0u;
            live->hiscore_scroll_px = 0u;
            live->hiscore_scroll_duration = 0u;
            enter_phase(live, WM_ATTRACT_LIVE_HSTD_TRANSITION,
                        (WM_FIX39_ATTRACT_TSEC * WM_HS_PRESENT_TRANSITION_TSEC_NUM) /
                        WM_HS_PRESENT_TRANSITION_TSEC_DEN, false);
            break;

        case WM_FIX39_ATTRACT_DESIGNER_HINT:
            /* OPEN_SCREEN_LINE is an external source effect. */
            enter_phase(live, WM_ATTRACT_LIVE_WAIT_EXTERNAL, 0u, false);
            break;

        case WM_FIX39_ATTRACT_GENERAL_TIPS:
            /* SET_UP_PIXEL_WIPE/print/SLEEPK1/BLOW_0_TO_1/RESET_FROM_PIXEL_WIPE
               is an external source presentation operation. */
            enter_phase(live, WM_ATTRACT_LIVE_WAIT_EXTERNAL, 0u, false);
            break;

        case WM_FIX39_ATTRACT_OPERATOR_MESSAGE:
            /* Source first scans byte zero of each CUSTOM_MESSAGE row. */
            enter_phase(live, WM_ATTRACT_LIVE_OPERATOR_QUERY, 0u, false);
            break;

        case WM_FIX39_ATTRACT_TIME_DATE:
            /* Source first READ_DIP checks DPTDON_B, then enables clock auto
               update and calls _GetTime before its 30-tick delay. */
            enter_phase(live, WM_ATTRACT_LIVE_TIME_DATE_QUERY, 0u, false);
            break;

        case WM_FIX39_ATTRACT_COPYRIGHT:
            live->page = 1u;
            enter_phase(live, WM_ATTRACT_LIVE_PREWAIT,
                        WM_FIX39_ATTRACT_COPYRIGHT_INITIAL_SLEEP_TICKS, false);
            break;

        case WM_FIX39_ATTRACT_AAMA:
            /* SLEEPK2 + do_the_grad_thang(SLEEPK2 ... SLEEPK4). */
            enter_phase(live, WM_ATTRACT_LIVE_AAMA_SETUP,
                        WM_FIX39_ATTRACT_AAMA_INITIAL_SLEEP_TICKS +
                        WM_FIX39_ATTRACT_AAMA_GRADIENT_PREWAIT_TICKS +
                        WM_FIX39_ATTRACT_AAMA_GRADIENT_POSTWAIT_TICKS,
                        false);
            break;

        default:
            wm_attract_live_reset(live);
            return false;
    }
    return true;
}

static void finish(WmAttractLive *live)
{
    live->done = true;
    live->active = false;
    live->button_enabled = false;
    live->waiting_external = false;
    live->phase = WM_ATTRACT_LIVE_DONE;
}


static bool hiscore_page_is_scrolling(const WmAttractLive *live)
{
    return live && (live->page == (uint8_t)WM_HS_PRESENT_INTER ||
                    live->page == (uint8_t)WM_HS_PRESENT_BEATEN);
}

static void hiscore_begin_scroll(WmAttractLive *live, uint8_t duration)
{
    live->hiscore_scroll_px = 0u;
    live->hiscore_scroll_duration = duration;
    enter_phase(live, WM_ATTRACT_LIVE_HSTD_SCROLL, duration, true);
}

static bool hiscore_advance_page(WmAttractLive *live)
{
    if (!live) return true;
    if (live->page + 1u >= (uint8_t)WM_HS_PRESENT_COUNT) {
        finish(live);
        return true;
    }
    ++live->page;
    live->hiscore_start_rank = 1u;
    live->hiscore_rows_since_hold = 0u;
    live->hiscore_scroll_px = 0u;
    live->hiscore_scroll_duration = 0u;
    enter_phase(live, WM_ATTRACT_LIVE_HSTD_TRANSITION,
                (WM_FIX39_ATTRACT_TSEC * WM_HS_PRESENT_TRANSITION_TSEC_NUM) /
                WM_HS_PRESENT_TRANSITION_TSEC_DEN, false);
    return false;
}

bool wm_attract_live_signal_external_result(WmAttractLive *live,
                                            bool available)
{
    if (!live || !live->active || !live->waiting_external)
        return false;

    switch (live->phase) {
        case WM_ATTRACT_LIVE_WAIT_EXTERNAL:
            if (!available) return false; /* Mandatory source effect. */
            if (live->screen == WM_FIX39_ATTRACT_DESIGNER_HINT) {
                enter_phase(live, WM_ATTRACT_LIVE_PREWAIT,
                            WM_ATTRACT_HINT_PREWAIT_TICKS, false);
                return true;
            }
            if (live->screen == WM_FIX39_ATTRACT_GENERAL_TIPS) {
                /* BLOW_0_TO_1/RESET completed; source then SLEEP TSEC. */
                enter_phase(live, WM_ATTRACT_LIVE_PREWAIT,
                            WM_FIX39_ATTRACT_GENERAL_TIPS_PREWAIT_TSEC *
                            WM_FIX39_ATTRACT_TSEC, false);
                return true;
            }
            return false;

        case WM_ATTRACT_LIVE_TIME_DATE_QUERY:
            if (!available) {
                /* READ_DIP/DPTDON_B clear -> #std_exit. */
                finish(live);
                return true;
            }
            enter_phase(live, WM_ATTRACT_LIVE_TIME_DATE_PREWAIT,
                        WM_FIX39_ATTRACT_TIME_DATE_MIN_PRE_TICKS, false);
            return true;

        case WM_ATTRACT_LIVE_TIME_DATE_PRESENT:
            if (!available) return false;
            /* GENERIC_DISPLAY and all six source text rows are now up. */
            enter_phase(live, WM_ATTRACT_LIVE_TIME_DATE_MIN_DISPLAY,
                        WM_FIX39_ATTRACT_TIME_DATE_MIN_DISPLAY_TICKS, false);
            return true;

        case WM_ATTRACT_LIVE_OPERATOR_QUERY:
            if (!available) {
                /* No CUSTOM_MESSAGE row begins nonzero -> #x/RETP. */
                finish(live);
                return true;
            }
            /* dan_test blocks show_operatormsg for 2 + 1 + 32 ticks. */
            enter_phase(live, WM_ATTRACT_LIVE_OPERATOR_DAN_SETUP,
                        WM_FIX39_ATTRACT_OPERATOR_DAN_SETUP_TICKS, false);
            return true;

        case WM_ATTRACT_LIVE_OPERATOR_PRESENT:
            if (!available) return false;
            enter_phase(live, WM_ATTRACT_LIVE_OPERATOR_PREWAIT,
                        WM_ATTRACT_OPERATOR_PREWAIT_TICKS, false);
            return true;

        case WM_ATTRACT_LIVE_OPERATOR_CLEANUP:
            if (!available) return false;
            finish(live);
            return true;

        default:
            return false;
    }
}

bool wm_attract_live_signal_external_complete(WmAttractLive *live)
{
    return wm_attract_live_signal_external_result(live, true);
}

static bool consume_ticks(WmAttractLive *live)
{
    if (live->phase_ticks > 0u) {
        --live->phase_ticks;
        ++live->total_ticks;
        if (live->phase == WM_ATTRACT_LIVE_HSTD_SCROLL &&
            live->hiscore_scroll_duration != 0u) {
            uint32_t elapsed = (uint32_t)live->hiscore_scroll_duration -
                               live->phase_ticks;
            uint32_t px = (elapsed * 70u) / live->hiscore_scroll_duration;
            if (px > 70u) px = 70u;
            live->hiscore_scroll_px = (uint8_t)px;
        }
    }
    return live->phase_ticks == 0u;
}

bool wm_attract_live_tick(WmAttractLive *live, bool any_button)
{
    if (!live) return true;
    if (live->done) return true;
    if (!live->active) return false;
    if (live->waiting_external) return false;

    /* wait_on_butn exits immediately once the button-reading phase is live. */
    if (live->button_enabled && any_button) {
        if (live->screen == WM_FIX39_ATTRACT_HISCORES) {
            /* show_hstd's wait loops skip only the current table; execution
               continues to the next of the five source pages. */
            return hiscore_advance_page(live);
        }
        if (live->phase == WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_WAIT) {
            live->page = 2u;
            enter_phase(live, WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_SETTLE,
                        WM_FIX39_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS, false);
            return false;
        }
        if (live->phase == WM_ATTRACT_LIVE_OPERATOR_WAIT) {
            /* Source still runs scrn_scaleout then WIPEOUT after wait_on_butn. */
            enter_phase(live, WM_ATTRACT_LIVE_OPERATOR_CLEANUP, 0u, false);
            return false;
        }
        finish(live);
        return true;
    }

    if (!consume_ticks(live)) return false;

    switch (live->phase) {
        case WM_ATTRACT_LIVE_HSTD_TRANSITION:
            if (hiscore_page_is_scrolling(live)) {
                hiscore_begin_scroll(live, WM_HS_PRESENT_SCROLL_STEP_TICKS_A);
            } else {
                enter_phase(live, WM_ATTRACT_LIVE_HSTD_STATIC_HOLD,
                            WM_HS_PRESENT_FINAL_HOLD_TSEC * WM_FIX39_ATTRACT_TSEC,
                            true);
            }
            break;

        case WM_ATTRACT_LIVE_HSTD_SCROLL:
            live->hiscore_scroll_px = 0u;
            if (live->hiscore_start_rank < 28u)
                ++live->hiscore_start_rank;
            ++live->hiscore_rows_since_hold;
            if (live->hiscore_start_rank >= 28u) {
                enter_phase(live, WM_ATTRACT_LIVE_HSTD_FINAL_HOLD,
                            WM_HS_PRESENT_FINAL_HOLD_TSEC * WM_FIX39_ATTRACT_TSEC,
                            true);
            } else if (live->hiscore_rows_since_hold >= 3u) {
                live->hiscore_rows_since_hold = 0u;
                enter_phase(live, WM_ATTRACT_LIVE_HSTD_GROUP_HOLD,
                            WM_HS_PRESENT_SCROLL_LAST_OFF_TICKS +
                            WM_HS_PRESENT_SCROLL_GROUP_HOLD_TICKS, true);
            } else {
                hiscore_begin_scroll(live, WM_HS_PRESENT_SCROLL_STEP_TICKS_B);
            }
            break;

        case WM_ATTRACT_LIVE_HSTD_GROUP_HOLD:
            hiscore_begin_scroll(live, WM_HS_PRESENT_SCROLL_STEP_TICKS_B);
            break;

        case WM_ATTRACT_LIVE_HSTD_FINAL_HOLD:
        case WM_ATTRACT_LIVE_HSTD_STATIC_HOLD:
            return hiscore_advance_page(live);

        case WM_ATTRACT_LIVE_PREWAIT:
            if (live->screen == WM_FIX39_ATTRACT_DESIGNER_HINT) {
                enter_phase(live, WM_ATTRACT_LIVE_WAIT_BUTTON,
                            WM_ATTRACT_HINT_WAIT_TSEC * WM_FIX39_ATTRACT_TSEC,
                            true);
            } else if (live->screen == WM_FIX39_ATTRACT_GENERAL_TIPS) {
                enter_phase(live, WM_ATTRACT_LIVE_WAIT_BUTTON,
                            WM_FIX39_ATTRACT_GENERAL_TIPS_WAIT_TSEC *
                            WM_FIX39_ATTRACT_TSEC,
                            true);
            } else if (live->screen == WM_FIX39_ATTRACT_COPYRIGHT) {
                /* Page 1 is drawn after SLEEPK2. fade_up starts, then source
                   waits SLEEPK2 and SLEEPK20 before wait_on_butn. */
                enter_phase(live, WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_SETTLE,
                            2u + WM_FIX39_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS,
                            false);
            }
            break;

        case WM_ATTRACT_LIVE_WAIT_BUTTON:
            finish(live);
            return true;

        case WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_SETTLE:
            enter_phase(live, WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_WAIT,
                        WM_FIX39_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC *
                        WM_FIX39_ATTRACT_TSEC, true);
            break;

        case WM_ATTRACT_LIVE_COPYRIGHT_PAGE1_WAIT:
            live->page = 2u;
            enter_phase(live, WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_SETTLE,
                        WM_FIX39_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS, false);
            break;

        case WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_SETTLE:
            enter_phase(live, WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_WAIT,
                        WM_FIX39_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC *
                        WM_FIX39_ATTRACT_TSEC, true);
            break;

        case WM_ATTRACT_LIVE_COPYRIGHT_PAGE2_WAIT:
            finish(live);
            return true;

        case WM_ATTRACT_LIVE_AAMA_SETUP:
            /* Text/fade setup, SLEEPK2, unblank, then guaranteed 20 ticks. */
            enter_phase(live, WM_ATTRACT_LIVE_AAMA_SETTLE,
                        WM_FIX39_ATTRACT_AAMA_POST_FADE_SLEEP_TICKS +
                        WM_FIX39_ATTRACT_AAMA_FADE_SETTLE_TICKS,
                        false);
            break;

        case WM_ATTRACT_LIVE_AAMA_SETTLE:
            enter_phase(live, WM_ATTRACT_LIVE_AAMA_WAIT,
                        WM_FIX39_ATTRACT_AAMA_WAIT_TSEC * WM_FIX39_ATTRACT_TSEC,
                        true);
            break;

        case WM_ATTRACT_LIVE_AAMA_WAIT:
            finish(live);
            return true;

        case WM_ATTRACT_LIVE_TIME_DATE_PREWAIT:
            /* _GetTime snapshot has aged 30 ticks; now GENERIC_DISPLAY and
               source text construction execute as one external presentation. */
            enter_phase(live, WM_ATTRACT_LIVE_TIME_DATE_PRESENT, 0u, false);
            break;

        case WM_ATTRACT_LIVE_TIME_DATE_MIN_DISPLAY:
            enter_phase(live, WM_ATTRACT_LIVE_TIME_DATE_WAIT,
                        WM_FIX39_ATTRACT_TIME_DATE_WAIT_TSEC *
                        WM_FIX39_ATTRACT_TSEC, true);
            break;

        case WM_ATTRACT_LIVE_TIME_DATE_WAIT:
            finish(live);
            return true;

        case WM_ATTRACT_LIVE_OPERATOR_DAN_SETUP:
            /* dan_test has returned; show_operatormsg now builds/prints each
               source CMOS row using osgfont_t. */
            enter_phase(live, WM_ATTRACT_LIVE_OPERATOR_PRESENT, 0u, false);
            break;

        case WM_ATTRACT_LIVE_OPERATOR_PREWAIT:
            enter_phase(live, WM_ATTRACT_LIVE_OPERATOR_WAIT,
                        WM_ATTRACT_OPERATOR_WAIT_TSEC * WM_FIX39_ATTRACT_TSEC,
                        true);
            break;

        case WM_ATTRACT_LIVE_OPERATOR_WAIT:
            enter_phase(live, WM_ATTRACT_LIVE_OPERATOR_CLEANUP, 0u, false);
            break;

        case WM_ATTRACT_LIVE_IDLE:
        case WM_ATTRACT_LIVE_WAIT_EXTERNAL:
        case WM_ATTRACT_LIVE_TIME_DATE_QUERY:
        case WM_ATTRACT_LIVE_TIME_DATE_PRESENT:
        case WM_ATTRACT_LIVE_OPERATOR_QUERY:
        case WM_ATTRACT_LIVE_OPERATOR_PRESENT:
        case WM_ATTRACT_LIVE_OPERATOR_CLEANUP:
        case WM_ATTRACT_LIVE_DONE:
            break;
    }
    return live->done;
}
