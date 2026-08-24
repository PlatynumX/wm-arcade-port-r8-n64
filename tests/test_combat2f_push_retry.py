from pathlib import Path
s=(Path(__file__).resolve().parents[1]/"termux_fix39_build.sh").read_text()
assert "for push_try in 1 2 3" in s
assert "git push attempt ${push_try}/3" in s
assert "exit 70" in s
print("Combat2f push retry regression: PASS")
