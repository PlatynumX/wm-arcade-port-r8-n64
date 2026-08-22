#include "wmania_attract_adapter.h"

static bool music_adjustment(const WmAttractAdapter *adapter)
{
    return adapter != 0 &&
           adapter->music_adjustment_nonzero != 0 &&
           adapter->music_adjustment_nonzero(adapter->user);
}

size_t wm_attract_run_cycle(
    WmAttractState *state,
    WmHsSystem *hiscore,
    const WmAttractOperatorMessage *operator_message,
    const WmAttractAdapter *adapter)
{
    WmAttractStep steps[20];
    size_t n;
    size_t i;

    if (state == 0 || adapter == 0) {
        return 0u;
    }

    n = wm_attract_build_cycle(
        state, steps, sizeof(steps) / sizeof(steps[0]));

    for (i = 0; i < n; ++i) {
        const WmAttractStep *step = &steps[i];

        switch (step->screen) {
        case WM_FIX39_ATTRACT_HISCORES:
            if (adapter->show_hiscores != 0 && hiscore != 0) {
                (void)wm_hs_system_table_cmos_check(hiscore);
                adapter->show_hiscores(adapter->user, hiscore);
            }
            break;

        case WM_FIX39_ATTRACT_DCS_LOGO:
            if (adapter->show_dcs_logo != 0)
                adapter->show_dcs_logo(adapter->user);
            break;

        case WM_FIX39_ATTRACT_SPORTS_LOGO:
            if (adapter->show_sports_logo != 0)
                adapter->show_sports_logo(adapter->user);
            break;

        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1:
        case WM_FIX39_ATTRACT_GAMEPLAY_DEMO_2:
            if (adapter->show_gameplay_demo_unimplemented != 0) {
                adapter->show_gameplay_demo_unimplemented(
                    adapter->user,
                    step->screen == WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1 ? 1u : 2u);
            }
            break;

        case WM_FIX39_ATTRACT_CREDITS_1:
        case WM_FIX39_ATTRACT_CREDITS_2:
        case WM_FIX39_ATTRACT_EVEN_LOOP_CREDITS:
            if (adapter->show_credits_crd_scrn2 != 0)
                adapter->show_credits_crd_scrn2(adapter->user);
            break;

        case WM_FIX39_ATTRACT_TITLE:
            if (adapter->show_title != 0)
                adapter->show_title(adapter->user);
            break;

        case WM_FIX39_ATTRACT_DESIGNER_HINT:
            if (adapter->show_designer_hint != 0) {
                adapter->show_designer_hint(
                    adapter->user,
                    &wm_attract_hints[step->hint_index]);
            }
            break;

        case WM_FIX39_ATTRACT_GENERAL_TIPS:
            if (adapter->show_general_tips != 0)
                adapter->show_general_tips(adapter->user);
            break;

        case WM_FIX39_ATTRACT_BIO:
            if (adapter->show_bio != 0) {
                WmAttractState at_screen = *state;
                at_screen.amode_loops = step->source_amode_loops;
                bool music = wm_attract_bio_music_allowed(
                    &at_screen, music_adjustment(adapter));
                adapter->show_bio(
                    adapter->user,
                    &wm_attract_bios[step->wrestler_index],
                    false, music);
                if (music && adapter->play_wrestler_tune != 0) {
                    adapter->play_wrestler_tune(
                        adapter->user,
                        wm_attract_bios[step->wrestler_index].tune_id);
                }
            }
            break;

        case WM_FIX39_ATTRACT_BIO_TIPS:
            if (adapter->show_bio != 0) {
                WmAttractState at_screen = *state;
                at_screen.amode_loops = step->source_amode_loops;
                bool music = wm_attract_bio_music_allowed(
                    &at_screen, music_adjustment(adapter));
                adapter->show_bio(
                    adapter->user,
                    &wm_attract_bios[step->wrestler_index],
                    true, music);
                if (music && adapter->play_wrestler_tune != 0) {
                    adapter->play_wrestler_tune(
                        adapter->user,
                        wm_attract_bios[step->wrestler_index].tune_id);
                }
            }
            break;

        case WM_FIX39_ATTRACT_OPERATOR_MESSAGE:
            if (adapter->show_operator_message != 0 &&
                wm_attract_operator_has_message(operator_message)) {
                adapter->show_operator_message(
                    adapter->user, operator_message);
            }

            /*
             * Source RemapIO follows show_operatormsg every cycle,
             * even when no operator message is present.
             */
            if (adapter->remap_io != 0)
                adapter->remap_io(adapter->user);
            break;

        case WM_FIX39_ATTRACT_TIME_DATE:
            if (adapter->time_date_dip_enabled != 0 &&
                adapter->time_date_dip_enabled(adapter->user) &&
                adapter->read_clock != 0 &&
                adapter->show_time_date != 0) {
                WmAttractClock clock;
                WmAttractClockText text;
                if (adapter->read_clock(adapter->user, &clock) &&
                    wm_attract_format_clock(&clock, &text)) {
                    adapter->show_time_date(adapter->user, &text);
                }
            }
            break;

        case WM_FIX39_ATTRACT_COPYRIGHT:
            if (adapter->show_copyright != 0)
                adapter->show_copyright(adapter->user);
            break;

        case WM_FIX39_ATTRACT_AAMA:
            if (adapter->show_aama != 0)
                adapter->show_aama(adapter->user);
            break;
        }
    }

    return n;
}
