# Combat2DM — selective source ownership

DL proved the N64 ROM is now compiling Fix39, but DI-DL used an over-broad rule: every basename overlap was replaced by Fix39. That also displaced newer working frontend/attract/high-score/ring/rope/roster/RNG modules.

DM changes the build graph policy, not combat behavior:

* Fix39 owns only the translated combat stack: combat, drone, react1-react9, animation/attach/move dispatch, special, wrestler port, and the eight wrestler modules/tables.
* `src/core/arcade` remains authoritative for attract/frontend, high-score, ring/rope, roster, and RNG overlaps.
* Unknown future overlaps fail the semantic audit instead of silently choosing an owner.
* Host CMake and N64 Makefile use the same selective policy.
* A regression proves combat overlaps switch to Fix39 while the known-good attract/presenter ownership remains core.

This is intentionally the ownership-path correction after DL showed frozen CPU-vs-CPU attract combat and a Bret-vs-Bret roster regression.
