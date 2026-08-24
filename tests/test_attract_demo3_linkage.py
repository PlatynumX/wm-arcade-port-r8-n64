from pathlib import Path
root = Path(__file__).resolve().parents[1]
apply = (root/'tools/apply_fix39.py').read_text()
build = (root/'termux_fix39_build.sh').read_text()
assert 'src/generated/bret_sprites.c' in apply, 'CMake patch does not link generated bret_sprites.c'
assert 'sh scripts/prepare_bret_sprites.sh' in build, 'build script does not generate bret_sprites.c before host CMake'
assert "grep -q 'wm_bret_sprite_find' src/generated/bret_sprites.c" in build, 'build script does not verify lookup implementation'
print('Fix39 Demo3e Bret WIMP linkage regression: PASS')
