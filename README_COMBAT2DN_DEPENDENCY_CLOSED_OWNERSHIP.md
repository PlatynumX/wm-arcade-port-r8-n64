# Combat2DN — dependency-closed selective ownership

Combat2DM proved that selecting Fix39 combat while preserving all ring/RNG/attract overlaps in `src/core/arcade` was not link-closed. The translated combat runtime requires four services that the current core copies do not define:

- `wm_ring_calc_line_x` — `src/fix39/wmania_ring_geometry.c`
- `wm_ring_inring_field` — `src/fix39/wmania_ring_geometry.c`
- `wm_rng_rnd_mask` — `src/fix39/wmania_rng.c`
- `wm_attract_demo_plan_make` — `src/fix39/wmania_attract_core.c`

DN therefore keeps the selective-ownership strategy but moves only those three shared provider modules to Fix39 authority. `wmania_attract_adapter.c`, `wm_arcade_roster.c`, high-score, rope, and the rest of the frontend/presenter path remain core-owned.

## Arcade-source comparison

The provider choice is behavioral, not just a linker fix. The bundled Fix39 implementations were translated against the original Midway sources:

- `WRESTLE.ASM` defines the ring boundary lines and uses `ARE_WE_IN_RING`; the Fix39 ring geometry computes the trapezoid line X and returns the same inside/outside field convention consumed by the translated runtime.
- The arcade `RNDRNG0` family mutates the global random state before reducing it to the requested range/mask; `wmania_rng.c` preserves that stateful ordering.
- `ATTR.ASM::show_gameplay` selects the player with `RNDRNG0(7)`, initializes/randomizes the ladder, then chooses battle `RNDRNG0(5)+1`. `wm_attract_demo_plan_make` preserves that order, including the ladder RNG consumption between the two visible selections.

A new generated-tree audit fails before compilation unless all four required services have exactly one active provider and that provider is the source-backed Fix39 module in both CMake and the N64 Makefile.
