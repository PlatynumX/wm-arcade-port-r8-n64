from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'termux_fix39_build.sh').read_text()
assert 'git add .github/workflows/build.yml' in s
assert 'tools/fix39_bret_attack_frames.py' in s.split('git add ',1)[1].split('\n',1)[0]
a=(root/'tools/apply_fix39.py').read_text()
assert 'sh ./scripts/prepare_bret_sprites.sh' in a
assert 'python3 tools/fix39_bret_attack_frames.py' in a
print('Demo5b CI commit wiring: PASS')
