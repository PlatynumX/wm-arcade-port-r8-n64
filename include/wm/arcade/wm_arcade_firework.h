#ifndef WM_ARCADE_FIREWORK_H
#define WM_ARCADE_FIREWORK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FIREWORK.ASM -- what happens when somebody actually finishes the
 * ladder.
 *
 * The port had the ladder, the final match and the ending story, and
 * nothing at all in between: `do_fireworks` was the last big sequence
 * in the game with none of it read. It is one routine that clears the
 * screen of everything the match left behind, twenty-two flares, a
 * camera that flies a figure of eight over the arena, explosions
 * going off behind it, and the congratulations text.
 *
 * Nine of its twenty-three routines are data (tools/wlfirework.py
 * reads them; src/generated/firework_tables.c is what comes out) and
 * the rest is here. The camera is the interesting part: three tiny
 * routines and a waypoint table, and between them they do the whole
 * flight.
 */

/* ---- the tables (src/generated/firework_tables.c) ---------------- */

typedef struct {
    /*
     * The first column of `panning_points`. INIT_PAN_SPEED (= TSEC,
     * 53) on the first row and 3 on every other -- and this really is
     * what calc_dxdy divides by. pan_around loads `movk 15,a10`
     * immediately after, commented "Number of ticks for this move",
     * but calc_dxdy reads @ticks and never looks at a10, so the 15 is
     * dead and the table's own column is what flies the camera.
     */
    int32_t ticks;
    int32_t x;
    int32_t y;
} wm_fw_waypoint_t;

extern const wm_fw_waypoint_t wm_fw_panning_points[];
extern const size_t wm_fw_panning_point_count;

/* FIREWORK.ASM:129 `EXP_FWY .equ -260`, the Y the waypoints and the
   explosion heights are both written relative to. */
#define WM_FW_EXP_FWY (-260)

typedef struct {
    int32_t x;
    int32_t y;
} wm_fw_point_t;

/*
 * The twenty-two flares do_fireworks lights: twelve across the back
 * of the ring from a `dsjs` loop stepping 50 pixels, then five down
 * the left side and five down the right, written out one at a time.
 */
#define WM_FW_FLARES 22
extern const wm_fw_point_t wm_fw_flare_positions[WM_FW_FLARES];

/*
 * `#flare_anim` -- thirteen images, and `#flare_anim2` is a label
 * five images INTO it rather than a table of its own. That is not a
 * detail: the flare plays all thirteen once, then loops the last
 * eight forever, and fizzles by walking five of them backwards.
 */
extern const char *const wm_fw_flare_anim[];
extern const size_t wm_fw_flare_anim_count;
#define WM_FW_FLARE_ANIM2 5

/* `#fwexa_anim` / `#fwexb_anim`, picked between at random. */
extern const char *const wm_fw_fwexa_anim[];
extern const size_t wm_fw_fwexa_anim_count;
extern const char *const wm_fw_fwexb_anim[];
extern const size_t wm_fw_fwexb_anim_count;

/* `#fw_pals`. */
#define WM_FW_PALETTES 5
extern const char *const wm_fw_palettes[WM_FW_PALETTES];

/*
 * The congratulations text. Three sets, chosen by what the player
 * just won, each pairing a JAM_STR placement with a string. The
 * source keeps those in two parallel tables of which only the
 * placement one is terminated, and they are different lengths per
 * set; the generator checks they line up rather than assuming it.
 */
typedef struct {
    const char *text;
    int32_t x;
    int32_t y;
    const char *font;
    const char *palette;
    /* The source's own JAM_STR label, so a row traces back. */
    const char *setup;
} wm_fw_congrat_line_t;

typedef struct {
    const char *name;
    const wm_fw_congrat_line_t *lines;
    size_t count;
} wm_fw_congrats_t;

/*
 * `#congrats_setup_tbl`, indexed by the a14 print_congrats is called
 * with: 0 = beat the three-man ladder, 1 = beat all eight alone,
 * 2 = beat all eight as a tag team.
 */
#define WM_FW_CONGRATS_SETS 3
#define WM_FW_CONGRATS_1V3 0
#define WM_FW_CONGRATS_1V8 1
#define WM_FW_CONGRATS_2V8 2
extern const wm_fw_congrats_t wm_fw_congrats[WM_FW_CONGRATS_SETS];

/*
 * `pan_around`'s three-way choice, written out. The source reads
 * @royal_rumble first and only calls is_8_on_1 if it is clear.
 */
