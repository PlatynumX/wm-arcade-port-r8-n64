#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT/'src/fix39/wm_arcade_drone_source_services.h').read_text()
build = (ROOT/'termux_fix39_build.sh').read_text()
tool = ROOT/'tools/fix39_drone_bodies.py'
assert tool.is_file(), 'C5f source-body recovery tool missing'
assert build.count('fix39_drone_bodies.py') >= 2, 'C5f build must self-test and execute body recovery'
assert 'fix39-v13e-c5f-drone-bodies.txt' in build, 'C5f body report not enforced'
assert header.count('typedef int (*wm_arcade_drone_source_service_handler_t)') == 1, 'service handler typedef must appear exactly once'
assert header.count('#endif') == 3, 'clean service header should have two C++ guards plus the include guard'
print('Fix39 V13e chunk 5f structural regression: PASS')
