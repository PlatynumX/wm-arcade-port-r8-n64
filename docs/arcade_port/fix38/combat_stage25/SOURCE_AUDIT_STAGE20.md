# Stage 20 source audit — Doink

Source: `DOINK.ASM` (4037 lines)
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/DOINK.ASM`

Translated in the dedicated `wm_arcade_doink.c/.h` module:

- The source 26-mode dispatcher and 32-action table are retained locally in this wrestler module; they are not executed through a generic cross-wrestler behavior engine.
- `doink_secret_moves`: charge buzz, grab/hip-toss variants, ear slap, hammer, neck grab, boxing punch.
- Buzz release threshold: 100 PUNCH ticks. Running/toward path chooses `dnk_2_buzz2_anim`/`dnk_4_buzz2_anim`; stationary path chooses `dnk_2_buzz_anim`/`dnk_4_buzz_anim`.
- Source quirk retained: buzz sound hook is the single first headbutt sound token, not a fabricated two-channel pair.
- Ear slap: PUNCH, toward, down+toward, down, max 50.
- Hammer: SKICK, toward, toward, max 32.
- Boxing punch: seven PUNCH inputs, max 60, with combo-count rejection.
- 50x45 close punch routing and Doink turnbuckle split (`dnk_diveofftb_anim` vs `dnk_4_bstomp_anim`).
- Full Doink special-process label table exported.
