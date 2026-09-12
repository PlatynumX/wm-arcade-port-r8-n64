#ifndef WM_APP_H
#define WM_APP_H

#include <stdbool.h>
#include <stddef.h>
#include "wm/attract.h"
#include "wm/arcade/wm_arcade_sound.h"
#include "wm/audio.h"
#include "wm/award.h"
#include "wm/demo.h"
#include "wm/match.h"
#include "wm/process.h"
#include "wm/roster.h"
#include "wm/source_clock.h"
#include "wm/pregame.h"
#include "wm/select_screen.h"
#include "wm/select_continue.h"
#include "wm/arcade/wmania_rng.h"

/* DISPLAY.EQU: TSEC equ 53. Source sleeps expressed in TSEC use this rate.
   Literal source sleeps such as SLEEP 60 stay literal 60 source ticks. */
#define WM_FRONTEND_TICKS_PER_SEC WM_SOURCE_TICKS_PER_SEC
#define WM_ATTRACT_BOOT_DELAY_TICKS 8u

/* ATTRACT.ASM::show_sports_logo exact sleep boundaries:
   SLEEPK 2; build logo; SLEEPK 1; display; SLEEPK 32; CREATE WATER_PID;
   SLEEP TSEC/2; wait_on_butn 8*TSEC. */
#define WM_SPORTS_SETUP_TICKS 2u
#define WM_SPORTS_LOGO_PREDISPLAY_TICKS 1u
#define WM_SPORTS_LOGO_VISIBLE_TICK \
    (WM_SPORTS_SETUP_TICKS + WM_SPORTS_LOGO_PREDISPLAY_TICKS)
#define WM_SPORTS_LOGO_SCROLL_START_TICKS \
    (WM_SPORTS_LOGO_VISIBLE_TICK + 32u)
#define WM_SPORTS_LOGO_BUTTON_ENABLE_TICKS \
    (WM_SPORTS_LOGO_SCROLL_START_TICKS + WM_SOURCE_TICKS_PER_SEC / 2u)
#define WM_SPORTS_LOGO_TOTAL_TICKS \
    (WM_SPORTS_LOGO_BUTTON_ENABLE_TICKS + 8u * WM_SOURCE_TICKS_PER_SEC)

/* ATTRACT.ASM::show_title: SLEEPK 2, build, SLEEPK 2, CREATE processes,
   SLEEP TSEC/2, wait_on_butn 10*TSEC. */
#define WM_TITLE_SETUP_TICKS 4u
#define WM_TITLE_BUTTON_ENABLE_TICKS \
    (WM_TITLE_SETUP_TICKS + WM_SOURCE_TICKS_PER_SEC / 2u)
#define WM_TITLE_TOTAL_TICKS \
    (WM_TITLE_BUTTON_ENABLE_TICKS + 10u * WM_SOURCE_TICKS_PER_SEC)
#define WM_TITLE_LAVA_PERIOD_TICKS 5u
#define WM_TITLE_LAVA_STEPS 32u

/* ATTRACT.ASM::show_gameplay (WRESTLE.ASM::start_match, PSTATUS==0 path):
   SLEEP 3*60 (literal, not TSEC-scaled), then wait_on_butn 10*TSEC. */
#define WM_GAMEPLAY_RUN_TICKS (3u * 60u)
#define WM_GAMEPLAY_BUTTON_ENABLE_TICKS WM_GAMEPLAY_RUN_TICKS
#define WM_GAMEPLAY_TOTAL_TICKS \
    (WM_GAMEPLAY_BUTTON_ENABLE_TICKS + 10u * WM_SOURCE_TICKS_PER_SEC)

/* Recovered from the rev 1.30 arcade program ROM.
   ATTRACT.ASM passes A8=[102,7], A10=WHERE_WRESTLMANIA_SPARKLES, A9=4.
   A9 is the inclusive maximum family number for RNDRNG0, not a glint count.
   The ROM table contains 40 (x,y) offsets followed by FFFF,FFFF. */
#define WM_TITLE_SPARKLE_ORIGIN_Y 102u
#define WM_TITLE_SPARKLE_ORIGIN_X 7u
#define WM_TITLE_SPARKLE_SITE_COUNT 40u
#define WM_TITLE_SPARKLE_FAMILY_COUNT 5u
#define WM_TITLE_SPARKLE_SLOT_COUNT 8u
#define WM_TITLE_SPARKLE_FRAME_TICKS 2u
#define WM_TITLE_SPARKLE_SWEEP_PERIOD_TICKS 5u
#define WM_TITLE_SPARKLE_RANDOM_POLL_TICKS 20u
#define WM_TITLE_SPARKLE_RANDOM_POST_TICKS 30u
#define WM_TITLE_GLINT_COUNT WM_TITLE_SPARKLE_SLOT_COUNT
#define WM_TITLE_GLINT_ANIM_FRAMES 15u
#define WM_TITLE_RANDOM_ANIM_FRAMES 13u
#define WM_TITLE_SPARKLE_INFERRED_FRAME_TICKS WM_TITLE_SPARKLE_FRAME_TICKS

/* ATTRACT.ASM::DCS_LOGO boundaries. The 60-tick sleep here is literal source
   data, not TSEC, and therefore intentionally remains 60 source ticks. */
#define WM_DCS_STATIC_TICKS 0x42u
#define WM_DCS_ROT_SETUP_TICKS 1u
#define WM_DCS_ROT_UNSKIPPABLE_TICKS 60u
#define WM_DCS_ROT_SKIPPABLE_TICKS (254u - 60u)
#define WM_DCS_STATIC_RETURN_TICKS 10u
#define WM_DCS_BURST_FLASH_TICKS 6u
#define WM_DCS_BURST_WAIT_TICKS 30u
#define WM_DCS_BURST_SKIPPABLE_TICKS 100u

