#include "wm/roster.h"

static const wm_wrestler_def roster[WM_WRESTLER_COUNT] = {
    [WM_WRESTLER_BRET]       = {"Bret Hart",       "BRET.ASM",  "HRTSEQ", "hrt", true},
    [WM_WRESTLER_BAM_BAM]    = {"Bam Bam Bigelow", "BAM.ASM",   "BAMSEQ", "bam", false},
    [WM_WRESTLER_YOKOZUNA]   = {"Yokozuna",        "YOKO.ASM",  "YOKSEQ", "yok", false},
    [WM_WRESTLER_DOINK]      = {"Doink",           "DOINK.ASM", "DNKSEQ", "dnk", false},
    [WM_WRESTLER_RAZOR]      = {"Razor Ramon",     "RAZOR.ASM", "RZRSEQ", "rzr", false},
    [WM_WRESTLER_LEX]        = {"Lex Luger",       "LEX.ASM",   "LEXSEQ", "lex", false},
    [WM_WRESTLER_UNDERTAKER] = {"Undertaker",      "TAKER.ASM", "UNDSEQ", "und", false},
    [WM_WRESTLER_SHAWN]      = {"Shawn Michaels",  "SHAWN.ASM", "SHNSEQ", "shn", false},
};

const wm_wrestler_def *wm_roster_get(wm_wrestler_id id) {
    if ((unsigned)id >= WM_WRESTLER_COUNT) return 0;
    return &roster[id];
}

size_t wm_roster_count(void) {
    return WM_WRESTLER_COUNT;
}
