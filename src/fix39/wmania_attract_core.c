#include "wmania_attract_core.h"

#include <string.h>

static bool push(
    WmAttractStep *steps,
    size_t capacity,
    size_t *count,
    WmAttractScreen screen,
    uint8_t hint,
    uint8_t wrestler,
    bool demo,
    bool optional,
    uint16_t source_amode_loops)
{
    if (*count >= capacity) {
        return false;
    }

    steps[*count].screen = screen;
    steps[*count].hint_index = hint;
    steps[*count].wrestler_index = wrestler;
    steps[*count].gameplay_demo_unimplemented = demo;
    steps[*count].source_condition_optional = optional;
    steps[*count].source_amode_loops = source_amode_loops;
    ++*count;
    return true;
}

void wm_attract_init(WmAttractState *state)
{
    memset(state, 0, sizeof(*state));

    /*
     * BSS last_hint is zero in the original. DO_HINTS increments before
     * selecting, so the first normal call chooses active hint index 1.
     */
    state->last_hint = 0;

    /* Source attract_mode explicitly enables page flipping. */
    state->page_flip_enabled = true;
}

size_t wm_attract_build_cycle(
    WmAttractState *state,
    WmAttractStep *steps,
    size_t capacity)
{
    size_t n = 0u;
    uint8_t hint;
    uint8_t bio;

    if (state == 0 || steps == 0) {
        return 0u;
    }

    /*
     * Exact active DO_HINTS behavior:
     * last_hint++ and wrap at NUM_HINTS=10.
     */
    hint = (uint8_t)(state->last_hint + 1);
    if (hint >= WM_ATTRACT_ACTIVE_HINTS) {
        hint = 0u;
    }
    state->last_hint = (int16_t)hint;

    /*
     * Exact show_bios selection:
     * (next_bio & 0x0f) + 1; if >=8 -> 0.
     * The subsequent tips path deliberately backs next_bio up one before
     * entering the same incrementing show_bios_tips body, so both screens
     * show the same wrestler and next_bio ends on that wrestler.
     */
    bio = (uint8_t)((state->next_bio & 0x0fu) + 1u);
    if (bio >= WM_ATTRACT_WRESTLERS) {
        bio = 0u;
    }
    state->next_bio = bio;

#define ADD(scr, demo, optional) \
    do { if (!push(steps, capacity, &n, (scr), hint, bio, \
                   (demo), (optional), state->amode_loops)) return n; } while (0)

    ADD(WM_FIX39_ATTRACT_HISCORES, false, false);
    ADD(WM_FIX39_ATTRACT_DCS_LOGO, false, false);
    /* Project-retained Midway Sports slot.  Keep this screen in the active
       attract cycle so it can be repurposed later; do not discard the
       existing source-backed renderer/adapter. */
    ADD(WM_FIX39_ATTRACT_SPORTS_LOGO, false, false);

    /* V13 carries the exact show_gameplay setup plan. The actual start_match
       platform handoff remains a separate live-match adapter. */
    ADD(WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1, true, false);

    ADD(WM_FIX39_ATTRACT_CREDITS_1, false, false);
    ADD(WM_FIX39_ATTRACT_TITLE, false, false);

    ADD(WM_FIX39_ATTRACT_GAMEPLAY_DEMO_2, true, false);

    ADD(WM_FIX39_ATTRACT_CREDITS_2, false, false);
    ADD(WM_FIX39_ATTRACT_DESIGNER_HINT, false, false);
    ADD(WM_FIX39_ATTRACT_GENERAL_TIPS, false, false);
    ADD(WM_FIX39_ATTRACT_BIO, false, false);
    ADD(WM_FIX39_ATTRACT_BIO_TIPS, false, false);
    ADD(WM_FIX39_ATTRACT_OPERATOR_MESSAGE, false, true);

    /*
     * RemapIO occurs here in the arcade source, then AMODE_LOOPS increments.
     * The platform runner gets a dedicated callback for RemapIO rather than
     * representing it as a visual screen.
     */
    ++state->amode_loops;

    if ((state->amode_loops & 1u) == 0u) {
        ADD(WM_FIX39_ATTRACT_EVEN_LOOP_CREDITS, false, false);
        ADD(WM_FIX39_ATTRACT_TIME_DATE, false, true);
    }

    if ((state->amode_loops & 7u) == 0u) {
        ADD(WM_FIX39_ATTRACT_COPYRIGHT, false, false);
        ADD(WM_FIX39_ATTRACT_AAMA, false, false);

        state->amode_loops = 0u;
        state->soundsup = 0u;
    }

#undef ADD
    return n;
}

bool wm_attract_demo_should_suppress_sound(
    const WmAttractState *state,
    bool music_adjustment_nonzero)
{
    if (state == 0) {
        return music_adjustment_nonzero;
    }

    return music_adjustment_nonzero || state->amode_loops >= 2u;
}

bool wm_attract_bio_music_allowed(
    const WmAttractState *state,
    bool music_adjustment_nonzero)
{
    if (state == 0) {
        return false;
    }

    return state->amode_loops < 2u && !music_adjustment_nonzero;
}

uint16_t wm_attract_wait_button_begin(WmAttractState *state)
{
    uint16_t previous;

    if (state == 0) {
        return 0u;
    }

    previous = state->soundsup;
    state->soundsup = 0u;
    return previous;
}

void wm_attract_wait_button_end(
    WmAttractState *state,
    uint16_t previous_soundsup)
{
    if (state != 0) {
        state->soundsup = previous_soundsup;
    }
}


void wm_attract_demo_plan_make(WmAttractDemoPlan *plan,
                               uint16_t amode_loops,
                               bool adjust_music,
                               WmAttractDemoRngFn rng,
                               void *rng_user)
{
    uint32_t wrestler = rng ? rng(rng_user, 7u) : 0u;
    uint32_t ladder = rng ? rng(rng_user, 5u) : 0u;
    if (!plan) return;
    memset(plan, 0, sizeof(*plan));
    if (wrestler == 7u) ++wrestler;
    plan->current_round = 1u;
    plan->match_count = 1u;
    plan->p1rounds = 1u;
    plan->p2rounds = 1u;
    plan->player_wrestler = (uint8_t)wrestler;
    plan->ladder_battle = (uint8_t)(ladder + 1u);
    plan->warmup_frames = 3u * 60u;
    plan->freeze_frames = 3u * 60u;
    plan->freeze_hold_frames = 60u;
    plan->fade_frames = 32u;
    plan->show_credits = true;
    plan->sound_suppression = (adjust_music || amode_loops >= 2u) ? 2u : 0u;
}
