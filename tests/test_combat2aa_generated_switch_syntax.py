#!/usr/bin/env python3
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'tools'/'fix39_character_assets.py').read_text()
assert "+'};return t[slot];}')" in s
assert "+'};return t[slot];}}')" not in s
# The switch/function itself still legitimately needs two closing braces.
assert "default:return 0;}}" in s
print('combat2aa generated switch syntax regression: PASS')
