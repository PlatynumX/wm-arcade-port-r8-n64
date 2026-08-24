# Combat2DB — execute_walk regression hardening

Input baseline: Combat2DA.

The DA Termux run successfully reached the Combat2CZ WRESTLE.ASM execute_walk patch, then failed in `test_combat2bo_execute_walk_source.py`. The generated C contains the required source multipliers as `(xv*230)>>8`, `(zv*230)>>8`, and `(xv*384)>>8`; the legacy test incorrectly required whitespace-specific strings (`xv * 230`, etc.).

DB makes both BO and BN execute_walk parity tests whitespace-insensitive while preserving every semantic token they verify. No gameplay/runtime behavior is changed from CZ/DA.
