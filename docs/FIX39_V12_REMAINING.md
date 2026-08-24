# Fix39 V12i — ATTRACT status and remaining work

## Live through V12i

V12g preserves V11b as the ATTRACT.ASM sequence owner and keeps the hardware-
tested V12f hint renderer hardening. The N64 adapter now advertises five
source-backed ATTRACT capabilities:

1. `WM_FIX39_ATTRACT_HISCORES`
2. `WM_FIX39_ATTRACT_DESIGNER_HINT`
3. `WM_FIX39_ATTRACT_GENERAL_TIPS`
4. `WM_FIX39_ATTRACT_COPYRIGHT`
5. `WM_FIX39_ATTRACT_AAMA`

Visible strings are generated from the historical source. Required WIMP pixels
and palettes are generated from the historical IMG set and IMGPAL.ASM. The
slate background is packed through the existing BMOD source pipeline.

### HSTD in V12g

`show_hstd` is no longer capability-gated. The port presents the five source
pages in their arcade order:

1. Intercontinental Champs
2. World Champions
3. Tag Team Champions
4. Fastest Pindown Times
5. Longest Winning Streaks

The first two use the translated three-row scrolling state and the remaining
three use fixed tables. The renderer uses the existing source-backed
`WmHsSystem` factory/session tables, WSF14 rank art, WGSF18 score/time art,
source highlight palettes, `SPEAR`/`BARBUTT`, crouton icons, wrestler labels,
`MVEBAR_R`, `SHADOW01`, and `hstd_mod`/`slateBMOD`.

V12g deliberately does **not** add persistence. High scores can change for the
running session through the existing table logic, but a reboot restores the
source factory tables until an N64 save backend is connected.

### Designer hint correction

Hardware V12f proved repeated hint loops stable, but exposed that the WGSF22
number overlapped `TIP #`. V12g directly translates `PUT_UP_TIP_NAME`: the
number is anchored at `200 + designer-name/TIP# width / 2`, not from the number
glyph's own width.

### V12h hardware findings

- Fixed external-palette transparency to follow the source `DMAWNZ` CI8-index
  rule. This removes the visible gray matte from TIP-name/TIP-number art without
  editing the arcade pixels.
- Fixed the Fastest Pindown wrestler-name X origin: source uses `A9.x + 302`;
  V12g incorrectly used `A10.x + 302`.
- Do **not** fill blank lower INTER/WORLD factory rows with invented data. Those
  zero/space records are present in the historical `HSTD.ASM` ROM tables.
- Numeric values are not missing from INTER/WORLD: their score word is a
  defeated-wrestler bit/nibble mask rendered as wrestler icons and dots by
  `draw_beaten_table_entry`.


## ATTRACT work still pending

- **HSTD persistence:** bind the exact translated tables/validation/reset rules
  to an N64 nonvolatile save backend. This is not required to display or update
  the tables during the current session.
- **HSTD transition fidelity:** recover/translate the exact pixel-wipe/object
  motion lifecycle around `show_hstd`; V12g carries the source page order,
  source timing constants and visible table motion but does not claim a
  bit-perfect private/common-library transition implementation.
- **Credits:** port/bind the external `CRD_SCRN2` presentation.
- **Bios and bio tips:** bind exact source selection/data to wrestler art, text,
  palettes, music and transitions.
- **Operator message:** connect the translated optional-message path to real
  `CUSTOM_MESSAGE`/CMOS data and exact SPORTBKBMOD/BALLD05A/text/scaleout/wipe
  presentation. Do not fabricate operator text.
- **Time/date:** connect DPTDON_B and the arcade clock service plus the exact
  source fonts/presentation. Do not substitute the phone/N64 host clock.
- **Audio:** map attract music/sound source calls and teardown to the verified
  DCS/N64 backend.
- **Transitions/fidelity:** port `OPEN_SCREEN_LINE`, the general-tip pixel wipe,
  copyright fade/unblank lifecycle, and remaining common-library text metric
  behavior once those source/common routines are recovered.
- **Loop regression:** repeated full-cycle, button interruption, reset and
  return-to-attract testing on emulator and hardware.

## Gameplay demos

Both ATTRACT.ASM gameplay call sites remain the largest missing attract item.
They are real `start_match` demos and still require arcade initialization,
wrestler/AI/input behavior, termination/freeze/fade and return-to-ATTRACT logic.
V12g does not replace them with a fake scripted demo.

## Broader gameplay port

The cumulative Fix39 bundle still contains the combat/DRONE, wrestler, REACT,
ring/rope, RNG, HSTD and keep-onscreen source translations from earlier passes.
Where a translated subsystem still depends on unported scheduler, animation,
collision, rendering, audio or persistence services, that boundary remains
explicit rather than being approximated.


### V12i hardware finding / correction

- Real-hardware video showed Intercontinental/World scrolling row objects
  drawing over the fixed red title banner.
- Source audit confirms this is an N64 ordering bug, not intended arcade
  behavior: `MVEBAR_R`/`SHADOW01` are Z `0x1799`; row objects are Z
  `0x1000..0x1003`; `MOVE_ALL_OBJS_UP` skips objects above `0x1798`.
- V12i draws the fixed header after the scrolling row layer, reproducing the
  source object Z order without inventing a clipping boundary.
