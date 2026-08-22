#include <stdio.h>
#include "wm/app.h"

int main(void) {
    wm_app app;
    wm_app_init(&app);
    wm_input_state button = {.run = true};

    unsigned guard = 0;
    while (app.attract.call != WM_ATTRACT_SHOW_SPORTS_LOGO && guard++ < 2000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_SPORTS_LOGO)
        return 2;

    while (app.attract.call == WM_ATTRACT_SHOW_SPORTS_LOGO && guard++ < 4000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_GAMEPLAY)
        return 3;

    while (app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY && guard++ < 6000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_TITLE)
        return 4;

    while (app.attract.call == WM_ATTRACT_SHOW_TITLE && guard++ < 8000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_GAMEPLAY)
        return 5;

    while (app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY && guard++ < 10000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_DCS_LOGO)
        return 6;
    if (app.attract.amode_loops != 1)
        return 7;
    if (!wm_attract_call_is_translated(WM_ATTRACT_SHOW_GAMEPLAY))
        return 8;
    if (!wm_attract_call_is_translated(WM_ATTRACT_SHOW_TITLE))
        return 9;
    printf("wm_arcade_port r9\n");
    printf("attract source calls=%zu current=%s loops=%u\n",
           wm_source_attract_loop_count,
           wm_attract_call_name(app.attract.call),
           app.attract.amode_loops);
    printf("show_gameplay status=%s\n",
           wm_port_status_name(wm_attract_call_port_status(WM_ATTRACT_SHOW_GAMEPLAY)));
    printf("frontend rule: source gameplay demos execute in normal arcade attract\n");
    printf("strict source attract executor: PASS\n");
    return 0;
}
