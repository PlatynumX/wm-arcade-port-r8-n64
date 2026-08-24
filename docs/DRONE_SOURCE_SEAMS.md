# DRONE source-data seams after V13e-c3

**V13e-c3 status:** scalar callbacks, `RNDRNG0`, short/medium/long range mode
lists, script-list metadata, decoded script bodies, and command-2 skill tables
are source-bound.  C4 owns the remaining executable service seams: distinct
plain `rnd`, `drone_seek`, and command-5 / EXGPC code-call targets.

## Source table callbacks — V13e-c1 complete

These callbacks are generated/bound from literal source tables:

- `block_base_pct(skill)` -> `blkbase_t`
- `block_attack_pct(missed_count)` -> `blkatk_t`
- `headhold_delay_max(skill)` -> `sklhhdly_t`
- `headheld_delay_max(skill)` -> `sklhrdly_t`

No values are tuned or inferred from gameplay.

## Range/mode tables — V13e-c2 complete

`range_script_list(self, opp, band, my_mode, opp_mode)` implements the original
per-wrestler mode-list contents selected through `wnshort_t`, `wnmed_t`, and
`wnlong_t`.  Signed mode bytes, wildcard/default records, source script-list
headers, wrestler slot 7, and source script ordering remain source-derived.

## Script bodies + command-2 tables — V13e-c3 complete

`resolve_script(source_label)` now resolves generated operations decoded from
the original script body.  The direct routes include the named DRONE core
scripts and all source script labels reachable from the generated range lists.

The decoder preserves the source command forms handled by `drone_script`:

- positive input word + delay
- done/yield
- command #1 seek
- command #2 skill-table abort test
- command #3 interruptible wait
- command #4 abort-if-blocking
- command #5 source code call
- command #6 random jump
- command #7 unconditional jump
- fallback EXGPC call seam

`script_skill_pct(source_table_label, skill)` is now backed by the literal
30-entry command-2 table emitted from `DRONE.ASM`.

The `#dsdone` path stores the already-advanced script pointer.  C3 therefore
yields for the current tick while retaining the script, instead of treating
DONE as `#dsabt` and clearing it.

## C4 executable service seams

- `rnd_upto(...)` -> direct source plain `rnd`; do not alias this to `RNDRNG0`.
- `script_seek(...)` -> direct port/binding of source `drone_seek`.
- `script_call(source_label)` -> exact command-5 and EXGPC source targets.

Until those callbacks exist, code-call operations fail closed at their current
script PC so an unavailable source service cannot be silently skipped.

## Completion criterion

DRONE is complete only when every table, script, random service, seek service,
and code-call label reached by the original combat path resolves to translated
source behavior, followed by live CPU-input activation and regression testing.
