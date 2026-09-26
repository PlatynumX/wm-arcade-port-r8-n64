#ifndef WM_ANNOUNCE_TABLES_H
#define WM_ANNOUNCE_TABLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wm/arcade/wm_arcade_announcer.h"
#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DCSSOUND.ASM:2911 ADD_TO_QUEUE, and the line tables it picks from.
 *
 * The queue in wm/arcade/wm_arcade_announcer.h is only the back half of
 * the announcer. The front half is this: a table of line ids with its
 * shape written in three values immediately before it, a percentage gate,
 * a random row, and an anti-repeat check that walks forward when the row
 * it drew was said too recently.
 *
 * The tables and every number in them come from tools/wlvoice.py reading
 * DCSSOUND.ASM; none of it is written by hand.
 */

/* SOUND.EQU:57-63. A row carrying one of these is not a line id -- it
   says "work the real one out first", which SET_UP_PERSONAL_CALL does
   from the attacking wrestler's number (or, for REPEAT_MODE, from a
   counter that climbs while the repeats keep coming). */
#define WM_ANN_GIVE_CREDIT          (-1)
#define WM_ANN_VERY_IMPRESSIVE      (-2)
#define WM_ANN_END_GAME_STUFF       (-3)
#define WM_ANN_IT_DOESNT_LOOK_GOOD  (-4)
#define WM_ANN_R_IMPRESSIVE_MOVE    (-5)
#define WM_ANN_GIDDUP_MODE          (-6)
#define WM_ANN_REPEAT_MODE          (-7)

/* The five per-wrestler tables SET_UP_PERSONAL_CALL chooses between, in
   the order it tests for them, and the nine WRESTLERNUM slots each has. */
#define WM_ANNOUNCE_PERSONAL_KINDS 5
#define WM_ANNOUNCE_WRESTLERS 9
/* REPEAT_STATE counts 3, 2, 1, 0 through ASCENDING_TABLE's four lines. */
#define WM_ANNOUNCE_REPEAT_STEPS 4
/* REPEAT_DUMMY's `MOVI 80,A9 / PRCSLP`, reset by SET_DUMMY_SLEEP. */
#define WM_ANNOUNCE_REPEAT_TICKS 80
/* DO_END_STUFF's `CMPI 40,A0 / JRLT` -- anybody this hurt makes the
   announcer switch to the end-of-match lines. */
#define WM_ANNOUNCE_END_GAME_HEALTH 40

typedef struct {
    const char *name;
    const int16_t *rows;      /* row-major, `stride` words per row */
    size_t word_count;        /* including the walk-forward padding */
    uint16_t last_index;      /* RNDRNG0's inclusive maximum */
    uint8_t stride;           /* words per row: 1, or 2 for a paired line */
    bool reset_repeat;        /* the `.WORD -1` at -050H */
    /* The `.LONG` at -040H: the crowd table DO_CROWD_ANYWAY draws from
       before the line is queued, or NULL for a table that leaves the
       crowd alone. */
    const char *crowd;
} wm_announce_table;

/*
 * GAME.EQU:299-306. crowd_cheer's A3, one bit each.
 *
 * C_SHORT is a literal 0 -- the absence of WM_CROWD_LONG -- so it has no
 * name here.
 */
#define WM_CROWD_LONG     1   /* B_L_OR_S: the long animation */
#define WM_CROWD_OVERRIDE 2   /* B_OVERRIDE: interrupt whatever is running */
#define WM_CROWD_RANDOM   4   /* B_RANDOM: only `percent` of the crowd */

/*
 * DCSSOUND.ASM:4443's crowd tables. DO_CROWD_ANYWAY picks one row with
 * RNDRNG0 over `last_index` (inclusive), plays `sound` for `ticks` unless
 * a crowd sound is already running, and then calls crowd_cheer with
 * `flags` -- passing `percent` only when C_RANDOM is set.
 */
typedef struct {
    int16_t sound;            /* SOUND.EQU CROWD_* id */
    int16_t ticks;            /* SOUND.EQU D_CROWD_* duration */
    int16_t flags;            /* WM_CROWD_* */
    int16_t percent;          /* RNDPER, per mille, when WM_CROWD_RANDOM */
} wm_crowd_row;

typedef struct {
    const char *name;
    const wm_crowd_row *rows;
    size_t row_count;
    uint16_t last_index;      /* RNDRNG0's inclusive maximum */
} wm_crowd_table;

extern const wm_crowd_table wm_crowd_tables[];
extern const size_t wm_crowd_table_count;
const wm_crowd_table *wm_crowd_table_find(const char *name);

typedef struct {
    const char *name;         /* CALL_MISSES, ... */
    const char *table;
    uint16_t sleep;           /* the CREATEd process's own SLEEP */
    uint16_t percent;         /* RNDPER, per mille */
    bool personal;            /* the process copies WRESTLERNUM into A5 */
} wm_announce_call;

