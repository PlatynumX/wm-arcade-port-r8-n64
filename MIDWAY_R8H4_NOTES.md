# r8h4 source-flow correction

r8h3 proved that the exact 17-piece Midway Sports WIMP artwork could be decoded
and assembled, but its surrounding `Midway -> title -> select -> match` state
machine was not the arcade program.

r8h4 changes the architecture rather than polishing that shell:

- `ATTRACT.ASM::attract_mode` active `JSRP` calls are extracted into generated C.
- `show_sports_logo` returns to the attract caller.  Its next source call is
  `show_gameplay`; a button never jumps from Midway Sports to title/select.
- Sports timing now retains the source `SLEEPK 2`, object `SLEEPK 1`, `SLEEPK 32`,
  `TSEC/2`, and `8*TSEC` boundaries.  `MOVE_BACK_OFF_SCREEN` updates the portable
  world origin exactly by `X -= 2`, `Y += 2` each source tick.
- The fake title and character-select renderers/transitions are removed from the
  default N64 path.
- The original `show_gameplay` wrapper now owns the partial gameplay translation:
  3*60 live lead, 10*TSEC skippable wait, 60-tick freeze, 32-tick fade interval.
  Attract buttons do not directly control the wrestler.
- DCS_LOGO source timing/button gates are represented.  CI converts the exact
  `IMG/DCSLOGO.IMG` object.  Its TMS34010 pixel-rotation/velocity plotters are not
  replaced by a made-up N64 effect; those plotters remain a dedicated translation
  task.
- Untranslated attract routines are skipped in bring-up builds instead of being
  simulated with replacement screens.

Expected test: after Midway Sports, the ROM enters the translated attract gameplay
segment. It should no longer go to the r8h3 fake title or fake character select.
