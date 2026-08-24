from pathlib import Path
p=Path('tools/fix39_combat_completion_patch.py').read_text()
# Semantic contract: on-ground recovery owns a countdown and only returns to normal
# after it expires; wrestler_main must not force an immediate stand transition.
assert '--a->getup_time' in p
assert 'getup_time' in p
assert 'Do not force MODE_NORMAL or a stand animation from wrestler_main' in p
assert 'if (a->player_mode == WM_PMODE_ONGROUND) a->player_mode = WM_PMODE_NORMAL' not in p
print('WRESTLE.ASM GETUP_TIME countdown ownership: PASS')
