# Combat2CO — ATTR.ASM single gameplay authority

Baseline: Combat2CN.

This pass removes the remaining attract gameplay ownership split.  `SHOW_GAMEPLAY`
now enters the translated Fix39 match runtime and ticks that runtime only.  The old
`wm_demo_tick` CPU simulation, presenter-derived attack/collision binding, demo
reset/roster gameplay setup, and presenter-to-gameplay state mutation are forbidden.

The base generator and the later combat completion patch both carry the same
single-authority contract so rerunning an older patch stage cannot resurrect the
harness.  `tools/fix39_attract_ownership_audit.py` runs against the final generated
working tree after all combat patchers and fails the build if the forbidden paths
return.

Rendering may still use `wm_demo_fighter` as a presentation-shaped temporary, but
world X/Z/facing/source frame are projected/read from `wm_fix39_actor` and the live
source animation runtime. Presentation state is not gameplay input.