int wm_fw_congrats_index(bool royal_rumble, bool is_8_on_1);

/* ---- the camera (calc_dxdy, move_camera, check_camera_position) -- */

/*
 * WORLDTLX and WORLDTLY are 16.16; targ_x and targ_y are the integer
 * halves of the current waypoint, kept separately because
 * check_camera_position compares in whole pixels.
 */
typedef struct {
    int32_t worldtlx;
    int32_t worldtly;
    int32_t dx;
    int32_t dy;
    int32_t ticks;
    int32_t p1x, p1y;      /* segment start */
    int32_t p2x, p2y;      /* segment end */
    int32_t final_x, final_y;   /* where the pan started, to return to */
    int16_t targ_x, targ_y;
} wm_fw_camera_t;

/*
 * `calc_dxdy`: (p2 - p1) / ticks, per axis, SIGNED divide. p1 is the
 * last waypoint rather than the camera's actual position, and the
 * camera is allowed to stop up to three pixels short of a waypoint
 * (below), so the two drift apart by a little over the flight. The
 * source does not correct for it.
 */
void wm_fw_calc_dxdy(wm_fw_camera_t *c);

/* `move_camera`: WORLDTLX += dx, WORLDTLY += dy. That is all of it. */
void wm_fw_move_camera(wm_fw_camera_t *c);

/*
 * `check_camera_position`: each axis independently, if
 * |(WORLDTL >> 16) - targ| is NOT greater than 3, that axis's delta is
 * zeroed. `sra 16` is an arithmetic shift, so a negative world
 * position floors rather than truncating toward zero -- which matters,
 * because every Y in the table is negative.
 */
void wm_fw_check_camera_position(wm_fw_camera_t *c);

/* The loop's own exit test: `move @dx,a14 / move @dy,a0 / or a0,a14 /
   jrnz`. A segment is over when BOTH deltas have been zeroed. */
bool wm_fw_segment_done(const wm_fw_camera_t *c);

/* ---- pan_around -------------------------------------------------- */

typedef enum {
    /* `SLEEP TSEC/2` before anything. */
    WM_FW_PAN_START = 0,
    /* Flying one segment of the figure of eight. */
    WM_FW_PAN_SEGMENT,
    /* `SLEEPK 5` before the text goes up, once, mid-flight. */
    WM_FW_PAN_TEXT_DELAY,
    /* `SLEEPK 2` after print_congrats. */
    WM_FW_PAN_TEXT_HOLD,
    /* Flying back to where the camera started. */
    WM_FW_PAN_BACK,
    /* `DIE`. */
    WM_FW_PAN_DONE
} wm_fw_pan_phase_t;

typedef struct {
    wm_fw_pan_phase_t phase;
    wm_fw_camera_t cam;
    int32_t timer;
    size_t waypoint;        /* index into wm_fw_panning_points */
    /* `@pan_status` -- the text goes up once, at the end of the first
       segment, and never again however many laps are flown. */
    bool text_shown;
    /* `@pan_down` -- set by do_fireworks to end the flight. Read at
       the END of a segment, so the current segment always finishes. */
    bool pan_down;
    /* Laps completed. `jrz #pan_again` restarts the table. */
    unsigned laps;

    /* Raised for the one tick each is due on. */
    bool update_background;   /* `calla BGND_UD1` */
    bool cheer;               /* `calla crowd_cheer` with a3 = 1 */
    /* `callr print_congrats` is due this tick; congrats_set says
       which of the three message sets it should put up. */
    bool show_congrats;
    bool delete_text;         /* `obj_del1c CLSMK3` on the way down */
    int congrats_set;         /* which of the three, once chosen */
} wm_fw_pan_t;

/*
 * `x0`/`y0` are WORLDTLX/WORLDTLY as the camera sits at the end of
 * the match: pan_around saves them to @final_x/@final_y and pans back
 * to them before it dies.
 */
void wm_fw_pan_begin(wm_fw_pan_t *p, int32_t x0, int32_t y0);

/*
 * One tick. `royal_rumble` and `is_8_on_1` are only read on the tick
 * the text is chosen. Returns true while the pan is still flying.
 */
bool wm_fw_pan_tick(wm_fw_pan_t *p, bool royal_rumble, bool is_8_on_1);

/* `move a0,@pan_down` -- do_fireworks tells the pan to come home. */
void wm_fw_pan_come_down(wm_fw_pan_t *p);

