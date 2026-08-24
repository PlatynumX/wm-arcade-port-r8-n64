# Fix39 V13 — source-completion wiring status

V13 is based on the hardware-approved V12i package and integrates the
source-backed Combat Completion Handoff while preserving the existing direct
Fix39 implementations.  The rule remains: no guessed services, no substitute
move IDs, no hand-authored AI probabilities, and no second implementation
running beside an existing direct port.

## Source-backed and wired in V13

### Live match core

- `WRESTLE2.ASM` `wrestler_veladd` is in the live Fix39 wrestler tick.
- `WRESTLE.ASM` `wrestler_friction` is in the live Fix39 wrestler tick.
- Exact arena/ring line interpolation, trapezoid containment and `INRING`
  field semantics are available and used before movement.
- Existing eight dedicated direct wrestler modules remain the character
  dispatch owners.
- Existing complete rope command/process interpreter remains live.
- Existing source RNG (`RAND`/`RNDRNG0`) remains the sole random service.

### ATTRACT

- V12i HSTD/tips/copyright/AAMA rendering and source timing are preserved.
- **Project override:** `show_sports_logo` remains scheduled in the active
  attract cycle because this screen will be repurposed later. Its existing
  source-backed renderer/adapter stays authoritative.
- Both source `show_gameplay` positions remain in the schedule.
- Exact gameplay-demo setup is translated: source wrestler random selection
  including spare-index skip, ladder selection, round/match fields, 3-second
  warmup, 3-second freeze, 1-second freeze hold, 32-frame fade, credit flag and
  sound-suppression rule.

### Match completion/progression source logic

- Post-match route selection.
- Two-player pregame/Royal Rumble branch semantics.
- Royal Rumble win/loss orchestration plan.
- Finale orchestration plan.
- `GAME_BEATEN` orchestration plan and source score-request metadata.
- Pin-speed context/action gates.
- Match music command selection and source voice gates.
- Wrestler ending-story orchestration and source-space placements/timing.
- Fireworks finale plan, 22 flare events and the 61-point source camera path.

### High scores

The existing Fix39 HSTD implementation remains authoritative.  V13 exposes
its direct result-entry adapters for:

- win streak;
- pin speed;
- World/Intercontinental beaten masks;
- tag-team match time;
- final initials commit.

The factory tables, checksum/validation behavior, initials editor and special
beaten/tag insertion routines are not duplicated or replaced by the completion
handoff.

## Still intentionally unbound

These are not “missing source files” anymore in many cases; they are target
adapter/backend seams required to execute the translated source faithfully.

### Match engine blockers

- **Animation/frame interpreter:** real animation object commands must update
  animation state, `OBJ_FRICTION`, hurt/attack boxes and command side effects.
  V13 deliberately does not invent `OBJ_FRICTION`; therefore friction is a
  source-correct no-op until that field is driven by the animation backend.
- **Collision boxes / `COLLIS.ASM`:** cannot run the real hit pass until the
  current source animation frame supplies the corresponding boxes.
- **DRONE Stage25 live binding:** V13e-c1/c2/c3 now bind the literal source
  scalar tables (`blkbase_t`, `blkatk_t`, headhold/headheld skill delays),
  `RNDRNG0`, exact `wnshort_t` / `wnmed_t` / `wnlong_t` mode/script-list data,
  decoded script bodies, and command-2 skill tables. Remaining here are the
  distinct source `rnd`, `drone_seek`, and command-5 / EXGPC code-call targets.
  Full DRONE stays gated until those exact services resolve; no substitute
  probabilities, scripts, seek behavior, or fallback actions are used.
- **Health/damage/ring-out services:** bind real health, `RING_OUT_ON`, operator
  state and match termination before enabling source ring-out damage.
- **Special/projectile spawning:** source animation opcodes must create the
  existing translated special-object processes; no timer-based fake spawns.
- **Keep-onscreen platform state:** the source routine exists, but the live
  app must supply actual `WORLDTLX`, `OLD_PSTATUS`, meter-save fields and
  climbing state before it can be inserted at its exact pre-velocity callsite.
- **Rope visuals:** rope process state is live; N64 object/image rendering of
  source rope frames is still an adapter seam.

### Completion/progression execution

The post-match/Rumble/finale/GAME_BEATEN/story/fireworks plans are compiled and
callable, but the current frontend app still needs to execute those plans at
its live match-completion/progression state boundaries.  Do not wire these by
inventing app state numbers; bind them against the actual current frontend
state machine.

### ATTRACT presentation still pending

- `CRD_SCRN2` credits presentation.
- Wrestler bios and bio-tip presentation/assets/music/transitions.
- Operator custom message backed by actual cabinet/CMOS `CUSTOM_MESSAGE` data.
- Time/date backed by the arcade clock/DIP semantics; do not substitute Android
  or N64 wall-clock text.
- Live `start_match` / match-end / return-to-attract handoff for the two source
  gameplay demos.  The exact setup plan is ready, but running it before the
  match blockers above are solved would be a fake/incomplete demo.
- Remaining common-library transitions (`OPEN_SCREEN_LINE`, pixel wipe/fade
  details and related object/text lifecycle) where not already source-bound.

### Audio

Source sound/DCS command numbers are preserved where known, but a verified
mapping into the N64 audio/sample backend is still required for attract,
combat, voice and finale parity.  No label-to-sample guesses are made in V13.

### Persistence

The HSTD serializer/backend interface already exists, but no N64 nonvolatile
medium has been selected/bound yet.  EEPROM/FlashRAM/SRAM/Controller Pak is not
assumed.  Session high scores work; reboot still restores factory data.

## What the next integration pass should target

The highest-leverage order is:

1. bind the real animation/frame interpreter and collision-box output;
2. finish DRONE C4 by binding source `rnd`, `drone_seek`, and code-call targets;
3. bind health/damage/ring-out plus special-spawn animation commands;
4. insert keep-onscreen using real frontend camera/meter state;
5. run an actual CPU-vs-CPU match through the translated match engine;
6. connect that match handoff to both ATTRACT gameplay calls;
7. execute post-match/progression/finale plans from the live app state machine;
8. finish credits/bios/operator/time-date/audio and nonvolatile HSTD storage;
9. multi-loop hardware torture/regression testing.

At that point the work is primarily parity/debugging rather than missing major
source-game systems.
