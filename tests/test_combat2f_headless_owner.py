from pathlib import Path
s=(Path(__file__).resolve().parents[1]/"tools/apply_fix39.py").read_text()
assert "wm_fix39_attract_step_owner(2u)" in s
assert "wm_fix39_attract_step_runnable(2u)" in s
assert "SHOW_GAMEPLAY not marked translated" not in s
print("Combat2f headless owner/runnable regression: PASS")