/* ---- firework_flare ---------------------------------------------- */

typedef enum {
    /* `movk 25,a0 / RNDRNG0 / addk 1 / PRCSLP` -- 1 to 26 ticks. */
    WM_FW_FLARE_DELAY = 0,
    /* The first pass: all thirteen images, two ticks each. */
    WM_FW_FLARE_RUN,
    /* Looping `#flare_anim2` -- the last eight -- until told to stop. */
    WM_FW_FLARE_LOOP,
    /* `#flare_fizzle`: five images backwards, four ticks each. */
    WM_FW_FLARE_FIZZLE,
    WM_FW_FLARE_DEAD
} wm_fw_flare_phase_t;

typedef struct {
    wm_fw_flare_phase_t phase;
    int32_t timer;
    /* The index into wm_fw_flare_anim the next image comes from. */
    int32_t at;
    int32_t palette;        /* 0..4 into wm_fw_palettes */
    int32_t x, y;
    /* Raised on the tick the image changes; `frame` is the new one. */
    bool changed;
    int32_t frame;
    bool created;
} wm_fw_flare_t;

/* `movk 25,a0 / RNDRNG0`: the maximum is INCLUSIVE, so the delay is
   1..26 ticks and the palette index is 0..4. */
#define WM_FW_FLARE_DELAY_MAX 26
#define WM_FW_FLARE_STEP_TICKS 2
#define WM_FW_FLARE_FIZZLE_TICKS 4

/*
 * `pos` is a9 as do_fireworks packs it. `delay` must be 1..26 and
 * `palette` 0..4 -- the two RNDRNG0 draws, passed in rather than
 * drawn here so a test can pin them.
 */
void wm_fw_flare_begin(wm_fw_flare_t *f, wm_fw_point_t pos,
                       int32_t delay, int32_t palette);

/* `fizzle` is @fizzle_flare, read at the END of a pass. */
bool wm_fw_flare_tick(wm_fw_flare_t *f, bool fizzle);

/* ---- animate_fwexp ----------------------------------------------- */

typedef struct {
    int32_t x, y, z;
    int32_t which;          /* 0 or 1: #fwexa_anim or #fwexb_anim */
    int32_t palette;
    int32_t at;
    bool changed;
    bool created;
    bool dead;
} wm_fw_explosion_t;

/*
 * The four RNDRNG0 draws, in the order animate_fwexp makes them:
 * a vertical position in 0..96, a horizontal in 0..350, a Z in
 * 0..0x200, and which of the two explosions. The offsets the source
 * adds to each are applied here.
 */
void wm_fw_explosion_begin(wm_fw_explosion_t *e, int32_t rnd_y,
                           int32_t rnd_x, int32_t rnd_z, int32_t which,
                           int32_t palette);

/* `SLEEPK 1` then the next image, and `addk 4,a9` -- the explosion
   DRIFTS DOWN four pixels per frame as it plays. */
bool wm_fw_explosion_tick(wm_fw_explosion_t *e);
#define WM_FW_EXPLOSION_DRIFT 4

/* ---- do_fireworks ------------------------------------------------ */

/*
 * `knockout_drones`: every process in process_ptrs whose PLYR_TYPE is
 * non-zero gets PTIME = 7FFFh. A drone is not killed, it is put to
 * sleep for about ten minutes -- which is why the ring still has
 * bodies in it while the camera flies over.
 */
#define WM_FW_DRONE_SLEEP 0x7fff

typedef struct {
    /* PLYR.EQU:263 PLYR_TYPE. Zero is a human and is left alone. */
    int32_t plyr_type;
    /* MPROC's PTIME, the process's sleep countdown. */
    int32_t ptime;
} wm_fw_process_t;

/*
 * The walk stops at the first null entry of process_ptrs (`move
 * *a9+,a8,L / jrz #ko_done`), so `count` is the array length and
 * `live` is how many of them the caller has filled in. Returns how
 * many were put to sleep.
 */
size_t wm_fw_knockout_drones(wm_fw_process_t *procs, size_t live);

/* The explosion budget: `movi TSEC*6,a8`, then each explosion costs a
   random 1..7 ticks off it and the loop runs while it is positive. */
#define WM_FW_EXPLOSION_BUDGET (53 * 6)
#define WM_FW_EXPLOSION_GAP_MAX 7

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_FIREWORK_H */
