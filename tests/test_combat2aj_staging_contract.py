from pathlib import Path
s = (Path(__file__).resolve().parents[1] / "termux_fix39_build.sh").read_text()
add_line = next((ln for ln in s.splitlines() if ln.startswith("git add ")), "")
for req in ("include/wm/demo.h", "src/core/demo.c", "src/core/app.c"):
    assert req in add_line, f"{req} missing from git add contract"
assert "required_staged in include/wm/demo.h src/core/demo.c src/core/app.c" in s
assert "UNSTAGED_TRACKED_ALL" in s and "UNSTAGED_TRACKED=" in s
assert "grep -vxF 'src/generated/sports_background.c'" in s
assert "grep -vxF 'src/generated/character_assets.c'" in s
assert "allowing expected derived-only dirty file: src/generated/sports_background.c" in s
assert "allowing expected CI-regenerated file: src/generated/character_assets.c" in s
assert 'if [ -n "$UNSTAGED_TRACKED" ]; then' in s
assert 'fail "Combat2AJ refusing to push an incomplete patch commit"' in s
# The derived/generated C payloads must NOT be staged by the scoped source add.
assert "src/generated/sports_background.c" not in add_line
assert "src/generated/character_assets.c" not in add_line
# Oversized character C must be explicitly rejected if it somehow enters the index.
assert "git diff --cached --name-only | grep -qxF 'src/generated/character_assets.c'" in s
print("Combat2AJ scoped staging guard regression: PASS")
