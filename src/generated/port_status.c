/* Auto-generated from port/translation_manifest.json. */
#include "wm/attract.h"

wm_port_status wm_attract_call_port_status(wm_attract_call call) {
    switch (call) {
        case WM_ATTRACT_SHOW_HSTD: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_DCS_LOGO: return WM_PORT_PARTIAL_SOURCE;
        case WM_ATTRACT_SHOW_SPORTS_LOGO: return WM_PORT_PARTIAL_SOURCE;
        case WM_ATTRACT_SHOW_GAMEPLAY: return WM_PORT_PARTIAL_SOURCE;
        case WM_ATTRACT_CREDITSCREEN: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_SHOW_TITLE: return WM_PORT_PARTIAL_SOURCE;
        case WM_ATTRACT_DO_HINTS: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_SHOW_GEN_TIPS: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_SHOW_BIOS: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_SHOW_BIOS_TIPS: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_SHOW_OPERATORMSG: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_SHOW_TIME_DATE: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_SHOW_COPYRIGHT: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_AAMA_MESSAGE: return WM_PORT_NOT_STARTED;
        case WM_ATTRACT_CALL_COUNT: break;
    }
    return WM_PORT_NOT_STARTED;
}

const char *wm_port_status_name(wm_port_status status) {
    switch (status) {
        case WM_PORT_NOT_STARTED: return "not-started";
        case WM_PORT_PARTIAL_SOURCE: return "partial-source";
        case WM_PORT_EXACT_SOURCE: return "exact-source";
        case WM_PORT_HARNESS_ONLY: return "harness-only";
    }
    return "?";
}
