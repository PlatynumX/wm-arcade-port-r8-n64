# Source Audit — Combat Stage 10 (`REACT8.ASM`)

Primary arcade source:
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/REACT8.ASM`

Current routing source:
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/REACT1.ASM`

Stage 10 ports the live `REACT8.ASM` hit reactions used by the current hit table:

- attack 35 / `hit_shnbfkik`: separate damage ID, reaction tail-jumps to `hit_flykick`.
- attack 36 / `hit_shnspdkik`: blocking -> `block_hit_flail`; otherwise living victims get KICK sound, `SETMODE NORMAL`, FACE24 head-hit animation; collisions are disabled.
- attack 37 / `hit_shnspdkik2`: blocking -> flail; otherwise FLYKICK sound, conditional `SETMODE NORMAL`, zero Z velocity and ROLL_POS, `set_getup_time`, fall-back animation, exact +/-4.0 X velocity away from attacker, collisions off.
- attack 38 / `hit_hitcheck`: clears global `hit_damage_pending` and disables collisions.

## Legacy `hit_flyelbow` body

`REACT8.ASM` still contains a body labelled as attack 39.  The current `REACT1.ASM`
hit table routes attack 39 to `hit_combo_uprcut` instead.  The old flying-elbow body
is therefore preserved as:

    wm_arcade_react8_legacy_flyelbow()

for source/audit parity, but it is deliberately **not** installed into the live hit
routing.  Its source behavior is: run normal `hit_flykick`; if the victim did not
block, negate the attacker's already modified X velocity again so the attacker does
not bounce away.

## Cumulative bridge

Stage 10 introduced:

    wm_arcade_react12345678_reaction_callback

The final bundle supersedes this with the Stage 11 cumulative bridge documented in
`MERGE_HANDOFF.md`.
