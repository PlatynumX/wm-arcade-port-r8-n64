#ifndef WM_ANIM_PUPPET_H
#define WM_ANIM_PUPPET_H

#include <stddef.h>
#include <stdint.h>

/*
 * ANIM.ASM:2681 ANI_SUPERSLAVE2's per-wrestler defender-frame tables.
 *
 * A grapple, slam, suplex or throw is ONE animation driving BOTH wrestlers.
 * The attacker's routine names its own frame inline and then looks the
 * defender's up in a table indexed by the defender's WRESTLERNUM -- which
 * is why the same throw shows a different victim pose depending on who is
 * being thrown.
 *
 * `xoff`/`yoff` are the raw table values. The runtime adjusts them by both
 * frames' own animation origins and widths before they mean anything on
 * screen, exactly as the source's get_mpart_offsets / get_mpart_xsize do.
 */
typedef struct {
    const char *frame;
    int16_t xoff, yoff, flip;
} wm_anim_puppet_row;

typedef struct {
    const wm_anim_puppet_row *rows;
    size_t count;
} wm_anim_puppet_slot;

typedef struct {
    /* "FILE.ASM:line" of the table's own definition. `#puppet_tbl` is a
       local label the files reuse freely -- HRTSEQ3.ASM alone defines it
       eight times -- so the line is what identifies it, not the name. */
    const char *source;
    const wm_anim_puppet_slot *slots;   /* nine, in WRESTLERNUM order */
} wm_anim_puppet_table;

const wm_anim_puppet_table *wm_anim_puppet_table_at(size_t id);
size_t wm_anim_puppet_table_count(void);

/* The defender's row: table `id`, wrestler `wrestler_num`, row `index`.
   NULL when any of those is out of range. */
const wm_anim_puppet_row *wm_anim_puppet_row_at(size_t id,
                                                int32_t wrestler_num,
                                                size_t index);

/*
 * ANIM.ASM:2130 ANI_SLAVEANIM's tables: nine per-wrestler ANIMATION labels
 * rather than frames. A slam does not pose the victim, it makes him run his
 * own landing animation -- so the entry is the name of a routine, which is
 * exactly what the program registry is keyed on. NULL where the source has
 * a real zero (the spare roster slot, mostly).
 */
const char *wm_anim_slave_label(size_t id, int32_t wrestler_num);
size_t wm_anim_slave_table_count(void);

#endif
