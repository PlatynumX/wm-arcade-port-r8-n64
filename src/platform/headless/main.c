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

    /* Strict source mode skips harness-only show_gameplay/credits and lands on
       the newly translated source title routine. */
    if (app.attract.call != WM_ATTRACT_SHOW_TITLE)
        return 3;

    while (app.attract.call == WM_ATTRACT_SHOW_TITLE && guard++ < 6000)
        wm_app_tick(&app, &button);

    if (app.attract.call != WM_ATTRACT_DCS_LOGO)
        return 4;
    if (app.attract.amode_loops != 1)
        return 5;
    if (wm_attract_call_is_translated(WM_ATTRACT_SHOW_GAMEPLAY))
        return 6;
    if (!wm_attract_call_is_translated(WM_ATTRACT_SHOW_TITLE))
        return 7;

    printf("wm_arcade_port r9\n");
    printf("attract source calls=%zu current=%s loops=%u\n",
           wm_source_attract_loop_count,
           wm_attract_call_name(app.attract.call),
           app.attract.amode_loops);
    printf("show_gameplay status=%s\n",
           wm_port_status_name(wm_attract_call_port_status(WM_ATTRACT_SHOW_GAMEPLAY)));
    printf("frontend rule: harness-only code excluded from normal arcade execution\n");
    printf("strict source attract executor: PASS\n");
    return 0;
}
