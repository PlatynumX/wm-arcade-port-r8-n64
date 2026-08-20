# Stage 18 source audit — Shawn Michaels

Source: `SHAWN.ASM` (3329 lines)
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/SHAWN.ASM`

Translated in the dedicated `wm_arcade_shawn.c/.h` module:

- The source 26-mode dispatcher and 32-action table are retained locally in this wrestler module; they are not executed through a generic cross-wrestler behavior engine.
- `shn_secret_moves` order: charge flying kick, grab/hip-toss variants, neck grab, frankensteiner, jump kick.
- Flying-kick release threshold: 85 SKICK ticks; sets INAIR, X velocity = 1 and selects `shn_flying_kick_anim`.
- `frankensteiner`: SKICK, toward, toward, max 32.
- `jump_kick`: SKICK, away, away, max 32.
- 50x45 close punch route and 160x140 grounded falling-punch route.
- Super-punch pummel route and Shawn-specific super-kick/frankensteiner path.
- Turnbuckle attack split: punch-family -> `shn_belbow_anim`, kick-family -> `shn_bstomp_anim`.
- Full `shn_smove_table` label order exported, including the aerial/headhold family.
