# Combat2DI — host/N64 build-graph convergence

Combat2DI fixes a source-ownership defect in the integrator rather than adding another combat approximation.

Earlier Fix39 integration copied the newer translated modules into `src/fix39`, but `patch_makefile()` suppressed every Fix39 C file whose basename already existed under `src/core/arcade`. The N64 ROM therefore continued compiling the older FIX38 copies for combat, DRONE, REACT1-9, wrestler modules, and attract-core/adapter. `patch_cmake()` also left most of those older modules in the host static-library source list, allowing archive resolution to select stale objects.

DI makes one rule authoritative for both build systems: when `src/fix39/<name>.c` exists, that module owns the basename and the matching `src/core/arcade/<name>.c` entry is removed from both CMake and the N64 Makefile source lists. Non-overlapping baseline modules remain untouched. The old files remain in the repository only as unbuilt historical/reference copies.

A new `tools/fix39_build_graph_audit.py` verifies the real post-integration tree. It requires the complete combat/DRONE/REACT/wrestler/attract owner set to come from `src/fix39` in both CMake and the N64 Makefile, rejects any duplicate basename owned by both trees, and verifies `$(FIX39_C)` is actually part of the N64 `C_FILES` graph. Termux runs this immediately after integration and GitHub Actions runs it again before host/N64 compilation.

This revision intentionally changes build ownership, not arcade gameplay logic. Its purpose is to make the already-ported Fix39 combat code actually reach the ROM.
