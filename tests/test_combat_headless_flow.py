from pathlib import Path
import importlib.util, tempfile
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('apply_fix39', root/'tools/apply_fix39.py')
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
sample='''#include <stdio.h>\nint main(void) {\n    wm_app app; unsigned guard=0; wm_input_state button={.run=true};\n    while (app.attract.call == WM_ATTRACT_SHOW_SPORTS_LOGO && guard++ < 4000)\n        wm_app_tick(&app, &button);\n    /* Strict source mode skips harness-only show_gameplay/credits and lands on\n       the newly translated source title routine. */\n    if (app.attract.call != WM_ATTRACT_SHOW_TITLE)\n        return 3;\n\n    while (app.attract.call == WM_ATTRACT_SHOW_TITLE && guard++ < 6000)\n        wm_app_tick(&app, &button);\n    if (app.attract.call != WM_ATTRACT_DCS_LOGO)\n        return 4;\n    if (app.attract.amode_loops != 1)\n        return 5;\n    if (wm_attract_call_is_translated(WM_ATTRACT_SHOW_GAMEPLAY))\n        return 6;\n    if (!wm_attract_call_is_translated(WM_ATTRACT_SHOW_TITLE))\n        return 7;\n    printf("wm_arcade_port r9\\n");\n    printf("frontend rule: harness-only code excluded from normal arcade execution\\n");\n}\n'''
with tempfile.TemporaryDirectory() as td:
    p=Path(td)/'main.c'; p.write_text(sample)
    mod.patch_headless_main(p)
    first=p.read_text()
    mod.patch_headless_main(p)
    second=p.read_text()
    assert first == second
    assert second.count('WM_ATTRACT_SHOW_GAMEPLAY') >= 4
    assert 'wm_fix39_attract_step_owner(2u)' in second
    assert 'wm_fix39_attract_step_runnable(2u)' in second
    assert 'source gameplay demos execute in normal arcade attract' in second
print('Fix39 combat headless attract-flow regression: PASS')
