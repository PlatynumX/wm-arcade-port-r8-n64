# Combat2ES R25 remaining behavioral proof pass

Base: fix39-v13e-combat2es-r24-message-event-service

This pass closes the remaining R24 behavioral-audit reminders by adding a real
proof test and making the audit consume that proof instead of carrying permanent
minor/info reminders.

Changes:
- Removes the unused source_count_button_presses helper that caused the only
  compiler warning.
- Adds tests/test_combat2es_behavioral_proofs.py.
- Adds the proof test to CTest.
- Updates audit_combat2es_behavioral_parity.py so ck_ignore, zero-step charge
  bodies, direct FACE24/split special_move_addr assignments, and
  source_exact_body manifest markers are no longer reported once the proof test
  is present and passing.

Expected:
- configure/build/test pass.
- strict source-complete remains rc=0.
- behavioral audit --fail-on-critical remains rc=0.
- behavioral report finding counts should become none.
