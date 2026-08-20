# Source Audit — Combat Stage 11 (`REACT9.ASM`)

Primary arcade source:
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/REACT9.ASM`

Current routing source:
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/REACT1.ASM`

Stage 11 ports all live hit reactions defined by `REACT9.ASM`:

- attack 40 / attack 43 -> `hit_rslash`
- attack 41 -> `hit_headdslash`
- attack 42 -> `hit_headuslash`
- attack 55 -> `hit_napalm`

## Razor slash

- blocking uses the shared flailing block reaction;
- non-block hit invokes the face-impact hook before the life test;
- zero-life victim simply has collisions disabled;
- living victim gets RSLASH sound and `SETMODE NORMAL`;
- if victim Y minus ground Y is at least 20, use fall-back and +/-3.0 X velocity;
- otherwise use FACE24 `head_hit2`;
- collisions are disabled on both live reaction paths.

## Down/up head slash

Both play RSLASH first.  Living victims use the knee-hit table and zero X velocity.
The down slash sets Y velocity to `0x0002C000` and nudges the victim 5 integer X
units toward the attacker.  The up slash sets Y velocity to `0x00040000` with no
X-position nudge.  Zero-life victims use fall-back with +/-4.0 X velocity.

## Napalm

- `MODE_NORMAL` and `MODE_BLOCK` explicitly zero pending damage, then disable collisions.
- Other modes disable collisions before the mode test.
- Only `MODE_ONGROUND` and `MODE_DEAD` select the burn animation.
- Burn path sets `DEAD_ANIM`, uses the LBOWDROP sound path, calls `triple_sound(0x43)`, and disables collisions again exactly as the source does.

## Final cumulative bridge

Use:

    wm_arcade_react123456789_reaction_callback

This routes all live REACT1..REACT9 reactions while leaving REACT6/7 legacy no-op
labels out of the current attack table.
