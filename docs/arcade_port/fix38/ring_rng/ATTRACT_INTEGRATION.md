# WrestleMania Arcade -> N64 non-gameplay attract integration

This package **includes the complete hi-score handoff** and adds the
non-gameplay portions of the arcade attract system.

## Compile these new attract files

- `wmania_attract_core.c/.h`
- `wmania_attract_data.c/.h`
- `wmania_attract_time.c/.h`
- `wmania_attract_operator.c/.h`
- `wmania_attract_secret.c/.h`
- `wmania_attract_visuals.c/.h`
- `wmania_attract_adapter.c/.h`

Also compile the included `wmania_hiscore_*.c/.h` files from the already
completed hi-score port.

## Exact top-level source order

One ordinary cycle is:

1. hi-score tables
2. DCS logo
3. Midway Sports logo
4. **gameplay demo slot 1 — deliberately not implemented here**
5. credits (`CRD_SCRN2`)
6. title
7. **gameplay demo slot 2 — deliberately not implemented here**
8. credits (`CRD_SCRN2`)
9. rotating designer hint
10. general tips
11. rotating wrestler bio
12. same wrestler's special-moves/tips page
13. operator message if configured
14. `RemapIO`
15. increment `AMODE_LOOPS`

Every even loop then adds:
- credits
- optional time/date screen

Every eighth loop additionally adds:
- two-page copyright/music-rights screen
- AAMA parental-advisory screen
- then resets `AMODE_LOOPS` and `SOUNDSUP` to zero

`wm_attract_build_cycle()` emits this exact sequence and retains both
gameplay positions as marked no-op placeholders.

## Existing frontend callbacks

DCS, Midway Sports and the lava/title screen have already been ported in the
main N64 project. This package therefore **does not fork or recreate them**.
Wire the existing implementations into:

- `show_dcs_logo`
- `show_sports_logo`
- `show_title`

That gives the scheduler the original source ordering without creating a
second implementation that could drift.

## Gameplay demos

Per this handoff's requirement, gameplay demos are the only normal-loop
attract content intentionally absent.

`show_gameplay_demo_unimplemented` is optional. Leave it `NULL` for a pure
no-op. The two source positions remain visible in the scheduler/test suite
so a later gameplay merge can fill them without changing attract ordering.

## Hi-scores

The previous complete hi-score source bundle is physically included here.
The runner invokes `wm_hs_system_table_cmos_check()` immediately before the
hi-score callback, matching `show_hstd`.

The renderer can continue using `wmania_hiscore_present.*` for:
IC -> World -> Tag -> Fastest Pin -> Winning Streak.

## Credits

`creditscreen` in `ATTRACT.ASM` itself does not contain a separate credit
roll. It calls the shared external system routine `CRD_SCRN2` with A10=1,
then restores the process ID to the attract-mode PID.

Accordingly, `show_credits_crd_scrn2` is an **external source dependency**,
not a recreated credit screen. If the main port already has the Midway
credit subsystem, bind it directly. Do not invent a replacement list.

## Designer hints

The executable source hardcodes `NUM_HINTS = 5`. Although more hint data
exists below it, the active attract path rotates only the first five
entries in `WHICH_HINT`:

- HNTT_2 / HNT_2 + JMSTIP/JASMUG
- HNTT_4 / HNT_4 + MIKTIP/MIKMUG
- HNTT_3 / HNT_3 + MJTTIP/MRKMUG
- HNTT_7 / HNT_7 + EUGTIP/EUGMUG
- HNTT_5 / HNT_5 + SHNTIP/SHNMUG

The state preserves the source's slightly odd BSS behavior: `last_hint`
starts at zero and is incremented before selecting, so the first normal
call selects active index 1 unless something else changed that BSS earlier.

Timing/layout constants are in `wmania_attract_data.h`.

## General tips

This remains a separate fixed screen, using:
- background `hstd_mod`
- title source label `gen_tip_mes`
- `MVEBAR_R` + `SHADOW01`
- first body Y=60, 15-pixel line step
- one TSEC pre-wait
- ten TSEC button/timeout wait

The source-label table is exported in `wm_attract_general_tip_labels`.

## Wrestler bios and tips

