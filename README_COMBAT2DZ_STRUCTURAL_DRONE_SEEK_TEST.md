# Combat2DZ structural DRONE seek regression

Combat2DY integrated the source-exact `DRONE.ASM` seek-dir/dist patch correctly but the retained Combat2BL regression still failed on a historical comment string in `wm_fix39_runtime.c`. Combat2DZ changes that regression to inspect the active `drone_seek_source_target` / `drone_seek_source_joy` function bodies and verifies that the authoritative Fix39 runtime is present in both host and N64 build graphs. Comment wording is no longer a correctness oracle.

A new post-integration test-contract sweep also rejects negative assertions against plain-English prose in tests invoked with `$WORK`, preventing this same stale-comment failure class from returning. No gameplay, WIMP, DRONE, ownership, or rendering behavior changes from Combat2DY.
