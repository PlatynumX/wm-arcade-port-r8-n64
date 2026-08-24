# Combat2DO — legacy ownership reconciliation

Combat2DO preserves Combat2DN gameplay/runtime behavior and the dependency-closed ownership policy.

The DN Termux run failed before clone because `tests/test_v13_completion.py` still asserted the pre-DN ownership of `wmania_ring_geometry.c`. DO reconciles the retained historical ownership tests with the current source-backed provider model:

- Fix39 owns `wmania_ring_geometry.c`, `wmania_rng.c`, and `wmania_attract_core.c` because the selected Fix39 combat runtime directly depends on the services they provide.
- Core continues to own the attract adapter/presenter, roster, high-score and rope/frontend paths.
- Historical DI/DM/DK ownership regressions remain in the suite, but now validate the current dependency-closed policy instead of superseded ownership decisions.
- A new `test_combat2do_legacy_ownership_reconciliation.py` runs the retained ownership regressions together and fails if they disagree with the current policy.
- The DO reconciliation test is run before the repository clone/build, so stale ownership assertions fail early.

No intentional gameplay behavior changes from DN are introduced by DO.
