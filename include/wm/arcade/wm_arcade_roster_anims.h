/*
 * The game's global per-wrestler animation tables.
 *
 * A lot of shared behaviour is "everyone does this, but each in his own
 * animation", and the source spells that as a flat table indexed by
 * WRESTLERNUM. Falling back, being hit on the ground, hitting a block,
 * climbing in and through the ropes -- one table each, nine or ten rows.
 *
 * Slot 7 is Adam Bomb, cut from the game. Slot 9 is the Referee, and it
 * only exists in the tables that declared ten rows -- `slots` says which.
 * A NULL entry is a `.long 0`: that wrestler has no animation for this at
 * all, and the caller has to cope rather than substitute one.
 *
 * Two things worth knowing, both read out of the data rather than
 * derived from the naming:
 *
 *   The six climb tables give Adam Bomb and the Referee *Doink's*
 *   animation rather than nothing, so those two do climb.
 *
 *   fall_back_tbukl_tbl -- the turnbuckle variant -- points Yokozuna at
 *   `yok_fall_back_anim`, the ordinary one, where every other wrestler
 *   gets his own `*_fall_back_tbukl_anim`. He has no turnbuckle fall of
 *   his own.
 *
 * Two macros index these, and they disagree about the row width.
 * FACETBL (MACROS.H:102) scales WRESTLERNUM by X32 -- one long per
 * wrestler. FACE24TBL (MACROS.H:65) scales by X64 -- two longs -- and
 * adds a long when MOVE_UP_BIT is *clear*, so column 0 is the facing-up
 * animation (the `_2_` name) and column 1 the facing-down one (`_4_`).
 *
 * A two-long row is not automatically a facing pair. PROGRESS.ASM's six
 * `*_addr` tables have the same shape, but the loop at PROGRESS.ASM:3218
 * reads their two longs as a leg animation and a torso animation and
 * hands them to change_anim1a and change_anim2a in turn. `columns` says
 * how wide a row is and `kind` says what the columns mean; the meaning
 * is read off the use site, never off the shape.
 *
 * Tables named as command operands (ANI_SLAVEANIM, ANI_CHANGEANIM_TBL)
 * are not here: those labels are `#local` and reused across files, so
 * they can only be resolved from a use site, which tools/wlpuppet.py
 * does. These are the plain globals, one definition each.
 */
#ifndef WM_ARCADE_ROSTER_ANIMS_H
#define WM_ARCADE_ROSTER_ANIMS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 0..8 as usual, plus the Referee at 9. */
#define WM_ROSTER_ANIM_SLOTS 10
/* The cut wrestler, and the one only some tables carry. */
#define WM_ROSTER_ANIM_ADAM_BOMB 7
#define WM_ROSTER_ANIM_REFEREE 9

typedef enum wm_roster_col_kind {
    WM_ROSTER_COL_SLOT = 0,   /* one long per wrestler */
    WM_ROSTER_COL_FACING,     /* [0] facing up (_2_), [1] facing down (_4_) */
    WM_ROSTER_COL_PAIR        /* two longs the use site gives meaning to */
} wm_roster_col_kind;

typedef struct wm_roster_anim_table {
    const char *name;           /* the source label */
    const char *file;           /* where it lives */
    int line;
    int slots;                  /* 9 or 10 -- how many rows it declared */
    int columns;                /* 1 or 2 longs per row */
    wm_roster_col_kind kind;
    const char *const *row;     /* [slots * columns], NULL where empty */
} wm_roster_anim_table;

extern const wm_roster_anim_table wm_roster_anim_tables[];
extern const int wm_roster_anim_table_count;

/* Look a table up by its source label, or NULL. */
const wm_roster_anim_table *wm_roster_anim_find(const char *name);

/*
 * The animation label for one wrestler, or NULL when the table has no
 * entry for him -- which includes asking a nine-row table for the
 * Referee.
 */
const char *wm_roster_anim_for(const wm_roster_anim_table *table, int wrestler);

/*
 * One cell of a multi-column table. `column` is clamped to the table's
 * width, so asking a one-column table for column 1 gives its only
 * entry rather than reading off the end.
 */
const char *wm_roster_anim_col(const wm_roster_anim_table *table,
                               int wrestler, int column);

/*
 * What FACE24TBL does: column 0 when MOVE_UP_BIT is set in FACING_DIR,
 * column 1 when it is clear. Only meaningful for WM_ROSTER_COL_FACING
 * tables; for the others this is wm_roster_anim_col(t, wrestler, 0).
 */
const char *wm_roster_anim_facing(const wm_roster_anim_table *table,
                                  int wrestler, int facing_up);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_ROSTER_ANIMS_H */
