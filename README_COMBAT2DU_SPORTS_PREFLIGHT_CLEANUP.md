# Combat2DU — Sports preflight manifest cleanup

Combat2DU preserves Combat2DT gameplay/runtime behavior.

The DT Termux preflight failed before repository integration because `tests/test_sports_override.py` duplicated an obsolete SHA-256 for the round-trip companion `SPRTBK.BDB`. The bundled Sports override itself is the authoritative source package and already sits under the package-wide SHA256 manifest.

DU removes the duplicate hard-coded checksum table. `assets/fix39_sports_override/SHA256SUMS.txt` is now the single local manifest for the four Sports source-slot files, regenerated from the exact bundled bytes; the regression requires exactly those four entries and verifies every file against that manifest. Runtime-consumed `SPORTLO8.IMG` and `SPORTBK.IMG` are unchanged from DT. No combat, attract, renderer, or asset-conversion behavior is changed.
