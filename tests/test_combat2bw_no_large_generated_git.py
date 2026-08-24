from pathlib import Path
s=(Path(__file__).parents[1]/'termux_fix39_build.sh').read_text()
assert 'src/generated/character_assets.c' not in next(line for line in s.splitlines() if line.startswith('git add .github/workflows'))
assert 'git restore --source=HEAD --staged --worktree src/generated/character_assets.c' in s
assert 'refusing to stage generated character_assets.c' in s
print('Combat2BW generated-character Git staging guard: PASS')
