#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'tools/fix39_crowd_renderer_patch.py').read_text()
assert 'if(!px||!pal)return false;' in s, 'dynamic crowd load failure must fall back to BMOD object'
assert 'if(!a)return false;' in s, 'missing dynamic frame must preserve BMOD object'
assert 'if (fix39_draw_crowd_block(b)) return;' in s, 'dynamic frame replaces static only after successful draw'
print('Combat2BC crowd visibility fallback PASS')