The bio renderer model includes the exact eight-row source metadata:
halfwidth, source-from label, weight, feet/inches, quote label, logo
symbol/dimensions and wrestler tune ID.

The rotate order follows `bio_data`:
Bret, Razor, Undertaker, Yokozuna, Shawn, Bam Bam, Doink, Lex.

The original BSS `next_bio` is zero and the routine increments before
selection, so the first normal bio call selects Razor (index 1). That
quirk is preserved.

The tips screen uses the **same wrestler** immediately after the bio. The
attract loop backs `next_bio` up one, calls the same incrementing body with
`bios_type=1`, and therefore arrives at the same wrestler.

Bio music is allowed only while `AMODE_LOOPS < 2` and ADJMUSIC is zero.
The exact tune table is preserved:
`5,2,1,7,6,4,8,3`.

## Operator message

The current `ATTRACT.ASM` uses `dan_test` as the operator-message backdrop,
not the older generic display.

This bundle therefore ports its actual behavior. Random ball setup now
uses the shared direct `RNDRNG0` translation in `wmania_rng.*`:

- SPORTBKBMOD background family
- fade/color-cycle metadata
- 32 `BALLD05A` objects
- source inclusive RNDRNG0 velocity ranges
- source fixed-point `[400,0]` / `[255,0]` position ranges
- bouncing-ball simulation
- custom message line detection
- first text position Y=50/X=200
- 45-pixel line step
- 120-tick initial display
- 6*TSEC button/timeout wait

The operator text storage itself is represented by
`WmAttractOperatorMessage`; bind it to whatever ported operator/settings
storage is used on N64. Its `line_count` and `line_size` must come from the
translated shared operator constants corresponding to `CMESS_LINES` and
`CMESS_LINE_SIZE`; this bundle does not guess them.

## Time/date

The source only shows this on even loops **and** only if its time/date DIP
is enabled.

`wmania_attract_time.*` ports the source validation and formatting:
- invalid weekday -> Sunday
- invalid month -> January
- invalid date -> 1
- source accepts years 0..98; 99 falls to 00
- invalid hour/minute -> 0
- 12-hour conversion maps 0 and 12 to 12
- minutes are zero-padded
- the cabinet prints no AM/PM suffix
- the date separator is the literal source `, 19`, so the two-digit RTC
  year is displayed as a 1900s year

The adapter supplies the actual N64/RTC clock and the option replacing the
cabinet DIP.

## Copyright screen

The screen is two pages:
- 9 text lines + `SMWWF2`
- wait 3*TSEC
- delete text
- 10 text lines
- wait 3*TSEC

Both pages use `RD7FONT`; placement helpers preserve the source Y/X layout.
The literal legal/music-rights strings remain keyed by their original
source labels.

## AAMA screen

The translated screen exports:
- the six source text-label positions
- the 31-up/32-down blue gradient row model
- 8-step fade
- mandatory 20-tick fade-completion wait
- 4*TSEC button/timeout wait

That 20-tick wait is source-critical: the comments explicitly explain it
prevents the fade process from colliding with the high-score wipe workspace.

## Source text

To avoid substituting or paraphrasing the cabinet strings, the C tables use
original source labels. Run:

```sh
python3 extract_attract_source_text.py /path/to/ATTRACT.ASM \
  > wmania_attract_source_text_generated.h
```

against the same original source revision used by the merge. The renderer
can then resolve labels to literal source strings.

## Hidden "octopus" attract path

`ATTRACT.ASM` also contains `octopus_page`, a top-level attract-mode secret
path. This bundle includes its sequence detector:

Kick -> Block -> Power Punch -> Punch -> Block -> Power Kick

Each step gets TSEC/3 and the page has an overall 10*TSEC timeout. Success
hands off to the existing `HID_P`/hidden-page implementation and then
returns to normal attract mode. No hidden-screen content is invented here.

## Not duplicated by this package

These are source dependencies / already-ported systems rather than missing
ATTRACT.ASM logic:

- DCS logo implementation
- Midway Sports logo implementation
- title/lava implementation
- shared `CRD_SCRN2` credit-system implementation
- translated asset/font/image system
- `HID_P` hidden-page destination
- **gameplay demos (explicitly excluded)**

Everything else in the normal non-gameplay attract flow is represented by
the included scheduler/data/logic/adapter modules.
