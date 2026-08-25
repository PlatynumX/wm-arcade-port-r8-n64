#ifndef WM_APP_H
#define WM_APP_H

#include <stdbool.h>
#include <stddef.h>
#include "wm/attract.h"
#include "wm/audio.h"
#include "wm/arcade_sound.h"
#include "wm/award.h"
#include "wm/demo.h"
#include "wm/process.h"
#include "wm/roster.h"
#include "wm/source_clock.h"
#include "wm/pregame.h"
#include "wm/select_screen.h"
#include "wm/select_continue.h"
#include "wm/sdcard_hiscore_backend.h"
#include "wm/cabinet_bridge.h"

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
    WM_APP_MODE_MATCH
} wm_app_mode;

typedef struct {
    wm_audio_state audio;
    wm_arcade_sound sound;
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

    /* Source execution infrastructure. The display backend calls
       wm_app_video_frame at 60 Hz; this advances wm_app_tick at exactly 53 Hz. */
    wm_source_clock source_clock;
    wm_scheduler scheduler;
    wm_input_state latched_input;
    unsigned boot_ticks;
    bool attract_started;

    /* N64 SD-card filesystem persistence backend for source HSTD tables. */
    WmHsSdCardBackend hiscore_sdcard_backend;
    WmHsSaveBackend hiscore_save_backend;

    /* Source PSTATUS semantics behind an explicit N64 platform-input bridge. */
    wm_cabinet_bridge_state cabinet;
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
