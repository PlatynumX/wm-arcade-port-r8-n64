# Fix39 V12 integration status

## V12 ATTRACT presentation checkpoint

V12 preserves the V11b ATTRACT.ASM ownership/timing work and now binds source-backed N64 renderers for designer hints, general tips, both copyright pages and AAMA. Visible text is generated from ATTRACT.ASM; WIMP pixels/palettes are generated from the historical IMG set; slateBMOD is packed through the existing BMOD pipeline. HSTD, credits, bios/bio-tips, operator/time-date presentation, attract audio parity and real gameplay demos remain pending. See `FIX39_V12_REMAINING.md`.


## Bundle verification

The merged source set contains the combat/DRONE stack, eight wrestler modules,
REACT1-9, attachments/specials, ring geometry/climb/ring-out, the complete rope
program corpus, RAND/RNDRNG0, HSTD, non-gameplay attract code and the
ROM-recovered `keep_onscreen` translation.

`tests/run_fix39_smoke.sh` compiles all Fix39 C modules as C11 with
`-Wall -Wextra -Wpedantic -Werror`.  V9 passes with both GCC and Clang.

## Live wiring carried into V9

- the translated attract-cycle builder owns top-level attract screen order
- shared RAND/RNDRNG0 owns the frontend RNG path
- HSTD owns the recent-champion data consumed by the progression screen
- MATCH_INIT creates the exact first source 1v1 actor seeds
- both wrestler actors enter `wm_arcade_move_wrestler` every source tick
- that dispatcher enters the dedicated source module for all eight wrestlers
- Bret/Razor source `ani_init` entrypoints run
- exact attachment helpers are connected to wrestler callbacks
- all four rope process banks tick every source tick
- the complete rope command -> source-program path is callable
- combat and special runtime state is initialized at match start

The smoke test proves all eight frontend wrestler IDs enter the live direct
character dispatcher and that the exact rope program resolver executes.

## V9 N64 namespace fix

V8 reached the host build but failed when `src/core/app.c` included both the
repo rope API and the direct-port rope API. C enum members share translation-
unit scope, so names such as `WM_ROPE_COMMAND_COUNT` were redefined. V9
namespaces every public direct-port rope constant as `WM_FIX39_ROPE_*`. No
source numeric value, command routing, script table, timing, or rope behavior
was changed. The namespace regression now deliberately predeclares the old
`WM_ROPE_*` names before including the Fix39 rope headers.

## Verified keep_onscreen order

The recovered WRESTLE.ASM loop order is:

1. `update_joystat`
2. `count_button_presses`
3. `keep_onscreen`
4. `wrestler_veladd`
5. `wrestler_friction`
6. `animate_wrestler`
7. collision-box work

`wm_fix39_keep_onscreen_before_velocity()` is the exact bridge. It remains
gated on real `WORLDTLX`, `OLD_PSTATUS`, `ALLOW_OFFSCRN`, climbing and
meter-process state rather than being called with made-up values.

## Why the match is not visually playable yet

The character decision code is now live, but the shared source movement,
animation-frame, collision-box and gameplay-rendering services are not yet
ported.  Therefore V9 deliberately does not fabricate movement or use the
existing development match renderer as a substitute.

See `FIX39_V9_REMAINING.md` for the complete blocker list.