/* Portable PID values are local identifiers for translated source CREATE /
   KILALL behavior. They intentionally remain distinct from presentation IDs. */
enum {
    WM_PID_CYCLE_LAVA = 0x0101,
    WM_PID_WATER = 0x0102,
    WM_PID_FLASH = 0x0103,
    WM_PID_ATTRACT_ANIM = 0x0104
};

typedef enum {
    WM_ATTRACT_FLOW_BASE = 0,
    WM_ATTRACT_FLOW_EVEN_CREDITS,
    WM_ATTRACT_FLOW_TIME_DATE,
    WM_ATTRACT_FLOW_COPYRIGHT,
    WM_ATTRACT_FLOW_AAMA
} wm_attract_flow;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t family;
    uint8_t frame;
    bool active;
} wm_title_sparkle;

typedef struct {
    wm_attract_call call;
    wm_attract_flow flow;
    size_t source_index;
    unsigned amode_loops;
    unsigned call_ticks;
    unsigned phase_ticks;

    wm_dcs_phase dcs_phase;
    int sports_world_x;
    int sports_world_y;
    unsigned title_lava_step;
    wm_title_sparkle title_glints[WM_TITLE_GLINT_COUNT];
    wm_title_sparkle title_random_sparkle;
    uint32_t title_random_state;
    uint32_t title_rng_counter;
} wm_attract_state;

typedef enum {
    WM_APP_MODE_ATTRACT = 0,
    WM_APP_MODE_SELECT,
    WM_APP_MODE_PREGAME,
    WM_APP_MODE_MATCH_INIT,
    /* WRESTLE.ASM::start_match's #1plyr path -- see wm/match.h for exactly
       what wm_match_start_selected/wm_match_tick do and don't translate. */
    WM_APP_MODE_MATCH,
    /*
     * WRESTLE.ASM:1116, the code after `JSRP start_match` -- "The only
     * time we return from start_match is when the match is over". This
     * mode is that return: one tick that decides where the game goes,
     * on the `PSTATUS andn match_winner` test.
     *
     * A human who WON goes straight back to WM_APP_MODE_PREGAME and
     * the next rung of the ladder, with no select screen, which is
     * the source's own `jruc do_pregame`. A human who LOST goes to
     * the buy-in below.
     */
    WM_APP_MODE_MATCH_OVER,
    /*
     * WRESTLE.ASM:1183 `JSRP buyin_select` -- SELECT.ASM's continue
     * offer, already translated in wm/select_continue.h and, until
     * now, initialised by this file and never used. Accept and the
     * same opponent comes round again; let it run out and the game
     * is over.
     */
    WM_APP_MODE_CONTINUE
} wm_app_mode;

typedef struct {
    wm_audio_state audio;
    /*
     * DCSSOUND.ASM's four-channel mixer (wm/arcade/wm_arcade_sound.h).
     * Every sound index the game decides on goes through this before
     * it reaches the queue above, which is what decides whether it is
     * played at all.
     */
    wm_sound_state_t sound;
    wm_app_mode mode;
    wm_select_screen_state select;
    wm_select_continue_state continue_select;
    bool done_howard;
    wm_award_state awards;
    wm_pregame_state pregame;
    wm_attract_state attract;
    wm_demo demo;
    wm_wrestler_id p1_choice;
    wm_wrestler_id p2_choice;
    bool show_debug;

    /* WRESTLE.ASM::start_match's PSTATUS==0 path, driven from
       WM_ATTRACT_SHOW_GAMEPLAY. See wm/match.h for exactly what is and is
       not translated. */
    wm_match_state match;

    /* Shared @RAND state. All RNDRNG0 draws (show_gameplay's wrestler pick
       included) come from this single stream, matching the source's one
       global RAND. HCOUNT/SP hardware entropy is not wired up yet -- see
       wmania_rng.h -- so this is seeded plainly rather than from real
       cabinet jitter. */
    WmRng rng;

    /* Source execution infrastructure. The display backend calls
       wm_app_video_frame at 60 Hz; this advances wm_app_tick at exactly 53 Hz. */
    wm_source_clock source_clock;
    wm_scheduler scheduler;
    wm_input_state latched_input;
    unsigned boot_ticks;
    bool attract_started;
    /*
     * WRESTLE.ASM:1131 `movi 60,a0 / move a0,@are_we_waiting_f` --
     * "set delay before allowing player to select a wrestler", run on
     * the way out of every match.
     */
    unsigned are_we_waiting_f;
    /*
     * @match_winner as the post-match code reads it: 1 = player one's
     * side, 2 = player two's, 0 = the CPU took it. Kept here because
     * the match state is re-initialised by the next wm_match_start_*
     * and this outlives it.
     */
    int32_t last_match_winner;
    /* The continue offer reads a Start EDGE, not the level: a Start
       still held from the match must not buy in by itself. */
    bool continue_start_was_down;
} wm_app;

void wm_app_init(wm_app *app);
/* Advance exactly one original source tick. Host tests and translated process
   code use this entry point directly. */
void wm_app_tick(wm_app *app, const wm_input_state *input);
/* Advance one 60 Hz N64 video frame and execute a source tick when the 53 Hz
   accumulator says one is due. Returns true when source state advanced. */
bool wm_app_video_frame(wm_app *app, const wm_input_state *input);

bool wm_attract_call_is_translated(wm_attract_call call);
bool wm_app_any_attract_button(const wm_input_state *input);

/* Execute one source tick with independent cabinet P1/P2 inputs. */
void wm_app_tick_dual(wm_app *app,
                      const wm_input_state *p1_input,
                      const wm_input_state *p2_input);
#endif