extern const wm_announce_table wm_announce_tables[];
extern const size_t wm_announce_table_count;
extern const wm_announce_call wm_announce_calls[];
extern const size_t wm_announce_call_count;
extern const int16_t
    wm_announce_personal[WM_ANNOUNCE_PERSONAL_KINDS][WM_ANNOUNCE_WRESTLERS];
extern const int16_t
    wm_announce_ascending[WM_ANNOUNCE_WRESTLERS][WM_ANNOUNCE_REPEAT_STEPS];

/*
 * DCSSOUND.ASM:3793 CALL_MATCH_OVER. Unlike the CALL_x family this is a
 * decision tree rather than one table and one percentage, so its
 * constants are carried here instead of in a wm_announce_call row.
 */
typedef struct {
    uint16_t sleep;               /* the process's own SLEEPK */
    uint16_t speech_percent;      /* -> WRESTLER_SPEECH instead */
    uint16_t streak_percent;      /* -> the over-four-wins line */
    uint16_t queue_percent;       /* the ordinary ADD_TO_QUEUE */
    uint16_t which_special_percent;   /* which of the two special lines */
    uint16_t winstreak_min;       /* `CMPI 4,A0 / JRLT` */
    int16_t special_line[2];      /* CAN_ANYBODY_STOP_HIM / L_NO_ONE_CAN_TOUCH */
} wm_announce_match_over_cfg;

extern const wm_announce_match_over_cfg wm_announce_match_over;
/* WRESTLER_SPEECH's WHICH_WRESTLER_TALKS, by WRESTLERNUM. NULL for the
   cut slot; the Undertaker's and Yokozuna's tables hold a single 0, so
   they win in silence. */
extern const char *const wm_announce_finishes[WM_ANNOUNCE_WRESTLERS];

const wm_announce_table *wm_announce_table_find(const char *name);
const wm_announce_call *wm_announce_call_find(const char *name);
/* True for a table some CALL_x draws from, as opposed to the
   end-of-match family PROC_MATCH_OVER reaches directly. */
bool wm_announce_drawn_by_a_call(const char *name);

/*
 * What ADD_TO_QUEUE reaches for beyond the queue itself. Every field may
 * be absent; a service that is missing makes the routine decline rather
 * than invent a draw or a health reading.
 */
typedef struct {
    WmRng *rng;               /* UTIL.ASM RNDPER / RNDRNG0 */
    int wrestler_num;         /* A5, for the personal calls */
    /* DO_END_STUFF walks every wrestler with get_health looking for one
       under 40. NULL means "cannot tell", and END_GAME_STUFF then falls
       through to the walk-forward exactly as a healthy roster does. */
    bool (*anyone_near_death)(void *user);
    void *user;
    /*
     * DCSSOUND.ASM:3046 DO_CROWD_ANYWAY, run when the drawn table carries
     * a crowd `.LONG`. Both halves are optional: `crowd_sound` is the
     * SNDSND of the picked row (the source guards it with
     * crowd_dummy_exists, so pass `crowd_busy` to say one is already
     * running), and `crowd_cheer` is CROWD.ASM's crowd_cheer itself.
     */
    bool crowd_busy;
    void *crowd_user;
    void (*crowd_sound)(void *user, int sound, int ticks);
    void (*crowd_cheer)(void *user, int flags, int percent);
} wm_announce_ctx;

/*
 * ADD_TO_QUEUE (`if_silent` false) and ADD_IF_SILENT (true).
 *
 * Returns the number of lines queued: 0 when the RNDPER gate refused, when
 * every row it could reach was said too recently, or when the queue itself
 * declined; 1 normally, and more for a table whose rows carry several
 * lines at once.
 */
int wm_announce_from_table(wm_announcer_state *a, const wm_announce_table *t,
                           uint16_t percent, bool if_silent,
                           const wm_announce_ctx *ctx);

/*
 * PROC_MATCH_OVER, run as one call. `loser_was_drone` is the source's
 * `xor a8,a9 / jrz #drn_l` -- the winning team's bit against PSTATUS --
 * and picks MATCH_OVER_DL over MATCH_OVER. `win_streak` is the winner's
 * own p1winstreak entry. Returns the number of lines queued.
 *
 * Note it uses ADD_TO_QUEUE at 1000 per mille, not ADD_IF_SILENT: the end
 * of a match always says something, over whatever else is being said.
 */
int wm_announce_match_over_run(wm_announcer_state *a, bool loser_was_drone,
                               int wrestler_num, int win_streak,
                               const wm_announce_ctx *ctx);

/* One tick of REPEAT_DUMMY: the 80-tick life of the repeat counter. */
void wm_announce_tick_repeat(wm_announcer_state *a);

#ifdef __cplusplus
}
#endif
#endif
