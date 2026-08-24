#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
script=(root/'termux_fix39_build.sh').read_text(errors='ignore')
violations=[]
for m in re.finditer(r'python "\$ROOT/tests/([^"]+)" "\$WORK"', script):
    rel=m.group(1); p=root/'tests'/rel
    if not p.exists():
        violations.append(f'{rel}: missing')
        continue
    text=p.read_text(errors='ignore')
    # A WORK-mode test may inspect generated/integrated outputs. It must not
    # unconditionally assume package-only tools/tests live under that WORK root.
    # Branches guarded by len(sys.argv) are allowed when the WORK branch does not
    # dereference root/tools.
    if "root/'tools/" in text:
        pre=text.split('else:',1)[0] if 'else:' in text else text
        if "root/'tools/" in pre:
            violations.append(f'{rel}: WORK path dereferences package-only tools')
if violations:
    raise SystemExit('Combat2DX post-integration ROOT/WORK contract failures: '+ '; '.join(violations))
print('Combat2DX post-integration ROOT/WORK path contract: PASS')
