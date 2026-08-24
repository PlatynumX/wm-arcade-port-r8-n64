# Fix39 V11 — live wiring and remaining source work

V11 changes the integration rule from **compiled into the ROM** to **entered by
live program flow whenever the supplied translation has all of the state it
needs**.  A translated subsystem that still depends on an unported arcade
service is connected at an explicit boundary and remains gated there; V11 does
not invent the missing service, data table, pointer token, camera value, or
operator adjustment.

## Live in V11

### Shared RNG

- `RAND` is advanced from the 53 Hz source loop through the translated
  `wm_rng_mainloop_step` path.
- title sparkle randomness uses translated `RNDRNG0`, replacing the old local
  frontend RNG bridge.
- DRONE's different source `rnd` dependency is **not** silently aliased to
  RNDRNG0.

### Top-level attract scheduling

- `wm_attract_build_cycle()` is the sole top-level ATTRACT.ASM sequence owner.
- V11 removes the old `wm_attract_call_port_status()` skip gate from live
  attract flow; Fix39 itself classifies every source step as runnable or
  pending a named dependency.
- Existing DCS/Sports/title source-backed frontend tickers remain runnable.
- Designer hints, general tips, copyright and AAMA now have translated
  source-control/timing state machines behind explicit platform capability
  bits.  Capabilities default off until their exact renderer/effect adapter is
  present, preventing black-screen stalls.
- `NUM_HINTS` is corrected to the source value 10, with exact WHICH_HINT order,
  number-art symbols and body-line counts.
- The two gameplay-demo positions remain explicit placeholders.

### High-score source data/system

- factory HSTD tables initialize and validate at runtime.
- PROGRESS.ASM's visible "recent champion" lookup is now sourced from the live
  HSTD BEATEN/INTER table instead of the N64 frontend's duplicate factory
  strings.
- the start/continue reset counter remains gated until the actual operator
  adjusted reset value is ported.

### Match entry and all eight wrestler modules

`MATCH_INIT` now starts a source-seeded two-actor match state and every source
tick enters:

`wm_arcade_move_wrestler()` -> `wm_arcade_move_ported_wrestler()` -> the
selected wrestler's dedicated translated module.

This is active for:

- Bret Hart
- Razor Ramon
- Undertaker
- Yokozuna
- Shawn Michaels
- Bam Bam Bigelow
- Doink
- Lex Luger

P1's current/down/up button and joystick state is fed into the arcade actor.
P2 is dispatched too, but V11 supplies no made-up AI input.

Bret/Razor `ani_init` entrypoints are executed.  For all eight wrestlers,
translated animation, torso-animation, sound, queued-special and external
special labels/tokens reach an explicit runtime trace/adaptor boundary.
Exact attachment helpers (`keep_attached` and `master_keep_attached`) are live.

### Ring/rope material that is complete

- source ring constants, boundary seeds and start coordinates are the live
  match seeds.
- all four translated rope process banks are created at match start and ticked
  once per source tick.
- `wm_fix39_rope_apply_command()` routes an exact rope command through
  `wm_rope_resolve_command()` and the complete direct ROPES.ASM program
  resolver/interpreter.
- ROM-recovered `keep_onscreen` is exposed at the verified source boundary
  immediately before velocity integration.  It is deliberately not invoked
  with fabricated camera/meter state.

### Combat/reaction/special infrastructure

- combat runtime and special-object lists are initialized as part of the live
  match.
- REACT1-9, collision, damage and projectile implementations are linked and
  callable from the same live runtime.
- they are not fired against fake zero hurt boxes or fake spawn opcodes; the
  missing animation/collision services below must supply those inputs first.

## Still needs ported or connected

### 1. Shared WRESTLE.ASM per-player loop services

These are the main blocker between character decisions and visible movement:

- exact `update_joystat`/run behavior beyond the current raw N64 input latch
- `count_button_presses` and the source secret-move history/process scheduler
- `wrestler_veladd`
- `wrestler_friction`
- `animate_wrestler`
- source animation pointer/label resolution and continuation (`CODE_ADDR`)
- source animation command dispatch and frame advance

