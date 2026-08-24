# Combat2DE staging-manifest hardening

Combat2DD proved the translated CZ runtime compiles and all four host CTest suites pass, then failed at the commit stage because `tools/fix39_dual_anim_channel_patch.py` was named in the `git add` path list without ever being copied from the integrator into the cloned worktree.

Combat2DE fixes the class of failure, not just the one filename:

- copies the CZ execute-walk patcher, dual-animation patcher, and CZ regression test into the cloned repository before staging;
- stages both CZ patchers so the committed tree remains reproducible;
- performs an explicit existence audit of every path in the main staging manifest immediately before `git add`, with a named diagnostic for any missing path;
- preserves the CZ gameplay/runtime changes unchanged.

No intentional gameplay behavior is changed from Combat2CZ/DA/DB/DC/DD.
