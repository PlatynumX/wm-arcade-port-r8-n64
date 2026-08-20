# Source Audit — Combat Stage 6 (`REACT4.ASM`)

Primary arcade source: `REACT4.ASM` from `historicalsource/wwf-wrestlemania`.
The port preserves source ordering, repeated collision-off calls, velocity
constants, named-animation hacks and no-op routines rather than simplifying them.

## Implemented routines

### `hit_stomp` / `hit_buttstomp`
- MODE_NORMAL or MODE_BLOCK: clear `hit_damage_pending`, collision off, return.
- Other non-ground/non-dead modes: source collision-off tail preserved.
- ONGROUND/DEAD: `hitonground_tbl` reaction.
- Shawn `shn_combo_run_stomp_anim` / `shn_run_stomp_anim`: scream-only sound path.
- Ordinary path: LBOWDROP sound + `triple_sound(0x43)`.
- Exact ANIBASE hacks:
  - `dnk_belly_anim` and `und_flying_butt_drop_anim`: attacker bounce
    Y=+5.0, Z=+1.0, X=0, `shake_all_ropes`, `SHAKER2(8)`.
  - `lex_flying_ground_punch_anim`: same bounce, then Y overwritten to +4.0.
  - the Bret bounce2 check is commented out in source and remains inactive.

### `hit_bstomp`
- Initial scream + collision off.
- Ground/dead victim -> `hitonground_tbl` + second collision off.
- Standing victim -> second scream, `triple_sound(0x43)`, `set_getup_time`,
  SETMODE NORMAL.
- Height <20 -> knockdown table.
- Height >=20 -> fall-back, except attacker wrestler #5 takes knockdown path.
- Fall-back velocity: X +/-2.0, Y -4.0.

### `hit_bstomp2`
- block -> `block_hit_flail`.
- nonblock -> FLYKICK sound + collision off.
- COMBO_COUNT==0 and victim ground/dead routes to the preceding source
  `#isdead` collision-off tail (second collision-off preserved).
- COMBO_COUNT!=0 toggles victim `M_FLIPH`, zeros Y velocity and snaps Y to GROUND_Y.
- then `set_getup_time`, scream, `triple_sound(0x43)`, `CALL_NASTY_MOVE`,
  knockdown table, collision off.

### `hit_hammer`
- always `triple_sound(0x45)` + `CALL_NASTY_MOVE` first.
- block -> standard block reaction.
- otherwise jumps into `hit_bstomp`, preserving all of that routine's side effects.

### `hit_spinkick`
- block -> flailing block.
- nonblock -> `triple_sound(0x43)` + `CALL_AVERAGE_MOVE`.
- dead health exits after collision off.
- living victim -> KICK sound + SETMODE NORMAL.
- height <20 -> the REACT4-specific `head_hit2` table.
- height >=20 -> `fall_back_tbl`, X +/-3.0.

### `hit_cline`
- block -> flailing block.
- otherwise `CALL_DROP_KICK`, FLYKICK sound, SETMODE NORMAL only if health nonzero.
- attacker Z velocity is zeroed.
- victim Z position and ROLL_POS become attacker Z minus exactly 1.0.
- `set_getup_time`, REACT4 `fall_back2` table.
- victim X velocity is +3.0 if attacker X velocity is positive, otherwise -3.0.

### `hit_headhold`
Bare `rets` in source; preserved as a no-op.

### `hit_jumpkick`
- block -> flailing block.
- dead health -> collision off.
- living victim -> KICK sound + SETMODE NORMAL.
- height <20 -> REACT4 jump-kick-specific head-hit table.
- height >=20 -> `fall_back_tbl`, X +/-3.0.
- no impact helper is invented because the source does not call one here.

## New exact-source adapter hooks

- `attacker_anim_tag`: resolves only the five ANIBASE labels actually compared
  by `hit_stomp`.
- `move_grade`: maps `CALL_AVERAGE_MOVE` / `CALL_NASTY_MOVE`.
- `shake_all_ropes` and `shaker2(8)`: source bounce side effects.

## New semantic animation groups

- `WM_R1_ANIM_SPINKICK_HEAD_HIT`
- `WM_R1_ANIM_FALL_BACK2`
- `WM_R1_ANIM_JUMPKICK_HEAD_HIT`

These remain separate because the actual per-wrestler tables differ in source.

## Cumulative bridge

Use `wm_arcade_react1234_reaction_callback` after Stage 6. It routes REACT4,
then REACT3, then the existing REACT2/REACT1 implementations.
