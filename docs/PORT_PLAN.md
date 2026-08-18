# Source-first whole-game translation plan

The arcade source is the program.  The N64 port must translate that program;
it must not invent an equivalent menu/game shell around translated assets.

## Architecture boundary

- `src/core/`: portable translations of original game processes, control flow,
  state, animation, collision, match logic and frontend routines.
- `src/generated/`: data extracted/reproduced from the historical source tree.
- `src/platform/n64/`: controller mapping, RDP rendering, audio/storage backends
  and other N64 hardware adaptation only.
- `tools/` + `scripts/`: parsers/converters that make the original source the
  reproducible input to the portable translation.

If an arcade routine is not translated yet, it is left unimplemented/skipped in
bring-up builds.  It is **not** replaced with a hand-authored screen or behavior.

## Translation order

1. **Original program/control flow** — translate the active attract/game/start
   process graph and its waits, process creation, input gates and transitions.
   Generate/check source call tables directly from the ASM where practical.
2. **Original frontend/background object system** — `BAKMODS`, `BGND_UD1`, WIMP
   objects, source fonts/messages, palette fades/cycles and wipes.
3. **Credit/start path and player select** — trace the real caller into
   `select_screen`; do not wire N64 Start directly to a made-up select menu.
4. **Match process graph** — translate `start_match` and the actual wrestler,
   drone, collision, rope, camera, HUD and round processes.
5. **Roster locomotion + attacks** — all eight wrestler modules and their own
   art/sequence/character logic.
6. **Animation VM coverage** — every reached `ANI_*` command with source-data
   regression tests.
7. **Damage/collision/reactions** — attack volumes, targeting, block results,
   stun/dizzy, knockdowns and recovery.
8. **Grapple/rope/special systems** — grabs, throws, reversals, pins/falls,
   ropes/outside-ring/turnbuckles, combos and finishers.
9. **Ladder/match progression/awards/endings** — preserve original state and
   process sequencing.
10. **DCS audio semantics on an N64 backend** — preserve game-side commands while
    replacing only the physical sound-board implementation.

## r8h4 correction

r8h4 removes the hand-built `Midway -> title -> select -> match` state machine.
`tools/attract_sequence.py` now extracts the active `JSRP` order from
`ATTRACT.ASM::attract_mode`, and the portable core executes that order.  The
original conditional `AMODE_LOOPS` tail is translated as control flow as well.

Currently translated attract handlers are `DCS_LOGO` timing/state,
`show_sports_logo` timing/world movement, and the timing wrapper around the
current partial `start_match` gameplay translation.  Other source routines are
skipped until their real implementations/assets are ported; no substitute title,
character-select, credits, tips or bios screens are shown.

## r8h5 strict-source enforcement

The normal program path may execute only routines marked `partial-source` or
`exact-source` in `port/translation_manifest.json`. `harness-only` code is never
considered a ported routine. In particular, the r8 portable combat demo remains
useful for isolated N64 renderer/input testing, but no longer implements
`ATTRACT.ASM::show_gameplay` in the product flow.

CI publishes a source-text bundle from the historical tree so remaining routines
can be translated from their actual bodies/data rather than reconstructed from
screenshots or expected behavior.
