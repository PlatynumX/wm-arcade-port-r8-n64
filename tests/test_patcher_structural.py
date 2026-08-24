#!/usr/bin/env python3
from pathlib import Path
import importlib.util

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("apply_fix39", root / "tools" / "apply_fix39.py")
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)

variants = [
'''void wm_app_tick(wm_app *app, const wm_input_state *input) {
    bool done = false;
    switch (app->attract.call) {
        case WM_ATTRACT_DCS_LOGO: done = tick_dcs_logo(app, input); break;
        case WM_ATTRACT_SHOW_SPORTS_LOGO: done = tick_sports_logo(app, input); break;
        case WM_ATTRACT_SHOW_TITLE: done = tick_title(app, input); break;
        default: break;
    }
    wm_scheduler_step(&app->scheduler);
}
''',
'''void wm_app_tick(wm_app *app, const wm_input_state *input)
{
    bool done = false;
    switch
    (
        app->attract.call
    )
    { // integrated baseline may carry comments/format drift
      case WM_ATTRACT_DCS_LOGO:
        done = tick_dcs_logo(app, input);
        break;
      case WM_ATTRACT_SHOW_TITLE: { done = tick_title(app, input); break; }
      default: /* old fallback */ break;
    }
    wm_scheduler_step(&app->scheduler);
}
''',
'''static void helper(void) {
    /* This earlier call line is what made the old function-span helper brittle. */
    wm_app_tick(global_app, global_input);
}

void wm_app_tick(wm_app *app, const wm_input_state *input) {
    if (!app) return;
    bool done = false;
    switch (app->attract.call) {
        case WM_ATTRACT_DCS_LOGO: done = tick_dcs_logo(app, input); break;
        case WM_ATTRACT_SHOW_SPORTS_LOGO: done = tick_sports_logo(app, input); break;
        case WM_ATTRACT_SHOW_TITLE: done = tick_title(app, input); break;
        default: break;
    }
    wm_scheduler_step(&app->scheduler);
}
'''
]

for i, src in enumerate(variants):
    out = mod.replace_attract_call_switch(src)
    assert out.count("wm_fix39_attract_screen_tick(") == 1, i
    assert "case WM_ATTRACT_DCS_LOGO" in out, i
    assert "case WM_ATTRACT_SHOW_SPORTS_LOGO" in out, i
    assert "case WM_ATTRACT_SHOW_TITLE" in out, i
    assert "wm_scheduler_step(&app->scheduler);" in out, i
    again = mod.replace_attract_call_switch(out)
    assert again == out, i

print("Fix39 V11b structural attract-switch regression: PASS")
