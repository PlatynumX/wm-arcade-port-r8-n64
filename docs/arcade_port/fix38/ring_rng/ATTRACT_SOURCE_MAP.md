# ATTRACT.ASM source map

| Arcade source | Portable handoff |
|---|---|
| `attract_mode` | `wm_attract_build_cycle`, `wm_attract_run_cycle` |
| `show_hstd` | included `wmania_hiscore_*` + `show_hiscores` adapter |
| `DCS_LOGO` | existing-port adapter callback |
| `show_sports_logo` | existing-port adapter callback |
| `show_gameplay` | two explicit unimplemented/no-op slots |
| `creditscreen` -> `CRD_SCRN2` | `show_credits_crd_scrn2` dependency |
| `show_title` | existing-port adapter callback |
| `DO_HINTS` | hint state/data in `wmania_attract_core/data` |
| `WHICH_HINT`, `NUM_HINTS=5` | `wm_attract_hints[5]` |
| `print_gen_tips`, `show_gen_tips` | general-tip labels/layout/timing |
| `show_bios`, `show_bios_tips` | bio state/data/layout callbacks |
| `bio_data` | `wm_attract_bios[8]` |
| `wrestler_tunes` | per-bio `tune_id` |
| `show_operatormsg` | `WmAttractOperatorMessage` + adapter |
| `dan_test` / `DISPATCH` / `ONE_BALL` | `wmania_attract_operator.*` |
| `TURN_SOUNDS_OFF_IF_NEED` | `wm_attract_demo_should_suppress_sound` |
| `wait_on_butn` | SOUNDSUP helpers + source sound ID |
| `show_time_date` | `wmania_attract_time.*` |
| `show_copyright` | two page placement/timing metadata |
| `aama_message`, `do_the_grad_thang` | AAMA placements/gradient |
| `AMODE_LOOPS` conditions | exact even/eighth-loop scheduler behavior |
| `RemapIO` | `remap_io` adapter callback |
| `octopus_page` | `wmania_attract_secret.*` |

## Deliberately source-faithful quirks

- First active designer hint is index 1 from zeroed BSS because the source
  increments before selection.
- First bio is index 1 (Razor) for the same reason.
- Bio and bio-tips screen show the same wrestler.
- Time/date occurs only on even cycles and has its own DIP gate.
- Copyright/AAMA occur only on every eighth cycle.
- `AMODE_LOOPS` and `SOUNDSUP` both reset after the eighth-cycle legal path.
- Current source operator-message path uses `dan_test` bouncing balls.
- Gameplay demo call sites remain present but contain no gameplay port.
