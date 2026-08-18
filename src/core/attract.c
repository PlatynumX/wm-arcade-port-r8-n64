#include "wm/attract.h"

const char *wm_attract_call_name(wm_attract_call call) {
    switch (call) {
        case WM_ATTRACT_SHOW_HSTD: return "show_hstd";
        case WM_ATTRACT_DCS_LOGO: return "DCS_LOGO";
        case WM_ATTRACT_SHOW_SPORTS_LOGO: return "show_sports_logo";
        case WM_ATTRACT_SHOW_GAMEPLAY: return "show_gameplay";
        case WM_ATTRACT_CREDITSCREEN: return "creditscreen";
        case WM_ATTRACT_SHOW_TITLE: return "show_title";
        case WM_ATTRACT_DO_HINTS: return "DO_HINTS";
        case WM_ATTRACT_SHOW_GEN_TIPS: return "show_gen_tips";
        case WM_ATTRACT_SHOW_BIOS: return "show_bios";
        case WM_ATTRACT_SHOW_BIOS_TIPS: return "show_bios_tips";
        case WM_ATTRACT_SHOW_OPERATORMSG: return "show_operatormsg";
        case WM_ATTRACT_SHOW_TIME_DATE: return "show_time_date";
        case WM_ATTRACT_SHOW_COPYRIGHT: return "show_copyright";
        case WM_ATTRACT_AAMA_MESSAGE: return "aama_message";
        case WM_ATTRACT_CALL_COUNT: break;
    }
    return "?";
}
