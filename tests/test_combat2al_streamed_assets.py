#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
g=(root/'tools/fix39_character_assets.py').read_text()
a=(root/'tools/apply_fix39.py').read_text()
assert "--out-fs" in g
assert "#if defined(__mips__)" in g
assert "rom:/%s" in g
assert "wm_char_cache_slots[8]" in g
assert "fopen(full,\"rb\")" in g
assert "FIX39_CHAR_DFS_FILES" in a
assert "filesystem/fix39_chars/*/*.bin" in a
assert "filesystem/fix39_chars -type f -name" in a
# N64 path must not compile the giant embedded pixel arrays.
assert "#if !defined(__mips__)" in g
print('Combat2AL streamed character art regression: PASS')

assert "data_cache_hit_writeback(cc->mem,total)" in g
assert "#include <libdragon.h>" in g
