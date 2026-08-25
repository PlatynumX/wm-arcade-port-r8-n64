#include "wm/app.h"
#include "wm/cabinet_bridge.h"
#include "wm/select.h"
#include "wm_fix39_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_cabinet_pstatus(void) {
    wm_cabinet_bridge_state s;
    wm_cabinet_bridge_init(&s);
    assert(wm_cabinet_bridge_pstatus(&s) == 0u);
    assert(wm_cabinet_bridge_accept_player_start(&s, 0u));
    assert(wm_cabinet_bridge_pstatus(&s) == 1u);
    assert(s.old_pstatus == 0u);
    assert(wm_cabinet_bridge_accept_player_start(&s, 1u));
    assert(wm_cabinet_bridge_pstatus(&s) == 3u);
    assert(s.old_pstatus == 1u);
    assert(s.physical_coin_events == 0u);
}

static void test_match_init_handoff(void) {
    wm_app app;
    wm_input_state in;
    memset(&in, 0, sizeof(in));
    wm_app_init(&app);
    app.p1_choice = WM_WRESTLER_BRET;
    app.pregame.opponent_count = 1u;
    app.pregame.opponents[0] = 1u; /* SELECT source id 1 = Razor */
    app.mode = WM_APP_MODE_MATCH_INIT;
    wm_app_tick_dual(&app, &in, 0);
    assert(app.mode == WM_APP_MODE_MATCH);
    assert(app.p2_choice == WM_WRESTLER_RAZOR);
    assert(wm_fix39_match_started());
    wm_app_tick_dual(&app, &in, 0);
}

int main(void) {
    test_cabinet_pstatus();
    test_match_init_handoff();
    printf("R37 remaining gap closure regression: PASS\n");
    return 0;
}
