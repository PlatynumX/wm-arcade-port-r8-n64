from pathlib import Path
import re
p=Path(__file__).resolve().parents[1]/'tools/fix39_combat_completion_patch.py'
s=p.read_text()
assert "re.search(r'^static\\s+bool\\s+live_start_source_anim" in s
assert "anchor='static void live_react_anim" in s
print('Combat2BT source animation helper declaration ordering: PASS')
