from pathlib import Path
import importlib.util

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('apply_fix39', ROOT/'tools'/'apply_fix39.py')
mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)

sample = '''#include <stdio.h>\nint main(void) {\n    wm_app app; unsigned guard=0; wm_input_state button={.run=true};\n    while (app.attract.call == WM_ATTRACT_SHOW_SPORTS_LOGO && guard++ < 4000)\n        wm_app_tick(&app, &button);\n    if (app.attract.call != WM_ATTRACT_SHOW_TITLE)\n        return 3;\n    printf("wm_arcade_port r9\\n");\n    printf("frontend rule: harness-only code excluded from normal arcade execution\\n");\n}\n'''
import tempfile
with tempfile.TemporaryDirectory() as td:
    p=Path(td)/'main.c'; p.write_text(sample)
    mod.patch_headless_main(p)
    out=p.read_text()
    for n in range(3,9):
        assert f'HEADLESS_ATTRACT_FAIL[{n}]' in out
    assert 'source gameplay demos execute in normal arcade attract' in out
print('Combat2f headless attract diagnostics regression: PASS')
