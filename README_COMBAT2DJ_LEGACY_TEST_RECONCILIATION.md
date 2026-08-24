# Combat2DJ — legacy-test reconciliation

Combat2DI changed host/N64 build ownership so `src/fix39` wins every basename overlap with `src/core/arcade`. The DI Termux preflight still carried legacy tests that required the superseded `BASELINE_OVERRIDES` / `difference_update()` implementation.

DJ removes that dead exception-list constant and rewrites the affected guards around the actual invariant: overlapping baseline arcade modules are removed from both build graphs and the Fix39 copy is authoritative. A dedicated regression rejects reintroduction of the stale exception-list scheme. No gameplay behavior is intentionally changed from DI.
