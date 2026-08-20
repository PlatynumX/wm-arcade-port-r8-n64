# Stage 23 source audit — all eight wrestlers use dedicated direct-port modules

## Structural correction

Stage 23 removes the cross-character Stage 16-21 behavior engine and restores the source-module boundary of the arcade code.

Dedicated C modules now correspond to the original wrestler ASM files:

- `wm_arcade_bret.c/.h` <- `BRET.ASM`
- `wm_arcade_razor.c/.h` <- `RAZOR.ASM`
- `wm_arcade_taker.c/.h` <- `TAKER.ASM`
- `wm_arcade_yoko.c/.h` <- `YOKO.ASM`
- `wm_arcade_shawn.c/.h` <- `SHAWN.ASM`
- `wm_arcade_bam.c/.h` <- `BAM.ASM`
- `wm_arcade_doink.c/.h` <- `DOINK.ASM`
- `wm_arcade_lex.c/.h` <- `LEX.ASM`

For Taker/Yoko/Shawn/Bam/Doink/Lex, each module owns its own source-secret table, local 32-entry action table, mode switch, normal/running/bouncing/turnbuckle/block/headhold behavior, charge release, unique secret handlers and persistent-special routing.

`wm_arcade_roster.c` is metadata/selection only. `wm_arcade_wrestler_port.c` is dispatch/label routing only. Neither implements wrestler behavior.

## Removed generic behavior symbols

The C/H tree contains no definitions or uses of:

- `wm_arcade_move_remaining_wrestler`
- `wm_arcade_roster_release_charge`
- `wm_arcade_roster_fire_secret`
- `wm_arcade_roster_start_special_process`

## Source boundary evidence

The original arcade sources define separate `move_taker`, `move_yoko`, `move_shawn`, `move_bam`, `move_doink` and `move_lex` control routines, each dispatching through its own local `mode_table`; their action tables are likewise local to those source modules. Stage 23 mirrors that separation in C.

## Fidelity rule

Shared code is allowed only for systems that are shared by the arcade itself (collision/damage/reaction core, attachment services, actor adapter contracts, metadata/selection). Character-specific decision logic must remain in the character-specific module. Unknown target mappings remain unresolved rather than approximated.
