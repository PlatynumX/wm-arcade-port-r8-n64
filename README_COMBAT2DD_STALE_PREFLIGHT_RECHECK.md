# Combat2DD stale-preflight recheck

Combat2DD is a preflight-hardening revision of Combat2DC. It intentionally changes no gameplay behavior from Combat2CZ.

The recheck found an additional stale regression in `test_combat2aj_staging_contract.py`: the test still required old wording (`remain unstaged` / `git diff --cached --quiet`) even though the build script had already moved to a stricter explicit `UNSTAGED_TRACKED` fail-closed guard. The test now validates the actual staging invariant instead of obsolete prose/implementation spelling.

Two older revision-marker assertions were also hardened: GETUP recovery now checks the countdown/ownership semantics instead of a `Combat2CD` comment marker, and streamed ANIM VM verification checks the DragonFS/runtime structure instead of a `Combat2CG` generated comment marker.

This revision also re-runs the package manifest, Python syntax checks, shell syntax check, archive integrity, and the complete self-contained pre-clone preflight chain.