The required order remains:

`update_joystat -> count_button_presses -> keep_onscreen -> wrestler_veladd -> wrestler_friction -> animate_wrestler -> collision`

### 2. Camera / keep_onscreen live inputs

The ROM-recovered function still needs the real live owners of:

- `WORLDTLX`
- `OLD_PSTATUS`
- `ALLOW_OFFSCRN`
- each player's `CLIMBING_THRU`
- meter-process saved A8/A9/A10 registers

Once those are translated, `wm_fix39_keep_onscreen_before_velocity()` can be
called at its already-verified position without approximation.

### 3. Animation boxes and wrestler combat

The translated combat code needs the source animation backend to produce:

- current IANI3 hurt-box data
- attack-box enable/disable and offsets from animation opcodes
- valid animation counters/modes

Then the live loop can safely run `wm_arcade_check_wrestler_collisions()` and
the complete damage/REACT1-9 chain.  It also still needs exact shared services
for health/life, pin/death/winner state, reversals, raise-arm, bonuses/first-hit
and related process callbacks used by the reaction modules.

### 4. Ring climb / crossing / ring-out shared services

The translated climb/ring-out modules still need:

- source `get_rope_x`
- source `calc_line_x`
- real collision-line values and turnbuckle collision entrypoints
- the source post-`any_opp_outside` quirk read where required
- source `RING_OUT_ON` operator state
- health adjust carry/result, DQ creation and round-winner process services

The climb animation labels are already translated, but must be handed to the
same source animation interpreter listed above.

### 5. Rope gameplay and rendering adapters

The 70-program ROPES.ASM interpreter is live, but normal wrestler contact still
needs the source service that decides the exact:

- rope bank
- action
- selector
- wrestler-Z input

The N64 renderer also needs an adapter from the source rope image symbols/image
pairs to the actual rope objects.  V11 does not guess either mapping.

### 6. Specials/projectiles

Constructors and process-state/collision logic are translated for:

- Doink pie
- Bam Bam fireball
- Undertaker spirit/reaper
- Yokozuna salt

What remains is the source animation/script command path that actually creates
those processes at the correct frame, plus the shared process scheduler,
renderer/audio object adapters and valid wrestler collision boxes.  Once those
exist, the existing special collision path can feed the translated reaction
stack.

### 7. DRONE / CPU opponent

Stage25 still has source-addressed dependencies that must be translated rather
than invented.

Raw tables:

- `blkbase_t`
- `blkatk_t`
- `sklhhdly_t`
- `sklhrdly_t`
- `wnshort_t`
- `wnmed_t`
- `wnlong_t`

Named scripts/services include:

- `slhtoss`
- `drn_enterring`
- `drn_opinair`
- `drn_oprun`
- `drn_roll`
- `drn_inair`
- `drn_ontb`
- `drn_run`
- `drn_combo`
- `M_shrtblkr`
- `M_shrtblkrdl`
- `drn_seekclose`
- `drn_oppdead`
- wrestler/range-specific script tables
- `script_skill_pct`
- `script_call`
- `script_seek`
- the source `rnd` service (distinct from `rndrng0`)

Until these are resolved, P2 remains a live wrestler process with no fabricated
CPU decisions.

### 8. High-score persistence, entry and presentation

Already translated: table logic, factory data, insertion/counter code,
serialization, initials-entry logic and renderer-neutral presentation rows.

Still needed on N64:

- actual save backend (EEPROM/SRAM/FlashRAM policy and callbacks)
- operator `GET_ADJ` reset-counter value
- HSTD attract renderer/font/background asset bindings
- initials-entry screen renderer/controller/audio adapter
- match-result hooks for streak, beaten wrestlers, pin time and tag records

### 9. Remaining attract-mode work after V11

