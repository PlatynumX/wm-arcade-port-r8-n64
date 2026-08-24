# Combat2CY preflight repair

Combat2CX was launched correctly. Its run stopped in package preflight before compilation because `test_combat2cv_late_staging_single_authority.py` still asserted the old Combat2CW diagnostic text while the current build script emitted the Combat2CX diagnostic. That was a stale regression-test string, not another gameplay/runtime failure.

Combat2CY updates the regression to the current CY diagnostic and explicitly runs `test_combat2cx_callback_prototype.py` from the Termux build script so the declaration-order fix that CX introduced is verified before the repository clone/build. No gameplay behavior is intentionally changed.
