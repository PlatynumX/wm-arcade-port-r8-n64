#include <stdio.h>
#include "wm/app.h"

#define SMOKE_GUARD 12000u

static int run_translated_base_loop(wm_app *app, const wm_input_state *button)
{
    size_t i;
    unsigned guard;

    for (i = 0; i < wm_source_attract_loop_count; ++i) {
        const wm_attract_call call = wm_source_attract_loop[i];

        /*
         * Follow the generated source table rather than a hand-maintained
         * list. The live dispatcher skips only genuinely untranslated calls.
         */
        if (!wm_attract_call_is_translated(call))
            continue;

        guard = 0;
        while (app->attract.call != call && guard++ < SMOKE_GUARD)
            wm_app_tick(app, button);
        if (app->attract.call != call)
            return 10 + (int)i;

        guard = 0;
        while (app->attract.call == call && guard++ < SMOKE_GUARD)
            wm_app_tick(app, button);
        if (app->attract.call == call)
            return 40 + (int)i;
    }
    return 0;
}

int main(void)
{
    wm_app app;
    wm_input_state button = {.run = true};
    int rc;

    wm_app_init(&app);
    rc = run_translated_base_loop(&app, &button);
    if (rc)
        return rc;

    /*
     * The last base-loop row is currently the untranslated operator message.
     * skip_untranslated_calls must step over it, finish loop zero and begin
     * loop one rather than getting stuck on the hole.
     */
    if (app.attract.amode_loops != 1)
        return 2;
    if (app.attract.call != WM_ATTRACT_SHOW_HSTD)
        return 3;

    if (!wm_attract_call_is_translated(WM_ATTRACT_CREDITSCREEN))
        return 4;
    if (!wm_attract_call_is_translated(WM_ATTRACT_SHOW_GAMEPLAY))
        return 5;
    if (!wm_attract_call_is_translated(WM_ATTRACT_SHOW_TITLE))
        return 6;

    printf("wm_arcade_port r9\n");
    printf("attract source calls=%zu current=%s loops=%u\n",
           wm_source_attract_loop_count,
           wm_attract_call_name(app.attract.call),
           app.attract.amode_loops);
    printf("frontend rule: every translated source row executed in generated order\n");
    printf("strict source attract executor: PASS\n");
    return 0;
}
