# r8h6 — source title/background translation

r8h6 continues the strict source-port contract. It does not restore the old hand-built frontend.

## Source changes in this revision

- `show_title` is now a `partial-source` routine instead of being skipped.
- Its setup sleeps, unblank point, half-second button lockout, 10-second `wait_on_butn` interval, and `cycle_lava` five-tick scheduler are represented in the portable core.
- `NTITLESCBMOD` is recovered from the original Midway artist data by reading the `NTITLESC` source block records from the matching BDB and the final module dimensions from `BGNDTBL.ASM`.
- The N64 renderer applies the same 400x256 arcade-coordinate transform used by the other frontend assets; it does not resize the title background to fit 320 pixels.
- `show_gameplay` remains harness-only and cannot be entered through normal attract flow.

## Deliberately still absent

- `cycle_lava` per-background-block palette substitution.
- `SPRINKLE_GLINTS` and `RANDOM_SPARKLE` object/process rendering.
- General BMOD/BGD block renderer.
- Original `start_match`, credit/start/select, credits, hints, bios, audio and remaining attract routines.

Those stay absent until their original implementations/dependencies are translated. No approximation is substituted.

## Build/source handling

The failed source-ZIP detour is removed. CI fetches the historical source tree directly, generates source-derived C assets, compiles them as a verification step, then builds/publishes `wm_arcade_r8h6.z64`.
