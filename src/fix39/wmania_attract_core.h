#ifndef WMANIA_ATTRACT_CORE_H
#define WMANIA_ATTRACT_CORE_H

#include "wmania_attract_data.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WM_FIX39_ATTRACT_HISCORES = 0,
    WM_FIX39_ATTRACT_DCS_LOGO,
    WM_FIX39_ATTRACT_SPORTS_LOGO,
    WM_FIX39_ATTRACT_GAMEPLAY_DEMO_1,
    WM_FIX39_ATTRACT_CREDITS_1,
    WM_FIX39_ATTRACT_TITLE,
    WM_FIX39_ATTRACT_GAMEPLAY_DEMO_2,
    WM_FIX39_ATTRACT_CREDITS_2,
    WM_FIX39_ATTRACT_DESIGNER_HINT,
    WM_FIX39_ATTRACT_GENERAL_TIPS,
    WM_FIX39_ATTRACT_BIO,
    WM_FIX39_ATTRACT_BIO_TIPS,
    WM_FIX39_ATTRACT_OPERATOR_MESSAGE,
    WM_FIX39_ATTRACT_EVEN_LOOP_CREDITS,
    WM_FIX39_ATTRACT_TIME_DATE,
    WM_FIX39_ATTRACT_COPYRIGHT,
    WM_FIX39_ATTRACT_AAMA
} WmAttractScreen;

typedef struct {
    WmAttractScreen screen;
    uint8_t hint_index;
    uint8_t wrestler_index;

    /* Gameplay demo steps retain their exact source call sites.  V13 carries
     * the translated setup plan; the platform still owns the live match handoff. */
    bool gameplay_demo_unimplemented;

    /*
     * `show_time_date` itself checks the cabinet time/date DIP. This flag
     * tells a platform adapter that it may legally skip if disabled.
     */
    bool source_condition_optional;

    /* AMODE_LOOPS value at the instant this source screen runs. */
    uint16_t source_amode_loops;
} WmAttractStep;

typedef struct {
    uint16_t amode_loops;
    int16_t last_hint;
    uint8_t next_bio;
    uint16_t soundsup;

    uint16_t total_matches;
    uint16_t music_hap;
    bool page_flip_enabled;
} WmAttractState;

void wm_attract_init(WmAttractState *state);

/*
 * Build exactly one top-level source attract cycle.
 * This advances source-persistent `last_hint`, `next_bio`, and AMODE_LOOPS.
 * The returned list includes gameplay demo placeholders at the exact two
 * source call sites but no gameplay implementation.
 */
size_t wm_attract_build_cycle(
    WmAttractState *state,
    WmAttractStep *steps,
    size_t capacity);

/* Source TURN_SOUNDS_OFF_IF_NEED. */
bool wm_attract_demo_should_suppress_sound(
    const WmAttractState *state,
    bool music_adjustment_nonzero);

/* Bio music gate used by show_bios_tips. */
bool wm_attract_bio_music_allowed(
    const WmAttractState *state,
    bool music_adjustment_nonzero);

/*
 * Source wait_on_butn temporarily unsuppresses sound to play the common
 * button-feedback sound, then restores SOUNDSUP. The platform adapter may
 * use this helper to preserve the state transition.
 */
uint16_t wm_attract_wait_button_begin(WmAttractState *state);
void wm_attract_wait_button_end(
    WmAttractState *state,
    uint16_t previous_soundsup);

/* Exact source feedback sound used by wait_on_butn. */
#define WM_ATTRACT_WAIT_BUTTON_SOUND_ID 0x00b6u


/* Direct ATTR.ASM show_gameplay setup plan. */
typedef struct {
    uint8_t current_round;
    uint8_t match_count;
    uint8_t p1rounds;
    uint8_t p2rounds;
    uint8_t player_wrestler;  /* 0..6 or 8: source skips spare slot 7 */
    uint8_t ladder_battle;    /* RNDRNG0(5)+1 => 1..6 */
    uint16_t warmup_frames;
    uint16_t freeze_frames;
    uint16_t freeze_hold_frames;
    uint16_t fade_frames;
    bool show_credits;
    uint8_t sound_suppression;
} WmAttractDemoPlan;

typedef uint32_t (*WmAttractDemoRngFn)(void *user, uint32_t inclusive_max);
void wm_attract_demo_plan_make(WmAttractDemoPlan *plan,
                               uint16_t amode_loops,
                               bool adjust_music,
                               WmAttractDemoRngFn rng,
                               void *rng_user);

#ifdef __cplusplus
}
#endif

#endif
