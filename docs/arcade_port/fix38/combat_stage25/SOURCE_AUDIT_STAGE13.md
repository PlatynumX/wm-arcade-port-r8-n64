# Source Audit — Combat Stage 13 (top-level wrestler move dispatch)

Primary arcade source:
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/WRESTLE.ASM`

Stage 13 ports the exact top-level `move_wrestler` dispatch contract and the
`convert_facing` table.  It deliberately exposes native-engine work as callbacks
instead of inventing N64 state or translating thousands of character-specific lines
without their animation/data integration.

## `move_wrestler` ordering

`wm_arcade_move_wrestler()` preserves this source order:

1. if global HALT is nonzero, return immediately;
2. if `SPECIAL_MOVE_ADDR` is nonzero, call the change-animation hook with that
   queued address/token, clear it, and return;
3. run `auto_pin_check`;
4. dispatch by `WRESTLERNUM`:
   - 0 Bret
   - 1 Razor
   - 2 Undertaker
   - 3 Yokozuna
   - 4 Shawn
   - 5 Bam Bam
   - 6 Doink
   - 7 spare / null entry
   - 8 Lex

The queued special therefore wins **before** auto-pin and before character input
logic, matching `WRESTLE.ASM`.

## `convert_facing`

The exact 16-entry source table is preserved:

    0,0,4,0,6,7,5,0,2,1,3,0,0,0,0,0

## Deliberate boundary

This stage does not claim that `move_bret`, `move_razor`, `move_taker`, `move_yoko`,
`move_shawn`, `move_bam`, `move_doink`, or `move_lex` are translated.  Those are
large character-specific move/input/animation modules and are the next project
phase.  The callback is the exact seam used to merge those translations later.
