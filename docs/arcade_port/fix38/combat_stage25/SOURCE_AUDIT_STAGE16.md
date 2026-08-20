# Stage 16 source audit — Undertaker

Source: `TAKER.ASM` (3266 lines)
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/TAKER.ASM`

Translated in the dedicated `wm_arcade_taker.c/.h` module:

- Undertaker's own 26-mode dispatcher and exact 32-entry button/action selector are retained in `wm_arcade_taker.c`.
- `und_secret_moves` ordering: `button_hold`, `grab_fling`, `hip_toss`, `grab_fling2`, `hip_toss2`, `neck_grab`, `tomb_smash`.
- `tomb_smash`: SKICK, toward, toward, max 32; clears attachment, returns to NORMAL, kills the endless process, starts `und_tombstone_smash_anim`.
- Punch/body routing including the source 75x45 close-hit threshold and grounded 160x140 elbow-drop branch.
- 110-tick PUNCH hold/release threshold for the spirit move path.
- Taker-specific turnbuckle butt-drop labels and normal-mode pin restriction (down-facing + Z < 0x40 + X < 35); otherwise raise-arm behavior.
- Full `und_smove_table` label order exported for the integration layer.

Integration seam: persistent arcade process labels (for example `und_hdhold_pile` and `und_spirit_pull`) are preserved by exact source name and passed through `start_special_label`/`resolve_label_token`. The port does not invent N64 animation/process addresses.
