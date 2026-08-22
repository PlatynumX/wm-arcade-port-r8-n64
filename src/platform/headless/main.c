#include <stdio.h>
#include "wm/app.h"
#include "wm_fix39_runtime.h"

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
    if (app.attract.call != WM_ATTRACT_SHOW_GAMEPLAY) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[3]: expected first SHOW_GAMEPLAY, call=%d guard=%u\n", (int)app.attract.call, guard);
        return 3;
    }

    while (app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY && guard++ < 6000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_TITLE) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[4]: first demo did not return to TITLE, call=%d guard=%u\n", (int)app.attract.call, guard);
        return 4;
    }

    while (app.attract.call == WM_ATTRACT_SHOW_TITLE && guard++ < 8000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_SHOW_GAMEPLAY) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[5]: expected second SHOW_GAMEPLAY, call=%d guard=%u\n", (int)app.attract.call, guard);
        return 5;
    }

    while (app.attract.call == WM_ATTRACT_SHOW_GAMEPLAY && guard++ < 10000)
        wm_app_tick(&app, &button);
    if (app.attract.call != WM_ATTRACT_DCS_LOGO) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[6]: second demo did not loop to DCS_LOGO, call=%d guard=%u\n", (int)app.attract.call, guard);
        return 6;
    }
    if (app.attract.amode_loops != 1) {
        fprintf(stderr, "HEADLESS_ATTRACT_FAIL[7]: amode_loops=%u expected=1\n", app.attract.amode_loops);
        return 7;
    }
    /* SHOW_GAMEPLAY is an existing-frontend-owned source step. The legacy
       wm_attract_call_is_translated() table does not own it; V13e owner/runnable
       state is authoritative. */
    {
        WmAttractOwner owner = wm_fix39_attract_step_owner(2u);
        if (owner != WM_ATTRACT_OWNER_EXISTING_FRONTEND ||
            !wm_fix39_attract_step_runnable(2u)) {
            fprintf(stderr, "HEADLESS_ATTRACT_FAIL[8]: first gameplay step not runnable, owner=%d\n", (int)owner);
            return 8;
        }
    }
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
