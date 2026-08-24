#!/usr/bin/env python3
from pathlib import Path
import subprocess,sys,tempfile,shutil
repo=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
cat=(repo/'src/fix39/wm_arcade_source_animation_catalog.h').read_text()
run=(repo/'src/fix39/wm_arcade_source_animation_runtime.c').read_text()
fix=(repo/'tools/fix39_combat_completion_patch.py').read_text() if (repo/'tools/fix39_combat_completion_patch.py').exists() else ''
assert 'WM_SRC_ANIM_CTRL_WAITROLL' in cat
assert 'd->control_flags & WM_SRC_ANIM_CTRL_WAITROLL' in run
assert 'a->getup_time != 0' in run
assert 'wm_source_anim_runtime_change(s, a, (uint8_t)a->wrestler_num, next)' in run
if fix:
    assert 'Do not force MODE_NORMAL or a stand animation from wrestler_main' in fix
print('Combat2CD ANIM.ASM WAITROLL/get-up control regression: PASS')
