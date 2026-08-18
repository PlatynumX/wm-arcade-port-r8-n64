# r8h6 — source title/background translation

r8h6 continues the strict source-port contract. It does not restore the old hand-built frontend.

## Source changes in this revision

- `show_title` is now a `partial-source` routine instead of being skipped.
- Its setup sleeps, unblank point, half-second button lockout, 10-second `wait_on_butn` interval, and `cycle_lava` five-tick scheduler are represented in the portable core.
- `NTITLESCBMOD` now uses the original paired `BIGWWF.BDB`/`BIGWWF.BDD` BLIMP data: 40 raw CI8 source blocks, their original RGB555 palettes, and the compiled placement/depth/transparent records from `BGNDTBL.ASM`.
- The N64 renderer applies the same 400x256 arcade-coordinate transform used by the other frontend assets; it does not resize the title background to fit 320 pixels.
- `show_gameplay` remains harness-only and cannot be entered through normal attract flow.

## Deliberately still absent

- `cycle_lava` per-background-block palette substitution.
- The original `SPARKLE.IMG` frame set and `show_title` sparkle process lifetime are now translated in r9. Exact `SPRINKLE_GLINTS` / `RANDOM_SPARKLE` placement and cadence remain pending because their shared helper bodies and `WHERE_WRESTLMANIA_SPARKLES` table are referenced externally but are not present in the checked-in WWF source tree.
- Generalize the now-working NTITLESC BDD/BMOD block renderer to the remaining backgrounds.
- Original `start_match`, credit/start/select, credits, hints, bios, audio and remaining attract routines.

Anything still source-missing stays explicitly marked as such; provisional behavior is not claimed as source-exact.

## Build/source handling

The failed source-ZIP detour is removed. CI fetches the historical source tree directly, generates source-derived C assets, compiles them as a verification step, then builds/publishes `wm_arcade_r8h6.z64`.
