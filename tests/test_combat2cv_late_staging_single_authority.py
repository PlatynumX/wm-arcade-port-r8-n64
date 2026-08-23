from pathlib import Path
root = Path(__file__).resolve().parents[1]
s = (root / 'termux_fix39_build.sh').read_text()
assert "app.c illegally restored wm_demo roster ownership" in s
assert "grep -q 'wm_demo_set_roster(&app->demo' src/core/app.c" in s
assert 'Combat2AJ app.c roster setter consumer missing' not in s
# Demo API may remain for standalone/core tests, but the live app consumer is forbidden.
assert "grep -q 'void wm_demo_set_roster' include/wm/demo.h" in s
assert "grep -q 'void wm_demo_set_roster' src/core/demo.c" in s
print('Combat2CV late staging single-authority regression: PASS')
