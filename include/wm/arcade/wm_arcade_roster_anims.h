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

typedef struct wm_roster_anim_table {
    const char *name;           /* the source label */
    const char *file;           /* where it lives */
    int line;
    int slots;                  /* 9 or 10 -- how many rows it declared */
    const char *const *row;     /* [WM_ROSTER_ANIM_SLOTS], NULL where empty */
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

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_ROSTER_ANIMS_H */
