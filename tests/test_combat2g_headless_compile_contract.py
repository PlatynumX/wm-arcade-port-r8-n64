from pathlib import Path
s=(Path(__file__).resolve().parents[1]/"tools/apply_fix39.py").read_text()
assert "#include \"wm_fix39_runtime.h\"" in s
assert "HEADLESS_ATTRACT_FAIL[8]: first gameplay step not runnable, owner=%d\\\\n" in s
assert "WmAttractOwner owner = wm_fix39_attract_step_owner(2u);" in s
print("Combat2g headless compile contract: PASS")
