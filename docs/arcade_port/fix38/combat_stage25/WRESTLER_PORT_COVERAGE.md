# Wrestler-specific combat coverage after Stage 23

All eight selectable arcade wrestlers now execute dedicated direct-port modules:

- Bret Hart — `wm_arcade_bret.c` <- `BRET.ASM`
- Razor Ramon — `wm_arcade_razor.c` <- `RAZOR.ASM`
- Undertaker — `wm_arcade_taker.c` <- `TAKER.ASM`
- Yokozuna — `wm_arcade_yoko.c` <- `YOKO.ASM`
- Shawn Michaels — `wm_arcade_shawn.c` <- `SHAWN.ASM`
- Bam Bam Bigelow — `wm_arcade_bam.c` <- `BAM.ASM`
- Doink — `wm_arcade_doink.c` <- `DOINK.ASM`
- Lex Luger — `wm_arcade_lex.c` <- `LEX.ASM`

## Common API, separate behavior bodies

`wm_arcade_roster_profile()` provides metadata and `wm_arcade_move_ported_wrestler()` is the single N64 integration entry point, but the wrapper contains no character behavior. It dispatches directly to the matching wrestler module.

Each of the six modules redone in Stage 23 owns its local action/mode logic instead of calling a generic six-wrestler implementation. The repeated code is intentionally retained where the original arcade sources repeat it; fidelity and source traceability take priority over deduplication.

## Remaining combat phases

1. `wrestler_hit_special` / special-object and projectile combat.
2. CPU/drone combat decisions.
3. N64-native adapter wiring and arcade-vs-port behavioral verification.
