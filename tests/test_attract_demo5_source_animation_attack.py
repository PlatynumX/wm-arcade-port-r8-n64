from pathlib import Path
root=Path(__file__).resolve().parents[1]
runtime=(root/"src/fix39/wm_fix39_runtime.c").read_text()
app=(root/"tools/apply_fix39.py").read_text()
build=(root/"termux_fix39_build.sh").read_text()
assert "g.status.animation_backend_ready = true" in runtime
assert "wm_fix39_match_bind_source_frame_attack" in runtime
assert "wm_arcade_character_attack_for_source_frame" in runtime
assert "wm_fix39_match_bind_bret_demo_attack" not in runtime
assert "fix39_character_attack_frames.py" in build
assert "fix39_character_attack_frames.py" in build
assert "wm_fix39_match_bind_source_frame_attack" in app
print("Fix39 Demo5 source animation attack-window bridge: PASS")
