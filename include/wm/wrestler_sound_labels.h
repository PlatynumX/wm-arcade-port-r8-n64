#ifndef WM_WRESTLER_SOUND_LABELS_H
#define WM_WRESTLER_SOUND_LABELS_H

/*
 * The names the per-wrestler dispatchers pass to their `sound_label`
 * seam, resolved to what the source actually plays.
 *
 * WHY THIS EXISTS AT ALL. Six of the eight dispatchers call sound by a
 * STRING -- `snd(a,"GRABFLING",c)` -- because that is how they were
 * transcribed: SOUND.H's `WRSND W_DOINK,GRABFLING_T1,GRABFLING_T2` names
 * its move in the mnemonic and the transcription kept the mnemonic. The
 * seam behind it, wm_arcade_roster_callbacks_t::sound_label, was
 * declared once and assigned by nobody, so every one of those calls --
 * fifty across the six files -- was dropped on the floor.
 *
 * Two shapes come through it, because two shapes go into the source.
 * Most are WRSND, a per-wrestler pair of move indexes looked up through
 * MASTER_SOUND_TABLE (wm/wrestler_sound_tables.h). BLOCK_WOOSH is not:
 * DCSSOUND.ASM:4265 is `MOVI 16h,A0 / CALLA triple_sound`, one fixed
 * call for everybody.
 *
 * An unknown label resolves to WM_SNDLABEL_UNKNOWN rather than to
 * silence, and a test walks every label the dispatchers actually use and
 * refuses one that does not resolve. That is deliberate: a string seam
 * that quietly plays nothing for a name nobody checked is how
 * `snd(a,"SPIRIT",c)` survived -- SOUND.H has no SPIRIT, and the source
 * at TAKER.ASM:404 plays `WRSND W_TAKER,GRABHOLD_T1,GRABHOLD_T2`.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WM_SNDLABEL_UNKNOWN = 0,
    /* SOUND.H's WRSND: a per-wrestler pair, through wm_wrsndx. */
    WM_SNDLABEL_WRSND,
    /* A plain triple_sound of one fixed call, for everybody. */
    WM_SNDLABEL_FIXED
} wm_sndlabel_kind_t;

typedef struct {
    wm_sndlabel_kind_t kind;
    /* WRSND: the two move indexes, SOUND.H:59-118. move2 is -1 for the
       one-sound form, which is what a label naming _T1 outright asks
       for. */
    int move1;
    int move2;
    /* FIXED: the triple_sound call. */
    uint16_t call;
} wm_sndlabel_t;

wm_sndlabel_t wm_wrsnd_label(const char *label);

/* Every label this table knows, for a test to walk. */
const char *const *wm_wrsnd_label_names(size_t *count);

#ifdef __cplusplus
}
#endif
#endif
