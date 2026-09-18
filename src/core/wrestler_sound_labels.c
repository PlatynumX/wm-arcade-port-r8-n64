/*
 * SOUND.H's move mnemonics, as the dispatchers spell them.
 * See wm/wrestler_sound_labels.h.
 */
#include "wm/wrestler_sound_labels.h"

#include <string.h>

/* SOUND.H:59-118. Each WRSND row is the _T1 index; _T2 is _T1 + 1,
   which is the layout the source's own comment describes -- "the first
   two are the noises a wrestler makes when he throws the move, and the
   second two are the noises he makes when he's hit with the move". */
typedef struct {
    const char *name;
    wm_sndlabel_kind_t kind;
    int move1;
    int move2;
    uint16_t call;
} row;

static const row TABLE[] = {
    /* The throw pairs, by their SOUND.H names. */
    { "PUNCH",       WM_SNDLABEL_WRSND,  0,  1, 0 },
    { "HDBUTT",      WM_SNDLABEL_WRSND,  4,  5, 0 },
    { "KICK",        WM_SNDLABEL_WRSND,  8,  9, 0 },
    { "FLYKICK",     WM_SNDLABEL_WRSND, 12, 13, 0 },
    { "GRABTHROW",   WM_SNDLABEL_WRSND, 16, 17, 0 },
    { "UPRCUT",      WM_SNDLABEL_WRSND, 20, 21, 0 },
    { "LBOWDROP",    WM_SNDLABEL_WRSND, 24, 25, 0 },
    { "GRABHOLD",    WM_SNDLABEL_WRSND, 28, 29, 0 },
    { "GRABFLING",   WM_SNDLABEL_WRSND, 32, 33, 0 },
    { "PUSH",        WM_SNDLABEL_WRSND, 36, 37, 0 },
    { "HIPTOSS",     WM_SNDLABEL_WRSND, 40, 41, 0 },
    { "SPUNCH",      WM_SNDLABEL_WRSND, 48, 49, 0 },
    { "TURNDIVE",    WM_SNDLABEL_WRSND, 52, 53, 0 },

    /* The one-sound forms, where a call site names _T1 outright. */
    { "PUNCH_T1",    WM_SNDLABEL_WRSND,  0, -1, 0 },
    { "HDBUTT_T1",   WM_SNDLABEL_WRSND,  4, -1, 0 },
    { "KICK_T1",     WM_SNDLABEL_WRSND,  8, -1, 0 },
    { "FLYKICK_T1",  WM_SNDLABEL_WRSND, 12, -1, 0 },
    { "GRABHOLD_T1", WM_SNDLABEL_WRSND, 28, -1, 0 },
    { "GRABFLING_T1",WM_SNDLABEL_WRSND, 32, -1, 0 },
    { "SPUNCH_T1",   WM_SNDLABEL_WRSND, 48, -1, 0 },

    { "UPRCUT_T2",   WM_SNDLABEL_WRSND, 21, -1, 0 },
    { "KICK_T2",     WM_SNDLABEL_WRSND,  9, -1, 0 },

    /*
     * The MIXED pairs, which are why this is a table and not a formula.
     * A WRSND's two moves need not come from the same family: Razor
     * grabs and then lands a punch (`WRSND W_RAZOR,GRABFLING_T1,
     * PUNCH_T2`, RAZOR.ASM:383 and four more), and Bret does the same
     * from a hip toss (`WRSND W_BRET,HIPTOSS_T1,PUNCH_T2`).
     */
    { "GRABFLING_PUNCH", WM_SNDLABEL_WRSND, 32, 1, 0 },
    { "HIPTOSS_PUNCH",   WM_SNDLABEL_WRSND, 40, 1, 0 },

    /* DCSSOUND.ASM:4265 `MOVI 16h,A0 / CALLA triple_sound` -- not a
       WRSND, and the same woosh for every wrestler. */
    { "BLOCK_WOOSH", WM_SNDLABEL_FIXED,  0,  0, 0x16u },
};

#define TABLE_COUNT (sizeof TABLE / sizeof TABLE[0])

wm_sndlabel_t wm_wrsnd_label(const char *label) {
    wm_sndlabel_t out;
    size_t i;

    out.kind = WM_SNDLABEL_UNKNOWN;
    out.move1 = out.move2 = -1;
    out.call = 0u;
    if (!label) return out;

    for (i = 0; i < TABLE_COUNT; ++i) {
        if (strcmp(TABLE[i].name, label) != 0) continue;
        out.kind = TABLE[i].kind;
        out.move1 = TABLE[i].move1;
        out.move2 = TABLE[i].move2;
        out.call = TABLE[i].call;
        return out;
    }
    return out;
}

const char *const *wm_wrsnd_label_names(size_t *count) {
    static const char *names[TABLE_COUNT];
    size_t i;
    for (i = 0; i < TABLE_COUNT; ++i) names[i] = TABLE[i].name;
    if (count) *count = TABLE_COUNT;
    return names;
}
