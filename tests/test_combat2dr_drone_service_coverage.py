#!/usr/bin/env python3
from pathlib import Path
import re,sys
root=Path(sys.argv[1])
svc=(root/'src/fix39/wm_arcade_drone_source_services_generated.h').read_text()
bod=(root/'src/fix39/wm_arcade_drone_source_bodies_generated.h').read_text()
a=int(re.search(r'WM_FIX39_DRONE_SERVICE_COUNT\s+(\d+)',svc).group(1))
b=int(re.search(r'WM_FIX39_DRONE_TRANSLATED_BODY_COUNT\s+(\d+)',bod).group(1))
# Diagnostic, not a fake parity pass: DRONE.ASM command #5 is CALL, so a missing
# handler cannot be treated as executed. Surface this prominently in every build.
print(f'Combat2DR DRONE.ASM executable-service coverage: translated={b}/{a}')
if a and b<a:
 print('Combat2DR NOTE: untranslated DRONE CALL seams remain source-parity work; runtime dispatch intentionally does not fake them.')
