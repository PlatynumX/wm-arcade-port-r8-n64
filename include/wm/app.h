#ifndef WM_APP_H
#define WM_APP_H

#include <stdbool.h>
#include <stddef.h>
#include "wm/attract.h"
#include "wm/arcade/wmania_attract_data.h"
#include "wm/arcade/wm_arcade_sound.h"
#include "wm/arcade/wm_arcade_music.h"
#include "wm/arcade/wmania_hiscore_entry.h"
#include "wm/arcade/wmania_hiscore_persist.h"
#include "wm/arcade/wmania_hiscore_system.h"
#include "wm/arcade/wmania_hstd_screen.h"
#include "wm/audio.h"
#include "wm/award.h"
#include "wm/arcade/wm_arcade_powerup.h"
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

/*
 * @royal_rumble. This port has no ladder state that can set it, so
 * it is false everywhere -- but it is read in two unrelated places
 * (#2plyr's PSIDE subtract and the powerup codes' `jrnz #die`), and
 * naming it once keeps those two from drifting apart when a rumble
 * does become reachable.
 */
#define WM_APP_ROYAL_RUMBLE false

/* ATTRACT.ASM::show_title: SLEEPK 2, build, SLEEPK 2, CREATE processes,
   SLEEP TSEC/2, wait_on_butn 10*TSEC. */
#define WM_TITLE_SETUP_TICKS 4u
#define WM_TITLE_BUTTON_ENABLE_TICKS \
    (WM_TITLE_SETUP_TICKS + WM_SOURCE_TICKS_PER_SEC / 2u)
#define WM_TITLE_TOTAL_TICKS \
    (WM_TITLE_BUTTON_ENABLE_TICKS + 10u * WM_SOURCE_TICKS_PER_SEC)
#define WM_TITLE_LAVA_PERIOD_TICKS 5u
#define WM_TITLE_LAVA_STEPS 32u

/*
 * ATTRACT.ASM:1095 show_copyright, exact sleep boundaries.
 *
 *   SLEEPK 2                      -- page one's nine lines are placed
 *   BEGINOBJ SMWWF2 / CREATE fade_up / DISPLAYON
 *   SLEEPK 2 / display_unblank
 *   SLEEPK 20
 *   wait_on_butn 3*TSEC
 *   obj_del1c CLSNEUT|TYPTEXT|SUBTXT   -- page one's text deleted
 *   ...page two's ten lines...
 *   SLEEPK 20
 *   wait_on_butn 3*TSEC
 *   RETP
 *
 * Both waits are skippable, which is what wait_on_butn means, so a
 * player can page through. The geometry (x 200, first y 110, step 12,
 * 9 then 10 lines) is in wm/arcade/wmania_attract_data.h and matches
 * the source's `movi [50+60,200],a9` through `[158+60,200]`.
 */
#define WM_COPYRIGHT_PLACE_TICKS 2u
#define WM_COPYRIGHT_UNBLANK_TICKS (WM_COPYRIGHT_PLACE_TICKS + 2u)
#define WM_COPYRIGHT_PAGE1_SETTLE_TICKS \
    (WM_COPYRIGHT_UNBLANK_TICKS + WM_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS)
#define WM_COPYRIGHT_PAGE1_TOTAL_TICKS \
    (WM_COPYRIGHT_PAGE1_SETTLE_TICKS + \
     WM_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)
#define WM_COPYRIGHT_PAGE2_SETTLE_TICKS \
    (WM_COPYRIGHT_PAGE1_TOTAL_TICKS + WM_ATTRACT_COPYRIGHT_FADE_SETTLE_TICKS)
#define WM_COPYRIGHT_TOTAL_TICKS \
    (WM_COPYRIGHT_PAGE2_SETTLE_TICKS + \
     WM_ATTRACT_COPYRIGHT_PAGE_WAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)

/*
 * ATTRACT.ASM:274 aama_message. SLEEPK 2; do_the_grad_thang; the six
 * lines; SLEEPK 2 + display_unblank; SLEEPK 20; wait_on_butn 4*TSEC.
 * One page, and a longer wait than the copyright screen's three
 * seconds -- it is a parental advisory and the source holds it longer.
 */
