from pathlib import Path
s=Path("termux_fix39_build.sh").read_text()
# Historical regression retained as a semantic check only: the build must reject
# app.c taking gameplay roster ownership back from ATTR/runtime.
assert "app.c illegally restored wm_demo roster ownership" in s
assert "wm_demo_set_roster" in s
print("late-staging single-authority legacy regression: PASS")
