#include "wm/arcade/wmania_attract_data.h"

/*
 * WHICH_HINT, ATTRACT.ASM:3625, in its own order -- which is the on-screen
 * order, because `last_hint` steps the index -- and NOT the #HNT_n numbering.
 *
 * number_image_index is the index again: DO_HINTS reaches WHICH_22_NUM with
 * `PULLP A2 / SLL 5,A2 / ADDI WHICH_22_NUM,A2` (:3471), the same hint number
 * it used for WHICH_HINT. WHICH_22_NUM (:3613) runs WGSF22_1..WGSF22_9 then
 * WGSF22_0, so index 9 shows the "0" glyph.
 *
 * Hint 9 reuses hint 4's tip-name object (EUGTIP) and hint 3's mugshot
 * (JSHMUG); that pairing is the source's, not a transcription slip.
 */
const WmAttractHint wm_attract_hints[WM_ATTRACT_ACTIVE_HINTS] = {
    { "HNTT_2", "HNT_2", "JMSTIP", "JASMUG", 0u },
    { "HNTT_4", "HNT_4", "MIKTIP", "MIKMUG", 1u },
    { "HNTT_3", "HNT_3", "MJTTIP", "MRKMUG", 2u },
    { "HNTT_9", "HNT_9", "JOSTIP", "JSHMUG", 3u },
    { "HNTT_7", "HNT_7", "EUGTIP", "EUGMUG", 4u },
    { "HNTT_5", "HNT_5", "SHNTIP", "SHNMUG", 5u },
    { "HNTT_8", "HNT_8", "JAKTIP", "JAKMUG", 6u },
    { "HNTT_1", "HNT_1", "SALTIP", "SALMUG", 7u },
    { "HNTT_6", "HNT_6", "TONTIP", "TONMUG", 8u },
    { "HNTT_A", "HNT_A", "EUGTIP", "JSHMUG", 9u }
};

const WmAttractBio wm_attract_bios[WM_ATTRACT_WRESTLERS] = {
    { "Bret Hart",       79u, "bhart_fromstr", 234u, 6u,  1u,
      "bhart_quote", "HRT3", 15u, 9u, 5u, "bret_tips" },
    { "Razor Ramon",     77u, "razor_fromstr", 262u, 6u,  7u,
      "razor_quote", "RZR3", 16u, 9u, 2u, "razor_tips" },
    { "Undertaker",      78u, "taker_fromstr", 322u, 6u, 11u,
      "taker_quote", "UND3", 17u, 9u, 1u, "taker_tips" },
    { "Yokozuna",        78u, "yoko_fromstr", 568u, 6u,  4u,
      "yoko_quote", "YOK3", 20u, 7u, 7u, "yoko_tips" },
    { "Shawn Michaels",  78u, "shawn_fromstr", 235u, 6u,  1u,
      "shawn_quote", "SHN3", 18u, 8u, 6u, "shawn_tips" },
    { "Bam Bam Bigelow", 78u, "bambam_fromstr",400u, 6u,  4u,
      "bambam_quote", "BAM3", 18u, 7u, 4u, "bambam_tips" },
    { "Doink",            78u, "doink_fromstr",243u, 6u,  0u,
      "doink_quote", "DNK3", 24u, 8u, 8u, "doink_tips" },
    { "Lex Luger",        81u, "luger_fromstr",270u, 6u,  4u,
      "luger_quote", "LEX3", 10u, 7u, 3u, "luger_tips" }
};

/*
 * CRD_SCRN2's five lines for a factory-set cabinet with no credits -- see
 * wm/arcade/wmania_attract_data.h for the whole derivation, including why
 * these five and not any of the others the routine can produce.
 *
 * The credit count and the two adjustment numbers are formatted here rather
 * than at draw time because they cannot change: this port has no coin
 * subsystem to change them. If one is ever added, these become a function of
 * it and this table goes away.
 */
