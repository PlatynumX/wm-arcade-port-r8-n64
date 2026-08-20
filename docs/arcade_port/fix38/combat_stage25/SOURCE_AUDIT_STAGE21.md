# Stage 21 source audit — Lex Luger

Source: `LEX.ASM` (2749 lines)
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/LEX.ASM`

Translated in the dedicated `wm_arcade_lex.c/.h` module:

- The source 26-mode dispatcher and 32-action table are retained locally in this wrestler module; they are not executed through a generic cross-wrestler behavior engine.
- `lex_secret_moves`: charge clobber, neck grab, grab/hip-toss variants, sliding elbow, hammer.
- Clobber release threshold: 100 PUNCH ticks.
- Current source behavior retained: the old running/sliding charge-clobber variant is commented out; charge resolves the normal `lex_2_clobber_anim` / `lex_4_clobber_anim` path.
- `sliding_elbow`: PUNCH, toward, toward, max 30 -> `lex_sliding_elbow_anim`.
- `hammer`: SKICK, toward, toward, max 32.
- 40x45 close punch/headbutt route, exact `lex_2_ground_punch_anim` / `lex_4_ground_punch_anim` grounded path, and 70x45 normal super-punch close branch.
- Full `lex_smove_table` label order exported.