V11 now has translated source control/timing for six previously inactive calls:
**designer hints, general tips, operator message, time/date, copyright and
AAMA**. Exact hint/general/copyright/AAMA strings are generated directly from
ATTRACT.ASM during the build. None of these new calls is advertised as N64-
runnable until its exact presentation/backend capability is bound.

The remaining attract work is explicit:

- **Designer hints:** bind OSGEMD/WSF10, `hstd_mod`, `MVEBAR_R`, `SHADOW01`,
  mug/tip/number art and exact `OPEN_SCREEN_LINE 18,6` behavior; then enable
  `WM_FIX39_ATTRACT_CAP_DESIGNER_HINT`.
- **General tips:** bind source fonts/background/bar/shadow and exact
  SET_UP_PIXEL_WIPE -> BLOW_0_TO_1 -> RESET_FROM_PIXEL_WIPE behavior; then
  enable `WM_FIX39_ATTRACT_CAP_GENERAL_TIPS`.
- **Copyright:** bind exact two-page source text, RD7FONT, SMWWF2 and source
  fade/unblank/object lifecycle; then enable `WM_FIX39_ATTRACT_CAP_COPYRIGHT`.
- **AAMA:** bind exact six source strings, RD7FONT, translated 63-row gradient
  and fade/unblank lifecycle; then enable `WM_FIX39_ATTRACT_CAP_AAMA`.
- **Operator:** connect the already-translated optional-message branch to real
  `CUSTOM_MESSAGE`, `CMESS_LINES`, `CMESS_LINE_SIZE`, SPORTBKBMOD, BALLD05A,
  `osgfont_t`, hscore palette cycle and scaleout/WIPEOUT; then enable
  `WM_FIX39_ATTRACT_CAP_OPERATOR_MESSAGE`. No fabricated operator text.
- **Time/date:** connect the translated optional-DIP branch to source DPTDON_B
  semantics, the real clock service and ogmd10/wsf14/wgsf24 text presentation;
  then enable `WM_FIX39_ATTRACT_CAP_TIME_DATE`. No host clock substitution.
- **HSTD:** connect the already-translated high-score presentation rows to the
  N64 source font/background, pixel wipe and attract transition.
- **Credits:** port/bind external `CRD_SCRN2`.
- **Bios/bio tips:** bind source selection/data to exact art, text, palette and
  music presentation.
- **Audio:** bind each attract routine's source music/sound calls and teardown.
- **Start/coin interruption:** final source-order audit after visible calls are
  enabled.
- **Loop regression:** repeated full-cycle/reset/return-to-attract testing.

Gameplay demo code was intentionally absent from the supplied attract bundle.
For literal 100% arcade attract mode, both demo call sites still need their
source match initialization, CPU/input behavior, termination/freeze/fade and
return-to-ATTRACT path ported. ATTRACT.ASM shows these are real `start_match`
demos; V11 does not replace them with prerecorded/scripted substitutes.

### 10. Source-backed gameplay renderer

The current N64 frontend intentionally renders `MATCH_INIT` as black, while its
existing `render_match()` is a development harness.  A real match therefore
still needs the source animation/frame/object renderer and its wrestler/ring/
special asset mappings.  V11 does **not** route the harness into normal play,
because doing that would substitute a recreation for the arcade source path.

### 11. Process/scheduler glue used by combat

Still required where referenced by the translated wrestler/reaction code:

- source charge/secret monitor process creation and lifetime
- continuation/code-address scheduler semantics
- GETUP process/event integration (`GETUP_PID 0x12B`)
- pin/death/DQ/winner/raise-arm process creation and lookup

### 12. Match audio label/ID backend

The character modules now emit their exact source sound labels/tokens into the
live adapter boundary.  What is still missing is the verified mapping from
those source labels/IDs to the already-porting DCS/N64 audio backend.  V11 does
not invent sound-number aliases.

## Bottom line

V11 makes ATTRACT.ASM the sole attract ownership layer and gives every supplied subsystem with complete inputs **enter live runtime flow**.  The remaining inactive pieces are inactive for a specific, named
missing arcade dependency—not because they were merely forgotten after being
compiled.
