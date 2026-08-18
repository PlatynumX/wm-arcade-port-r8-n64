/* Auto-generated from ATTRACT.ASM::attract_mode active JSRP calls. */
#include "wm/attract.h"

const wm_attract_call wm_source_attract_loop[] = {
    WM_ATTRACT_SHOW_HSTD,
    WM_ATTRACT_DCS_LOGO,
    WM_ATTRACT_SHOW_SPORTS_LOGO,
    WM_ATTRACT_SHOW_GAMEPLAY,
    WM_ATTRACT_CREDITSCREEN,
    WM_ATTRACT_SHOW_TITLE,
    WM_ATTRACT_SHOW_GAMEPLAY,
    WM_ATTRACT_CREDITSCREEN,
    WM_ATTRACT_DO_HINTS,
    WM_ATTRACT_SHOW_GEN_TIPS,
    WM_ATTRACT_SHOW_BIOS,
    WM_ATTRACT_SHOW_BIOS_TIPS,
    WM_ATTRACT_SHOW_OPERATORMSG,
};

const size_t wm_source_attract_loop_count =
    sizeof(wm_source_attract_loop) / sizeof(wm_source_attract_loop[0]);
