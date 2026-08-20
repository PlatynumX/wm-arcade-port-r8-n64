#include "wm/arcade/wmania_attract_data.h"

const WmAttractHint wm_attract_hints[WM_ATTRACT_ACTIVE_HINTS] = {
    { "HNTT_2", "HNT_2", "JMSTIP", "JASMUG", 0u },
    { "HNTT_4", "HNT_4", "MIKTIP", "MIKMUG", 1u },
    { "HNTT_3", "HNT_3", "MJTTIP", "MRKMUG", 2u },
    { "HNTT_7", "HNT_7", "EUGTIP", "EUGMUG", 3u },
    { "HNTT_5", "HNT_5", "SHNTIP", "SHNMUG", 4u }
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
