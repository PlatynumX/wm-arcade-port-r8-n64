# WrestleMania Arcade -> N64 Ring Chunk 3

Chunk 3 ports the source bodies that were newly located in `WRESTLE2.ASM`
and `SPECIAL.ASM`.

## New modules

### `wmania_ring_climb.c/.h`
Direct source behavior for:
- turnbuckle climb eligibility and facing
- bottom/top climb out
- bottom/top climb in
- side climb out
- side climb in
- zombie top roll-through
- continuous-stick `idiot_check`
- live-opponent-outside test
- wrestler-specific source animation labels
- rotate-animation continuations

### `wmania_ring_out.c/.h`
Direct source behavior for:
- signed `RING_TIME`
- crossing in/out
- seven-second-source-unit ring-out damage threshold
- every-eighth-PCNT health drain
- sleeping-drone PTIME adjustment
- ring-out death/disqualification
- teammate/zombie suppression
- dufus warning predicate
- delayed -150 ground-hit helper

## Engine boundaries kept explicit

No N64 animation engine is invented. The climb result says which original
animation label is selected and whether the source first runs a rotation
animation with a CODE_ADDR continuation.

`calc_line_x` is still an external collision-system dependency; side-in
takes its exact translated result as input.

`ck_climb_out_side` contains a real suspicious source memory read after a
helper that trashes A0. That is surfaced as a source-quirk read callback,
not corrected based on intent.

`adjust_health`, process creation, disqualification process creation and
winner announcement remain adapter/main-engine responsibilities; the exact
conditions and requested deltas are ported here.

## Still being hunted

`keep_onscreen`