/*
 * creditscreen (ATTRACT.ASM:793) through CRD_SCRN2 (AUDIT.ASM:998). Three
 * SLEEPK 2 with the text going up between the first two, then SLEEP 1*TSEC,
 * then a 4*TSEC button loop. See wm/arcade/wmania_attract_data.h for why
 * this screen shows one fixed state.
 */
#define WM_CREDIT_SETTLE_TICKS \
    (WM_ATTRACT_CREDIT_UNBLANK_TICKS + \
     WM_ATTRACT_CREDIT_SETTLE_TSEC * WM_SOURCE_TICKS_PER_SEC)
#define WM_CREDIT_TOTAL_TICKS \
    (WM_CREDIT_SETTLE_TICKS + \
     WM_ATTRACT_CREDIT_WAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)

#define WM_AAMA_PLACE_TICKS 2u
#define WM_AAMA_UNBLANK_TICKS (WM_AAMA_PLACE_TICKS + 2u)
#define WM_AAMA_SETTLE_TICKS \
    (WM_AAMA_UNBLANK_TICKS + WM_ATTRACT_AAMA_SETTLE_TICKS)
#define WM_AAMA_TOTAL_TICKS \
    (WM_AAMA_SETTLE_TICKS + \
     WM_ATTRACT_AAMA_WAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)

/*
 * show_gen_tips (ATTRACT.ASM:1416). print_gen_tips puts the column up before
 * the SLEEPK 1, then BLOW_0_TO_1 blanks the screen for its own run and brings
 * it back, then SLEEP TSEC, then wait_on_butn for 10*TSEC.
 */
#define WM_GEN_TIPS_PLACE_TICKS 1u
#define WM_GEN_TIPS_REVEAL_TICKS \
    (WM_GEN_TIPS_PLACE_TICKS + WM_ATTRACT_BLOW_0_TO_1_TICKS)
#define WM_GEN_TIPS_SETTLE_TICKS \
    (WM_GEN_TIPS_REVEAL_TICKS + \
     WM_ATTRACT_GENERAL_TIPS_PREWAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)
#define WM_GEN_TIPS_TOTAL_TICKS \
    (WM_GEN_TIPS_SETTLE_TICKS + \
     WM_ATTRACT_GENERAL_TIPS_WAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)

/*
 * DO_HINTS (ATTRACT.ASM:3390). CLOSE_SCREEN_LINE first, then the page is
 * built, then OPEN_SCREEN_LINE reveals it, then SLEEP 80, then wait_on_butn
 * for 15*TSEC. Both line wipes cost the same; see WM_ATTRACT_SCREEN_LINE_TICKS.
 */
#define WM_HINTS_PLACE_TICKS ((unsigned)WM_ATTRACT_SCREEN_LINE_TICKS)
#define WM_HINTS_REVEAL_TICKS \
    (WM_HINTS_PLACE_TICKS + WM_ATTRACT_SCREEN_LINE_TICKS)
#define WM_HINTS_SETTLE_TICKS \
    (WM_HINTS_REVEAL_TICKS + WM_ATTRACT_HINT_PREWAIT_TICKS)
#define WM_HINTS_TOTAL_TICKS \
    (WM_HINTS_SETTLE_TICKS + \
     WM_ATTRACT_HINT_WAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)

/*
 * show_bios and show_bios_tips (ATTRACT.ASM:2067/:2070) -- one routine with
 * two entry points, @bios_type choosing which text it prints. Unlike
 * show_gen_tips it has no SLEEPK before BLOW_0_TO_1, so the page is placed on
 * the call's first tick.
 */
#define WM_BIOS_PLACE_TICKS 0u
#define WM_BIOS_REVEAL_TICKS \
    (WM_BIOS_PLACE_TICKS + WM_ATTRACT_BLOW_0_TO_1_TICKS)
#define WM_BIOS_SETTLE_TICKS \
    (WM_BIOS_REVEAL_TICKS + \
     WM_ATTRACT_BIO_NORMAL_PREWAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)