const WmAttractCreditLine
wm_attract_credit_lines[WM_ATTRACT_CREDIT_LINES] = {
    /* #crd_str "CREDITS : " + dec_to_asc(CRED_P = 0), AUDIT.ASM:1421-:1422.
       No " (MAXIMUM)" because 0 < ADJMAXC 50. LN1_setup's y is LN1+4. */
    { "CREDITS : 0", 74, "crd_str" },
    /* CSELCT entry 0's CS_LIST -> Q_Q -> C11, MENU.ASM:7132. */
    { "1 CREDIT / 1 COIN", 132, "C11" },
    /* dec_to_asc(ADJCSTRT = 2) + #crd_2starts, the plural, AUDIT.ASM:1425-:1426. */
    { "2 CREDITS TO START", 149, "crd_2starts" },
    /* dec_to_asc(ADJCCONT = 2) + #crd_2conts, AUDIT.ASM:1429-:1430. */
    { "2 CREDITS TO CONTINUE", 166, "crd_2conts" },
    /* #str_insert, AUDIT.ASM:1435-:1436, because 0 credits is #not_ready.
       LN5_setup's y is LN5+10 = 200, plus YSPACE 25. */
    { "INSERT COINS", 225, "str_insert" }
};

const WmAttractScreenLayout wm_attract_general_tips_layout = {
    "hstd_mod", "gen_tip_mes", 200, 10, 200, 60, 15,
    0u, 10u, 1u
};

const WmAttractScreenLayout wm_attract_operator_layout = {
    "SPORTBKBMOD", 0, 0, 0, 200, 50, 45,
    120u, 6u, 1u
};

const WmAttractScreenLayout wm_attract_aama_layout = {
    0, "aama_ln1", 200, 94, 200, 114, 11,
    20u, 4u, 1u
};

const char *const wm_attract_general_tip_labels[] = {
    "gen_tip1", "gen_tip1a", "blank",
    "gen_tip2", "gen_tip2a", "blank",
    "gen_tip3", "gen_tip3a", "blank",
    "gen_tip4", "gen_tip4a", 0
};

const char *const wm_attract_copyright_page1_labels[
    WM_ATTRACT_COPYRIGHT_PAGE1_LINES] = {
    "copyright_ln1", "copyright_ln2", "copyright_ln3",
    "copyright_ln4", "copyright_ln5", "copyright_ln6",
    "copyright_ln7", "copyright_ln8", "copyright_ln9"
};

const char *const wm_attract_copyright_page2_labels[
    WM_ATTRACT_COPYRIGHT_PAGE2_LINES] = {
    "copyright_ln10", "copyright_ln11", "copyright_ln12",
    "copyright_ln13", "copyright_ln14", "copyright_ln15",
    "copyright_ln16", "copyright_ln17", "copyright_ln18",
    "copyright_ln19"
};

const char *const wm_attract_aama_labels[6] = {
    "aama_ln1", "aama_ln2", "aama_ln2b",
    "aama_ln3", "aama_ln4", "aama_ln5"
};

/*
 * ATTRACT.ASM:308-343. See the header for the source lines these come
 * from and why the screen needs a per-line table.
 *
 * The two colours are the source's own words: >1111 is its "pal 0,
 * color 17" and >0606 is color 6, the one ln2b uses so that "- MILD"
 * reads apart from the advisory above it. They are carried here and NOT
 * resolved: turning a TMS palette index into RGB needs the palette, and
 * this port has no pal_getf behind it. The renderer draws both in the
 * same colour as a result -- a known, visible divergence rather than an
 * invented pair of colours.
 */
const WmAttractAamaLine wm_attract_aama_lines[WM_ATTRACT_AAMA_LINES] = {
    { "aama_ln1",  200,  94, 0x1111u },
    { "aama_ln2",  177, 114, 0x1111u },
    { "aama_ln2b", 260, 114, 0x0606u },
    { "aama_ln3",  200, 125, 0x1111u },
    { "aama_ln4",  200, 136, 0x1111u },
    { "aama_ln5",  200, 147, 0x1111u }
};
