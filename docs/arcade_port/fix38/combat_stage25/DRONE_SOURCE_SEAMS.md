# DRONE source-data seams for the next direct-port substage

This file is a checklist for materializing the remaining raw `DRONE.ASM` data without changing Stage 25 logic.

## Source table callbacks

Map these callbacks to literal source tables:

- `block_base_pct(skill)` -> `blkbase_t`
- `block_attack_pct(missed_count)` -> `blkatk_t`
- `headhold_delay_max(skill)` -> `sklhhdly_t`
- `headheld_delay_max(skill)` -> `sklhrdly_t`

Do not infer values from gameplay videos or tune them by feel.

## Range/mode tables

`range_script_list(self, opp, band, my_mode, opp_mode)` must implement the original per-wrestler mode-list contents selected via:

- `wnshort_t` for band 0 (`max(X, Z*2) < 100`)
- `wnmed_t` for band 1 (`100 <= max(X, Z*2) < 180`)
- `wnlong_t` for band 2 (`max(X, Z*2) >= 180`)

Preserve each mode-list record's signed my-mode byte, signed opponent-mode byte, and script-list pointer. Preserve the script list's signed first word and original script ordering.

## Named scripts

`resolve_script(source_label)` must return decoded operations produced directly from the original script body. Stage 25 already implements the execution semantics; do not reinterpret the script as higher-level strategy.

Named routes seen in the executable core include at least:

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

The range tables add further wrestler-specific script labels; materialize them exactly from source.

## Script skill/call seams

- `script_skill_pct(source_table_label, skill)` -> literal table used by command #2
- `script_call(source_label)` -> exact original code/function target for command #5 and source fallback-call commands
- `script_seek(...)` -> direct port of the source `drone_seek` service

## Random services

Keep source `rnd` and `rndrng0` distinct. The Stage 25 interface intentionally has separate `rnd_upto` and `rndrng0_upto` callbacks.

## Completion criterion

This AI-data substage is complete only when every table/script/call label reached in the original `DRONE.ASM` combat path resolves to source-derived data/code, with no guessed percentages, weights, scripts, or fallback actions.