#define WM_BIOS_TOTAL_TICKS \
    (WM_BIOS_SETTLE_TICKS + \
     WM_ATTRACT_BIO_NORMAL_WAIT_TSEC * WM_SOURCE_TICKS_PER_SEC)

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
    /*
     * show_copyright's page, 0 or 1. The source has no variable for it
     * -- the two pages are straight-line code either side of an
     * obj_del1c -- so the port needs one to say which set of lines is
     * on screen. -1 means the text has not been placed yet, which is
     * the SLEEPK 2 before the first line goes down.
     */
    int copyright_page;
    /* aama_message has one page; this is simply whether its lines have
       been placed yet, i.e. whether the SLEEPK 2 is past. */
    bool aama_placed;

    /*
     * DO_HINTS' and show_bios' source-persistent choices.
     *
     * ATTRACT.ASM keeps `last_hint` (.bss, :3388) and `next_bio` (.bss,
     * :2065) across attract cycles and steps each on entry, so which hint
     * and which wrestler come up is a function of how long the cabinet has
     * been looping, not of the call. attract_mode backs next_bio up one
     * between show_bios and show_bios_tips (:237-:241) so both screens show
     * the same wrestler; the source's nb_save (.bss, :131) is not carried
     * here because its round trip is dead -- see the comment on that branch
     * in begin_call().
     *
     * hint_index and bio_index are the resolved choices for the call that is
     * running, each set on entry to its own call. -1 is "nothing has chosen
     * yet" and is what wm_app_init leaves them at, because 0 is a real hint
     * and a real wrestler. bios_tips is @bios_type,
     * which attract_mode sets to 1 for the SPECIAL MOVES page (:235) and back
     * to 0 after (:244).
     *
     * NOTE: src/core/arcade/wmania_attract_core.c holds a second copy of
     * these same stepping rules, for a sequencer that only its own tests
     * drive. Unifying the two is a refactor and not done here; if either
     * changes, both must.
     */
    int16_t last_hint;
    int16_t next_bio;
    int hint_index;
    int bio_index;
    bool bios_tips;

    /*
     * show_hstd's whole machine (ATTRACT.ASM:1443). It is big enough and
     * self-contained enough to live in its own module; see
     * wm/arcade/wmania_hstd_screen.h. It reads the high-score system this
     * struct's owner already holds.
     */
    WmHstdState hstd;
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
    WM_APP_MODE_CONTINUE,
    /*
     * SELECT.ASM:1190 do_game_over, which the declined continue used
     * to skip straight past on its way back to attract. Four seconds
     * of GAME OVER with the master volume fading under it, and the
     * bookkeeping a finished game owes the next one.
     */
    WM_APP_MODE_GAME_OVER,
    /*
     * HSTD.ASM's initials input, reached from SELECT.ASM:211's
     * `JSRP DO_BEATEN_GAME` when a finished game qualifies for a
     * table. The source runs it as a HI_INPUT_PID process the
     * game-over path waits on (`are_we_waiting4`); this port has no
     * process system for it, so it is an app mode instead.
     */
    WM_APP_MODE_HISCORE_ENTRY
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
    /*
     * The two SOUND_PID processes a match asks for through its own
     * seams: LIFEBAR.ASM's ring_bell and AWARD.ASM's END_MATCH_SPEECH.
     * They live here because they need the mixer and the audio queue,
     * both of which are the app's; the match only says when.
     */
    wm_sound_bell_t bell;
    wm_sound_pin_him_t pin_him;
    /*
     * @MUSIC_HAP (LIFEBAR.ASM:110) and the DO_RIGHT_MUSIC process that
     * sets it. A third SOUND_PID process beside the two above, and the
     * one piece of state the attract, the pregame and the match-end
     * all three touch -- which is why it is here and not on any of
     * them. wm/arcade/wm_arcade_music.h has the whole chain.
     */
    wm_music_state_t music;
    /*
     * ADJMUSIC, the one CMOS adjustment the music gates read. This
     * port has no operator-settings system, so it holds the value
     * FACTORY_TABLE ships (AUDIT.ASM:2927: `.word 1 ;attract mode
     * music = off`) -- the same reasoning as ADJVOLUME's
     * WM_SOUND_ADJVOLUME_DEFAULT, and the same reasoning the credit
     * screen's figures already rest on. It is a field rather than a
     * constant so a test can turn attract music on and see the other
     * half of every gate.
     */
    uint16_t adj_music;

    /*
     * HSTD.ASM's high-score tables, and the initials entry that feeds
     * them.
     *
     * Nine files and ~1,900 lines of this were translated, unit-tested,
     * and called by nothing outside themselves: no game ever qualified
     * for a table, no initials were ever entered, and nothing was ever
     * saved or loaded. The tables existed and the game could not reach
     * them.
     *
     * The save backend is deliberately abstract -- read/write callbacks
     * the port supplies (wm/arcade/wmania_hiscore_persist.h says so in
     * as many words). The host build keeps the bytes in RAM, which is
     * exactly right for a host build: they survive an attract loop and
     * a game, and nothing pretends they survive the process.
     */
    WmHsSystem hiscore;
    WmHsEntryState hs_entry;
    WmHsPendingEntry hs_pending;
    bool hs_pending_valid;
    WmHsSaveBackend hs_backend;
    /* WM_HS_SAVE_MAX_BYTES of encoded table, plus how much is real. */
    uint8_t hs_save_bytes[WM_HS_SAVE_MAX_BYTES];
    size_t hs_save_len;
    size_t hs_save_cursor;
    /* Counters a test can read: how many entries have been committed,
       and how many times the tables have been written out. */
    uint32_t hs_commits;
    uint32_t hs_writes;
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

    /*
     * AWARD.ASM's @p1powerup_request / @p2powerup_request, the two
     * words #2plyr ANDs to decide buddy mode, plus the output flags
     * get_powerups derives from them.
     *
     * Filled for real: powerup_window_start opens the code window
     * where PROGRESS.ASM's CLOSE_PROGRESS_SCREEN opens it, the
     * attempts below run through the pregame, and
     * powerup_window_close runs get_powerups on the way into the
     * match.
     */
    wm_powerup_flags powerups;

    /*
     * AWARD.ASM:2182 player_powerup_checker: each player has one
     * live attempt per code, all racing at once, and PROGRESS.ASM's
     * CLOSE_PROGRESS_SCREEN is what starts them -- which is why
     * they run through WM_APP_MODE_PREGAME and are reconciled by
     * get_powerups on the way into the match.
     *
     * `spawned` on a code row is the source's own commented-out
     * CREATE (no_ring is disabled "until we get blimp module"), so a
     * row that is not spawned never gets an attempt here either.
     */
    wm_powerup_attempt powerup_attempt[2][WM_PUP_CODE_SLOTS];
    bool powerup_window_open;
    /* PUPWAITSWITCH compares switches-DOWN, so the level a
       controller reports has to be edge-detected first. */
    int32_t powerup_prev_switches[2];

    /*
     * @PSTATUS as the select screen left it: 1 for one player, 3
     * when a second joined. start_match branches on it, so it is
     * settled once when select finishes and read again at
     * MATCH_INIT rather than re-derived.
     */
    int32_t match_pstatus;
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
    /*
     * WRESTLE.ASM:160 PCNT, "Main loop cnt" -- bumped by
     * WRESTLE.ASM:542's `move @PCNT,a0,L / addk 1,a0 / move a0,@PCNT,L`
     * at the bottom of every pass through mainlp, and cleared by
     * nothing. wm_app_tick IS that pass, so this counts there and is
     * lent to the pregame, whose scramble_table_entry reads its low five
     * bits for the triple-Doink draw.
     *
     * The match still keeps its own per-match tick_count for the PCNT
     * its combat code wants; the two are deliberately not merged here,
     * because doing so changes timestamps all through REACT and DRONE
     * and belongs in its own change.
     */
    uint32_t pcnt;
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
    /* FADE_MASTER_VOL's own process state, for the one place that
       starts it: do_game_over. */
    wm_sound_fade_t volume_fade;
    /* `SLEEP TSEC*4` -- how long GAME OVER stays up. */
    int32_t game_over_ticks;
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
