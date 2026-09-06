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
} wm_announce_table;

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

const wm_announce_table *wm_announce_table_find(const char *name);
const wm_announce_call *wm_announce_call_find(const char *name);

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

/* One tick of REPEAT_DUMMY: the 80-tick life of the repeat counter. */
void wm_announce_tick_repeat(wm_announcer_state *a);

#ifdef __cplusplus
}
#endif
#endif
