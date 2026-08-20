# Stage 17 source audit — Yokozuna

Source: `YOKO.ASM` (2788 lines)
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/YOKO.ASM`

Translated in the dedicated `wm_arcade_yoko.c/.h` module:

- The source 26-mode dispatcher and 32-action table are retained locally in this wrestler module; they are not executed through a generic cross-wrestler behavior engine.
- `yok_secret_moves` order, including `charge_salt`, neck/grab/hip-toss variants, `scissors`, `gut_push`, and `jabs`.
- Salt charge threshold: 85 PUNCH ticks; resets `RUN_TIME`, returns to NORMAL, resolves `yok_2_salt_anim`/`yok_4_salt_anim`.
- `scissors`: SKICK, toward, toward, max 32.
- `gut_push`: PUNCH, toward, toward, max 40.
- `jabs`: PUNCH, toward, down+toward, down, max 50.
- Source close punch routing uses `yok_heldheadbutt_rpt_anim` at 62x95, not a generic headbutt substitute.
- Yoko running slap, scissor and turnbuckle butt-drop labels preserved.
- Full `yok_smove_table` label order exported.
