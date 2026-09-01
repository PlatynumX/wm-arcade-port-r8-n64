#!/usr/bin/env python3
import pathlib, sys
root=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
main=(root/'src/platform/n64/main.c').read_text(errors='replace')
hdr=(root/'src/fix39/wm_fix39_runtime.h').read_text(errors='replace')
rt=(root/'src/fix39/wm_fix39_runtime.c').read_text(errors='replace')
aud=(root/'tools/audit_collis_source_parity.py').read_text(errors='replace')
checks=[
 ('USB diag marker','R37N15 FREEZE-DIAG' in main),
 ('low-rate snapshots','WM_R37N15_DIAG_PERIOD 53u' in main and 'wm_r37n15_print_actor("SNAP"' in main),
 ('dispatch stall event','DISPATCH-STALL' in main),
 ('state stall event','STATE-STALL' in main),
 ('diagnostic tick wired','wm_r37n15_diag_tick();' in main),
 ('read-only resume probe','wm_fix39_actor_process_resume(size_t index)' in rt and 'wm_fix39_actor_process_resume(size_t index);' in hdr),
 ('read-only getup probe','wm_fix39_actor_getup_phase(size_t index)' in rt),
 ('read-only SMOVE count','wm_fix39_actor_smove_active_count(size_t index)' in rt),
 ('N14 gameplay retained','R37N14 / COLLIS.ASM readiness translation.' in rt),
 ('N13 scheduler retained','wm_arcade_wrestler_process_resume' in rt),
 ('stale R37N4 audit repaired','valid_defenders, g.active_actor_count' in aud),
]
failed=False
for label,ok in checks:
    print(('PASS: ' if ok else 'FAIL: ')+label)
    failed |= not ok
if failed: raise SystemExit(1)
print('R37N15 P2 freeze diagnostic audit: PASS')
