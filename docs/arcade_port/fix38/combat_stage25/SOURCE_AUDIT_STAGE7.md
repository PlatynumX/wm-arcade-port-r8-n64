# Source Audit — Combat Stage 7 (`REACT5.ASM`)

Primary arcade source: `REACT5.ASM` from `historicalsource/wwf-wrestlemania`.
This chunk ports the live REACT5 routines referenced by the current `REACT1.ASM`
`hit_table`. It intentionally preserves duplicate run validation, dead-victim
exceptions, direct mode writes, and commented-out dead checks where the arcade
source does so.

## Implemented routines

### `good_run_hit`
- Computes absolute attacker/victim Z delta in integer units from the 16.16 positions.
- Ordinary wrestlers: accept only delta <= 2.
- Yokozuna (wrestler index 3): reject if attacker `GETUP_TIME != 0`; otherwise
  accept only delta < 5.
- Exported both as a direct helper and as the Stage 2 `good_run_hit` callback.

### `hit_run`
- Calls `good_run_hit` a second time exactly as REACT5 does, even though
  `wrestler_hit` already performs the prefilter.
- Failed second check writes victim `MOVE_DIR` to `new_victim_movedir`, turns
  collision off and returns.
- INAIR/INAIR2 victims: collision-off only.
- Blocking victim: flailing block reaction + victim Y velocity +3.0, then runner
  bounce logic continues.
- ONGROUND or zero-life victim: collision-off and return; runner is not bounced.
- Normal live victim: LBOWDROP sound, SETMODE NORMAL, lose-balance animation.
- Yokozuna run-hit special: fall-back animation, replace pending damage with
  `-D_GUTPUSH`, victim X velocity +/-3.0 away from attacker.
- Genuine runner bounce: runner Y velocity +3.0 and direct `PLYRMODE=MODE_NORMAL`.
- Dizzy runner branch: clears runner `RUN_TIME`, uses dizzy bounce animation,
  preserves GETUP_TIME/meter state.
- Non-dizzy branch: if GETUP_TIME and METER_PROC exist, invokes the exact adapter
  hook for the arcade `slide_offscr` process transfer; then clears both run timers
  and runner GETUP_TIME and uses the normal bounce-off table.
- Runner X velocity becomes +/-3.0 away from victim.
- Repeated collision-off calls are preserved.

### `hit_puppet` / `hit_puppet_even_if_dead`
- Blocking victim -> standard block reaction.
- Otherwise attempts SETMODE PUPPET, mutually attaches processes, selects
  `wres_slave_anim`, clears victim GETUP_TIME, collision off.
- Because arcade SETMODE refuses to change MODE_DEAD, a dead victim remains DEAD
  while still receiving the mutual attachment. This is preserved.

### `hit_puppet_noflail`
- Blocking victim -> standard block reaction.
- Dead victim -> collision-off only; no attachment.
- Otherwise same mutual puppet attachment/slave animation and GETUP_TIME clear.

### `hit_puppet2`
- If victim GETUP_TIME is zero: clear attacker ANIMODE `MODE_STATUS`, collision off.
- Dead victim with nonzero GETUP_TIME: collision-off only.
- Otherwise attach as puppet, slave animation, clear GETUP_TIME.

### `hit_puppet_hdgrab`
- SAFE_TIME + blocking victim -> flailing block reaction.
- Dead victim -> collision-off only.
- Otherwise clear attacker HITBLOCKER, copy current PCNT to victim HEAD_GRAB_TIME,
  then perform puppet attachment/slave animation and clear GETUP_TIME.
- In the portable bridge, Stage 2 has already written current 32-bit PCNT into
  attacker `last_hit_time` before the reaction callback, so this field is used as
  the exact PCNT handoff rather than inventing another clock.

### `hit_puppet_toss`
- SAFE_TIME != 0: ordinary held block is sufficient to block the toss.
- SAFE_TIME == 0: block only succeeds when the stick is down + away using the
  literal horizontal mask `0x0c` and down bit 1 test from source.
- Source's dead-player rejection is commented out; therefore successful toss can
  mutually attach a dead victim while SETMODE leaves the victim in MODE_DEAD.
- Clears attacker HITBLOCKER, attaches both ways, slave animation, clears
  GETUP_TIME, collision off.

### `hit_backhand`
- Block -> flailing block.
- Shawn (wrestler index 4) uses `triple_sound(0x33)`; all others use `0x43`.
- Calls `CALL_AVERAGE_MOVE`.
- Zero-life victim exits after collision off.
- Live victim: UPRCUT sound + SETMODE NORMAL.
- Height >= 20 -> fall-back table + X velocity +/-3.0 away from attacker.
- Lower victim -> REACT5-specific `head_hit2`/head-hit-3 table.

### `hit_earslap`
- Block -> standard block.
- `triple_sound(0x43)`.
- Zero-life victim exits after collision off.
- Live victim: HDBUTT sound, SETMODE NORMAL, REACT5 head-hit-4 table.

### `hit_buzz`
- Block -> standard block.
- Live victim gets SETMODE NORMAL; zero-life branch deliberately skips only that.
- Mutual attachment is performed even on the zero-life branch, matching the
  source's explicit "want to see the electrocution" fix.
- PUNCH sound + per-wrestler `get_buzz` animation + collision off.

### `hit_haymaker`
- Block -> flailing block.
- FLYKICK sound.
- Live victim gets SETMODE NORMAL; dead mode remains dead.
- Fall-back animation regardless of life, X velocity +/-4.0 away, collision off.

## Actor adapter additions

The portable actor adapter now exposes only the additional original fields needed
by REACT5:

- `facing_dir`
- `new_facing_dir`
- `stick_val_cur`
- `safe_time`
- `dizzy`
- `head_grab_time`
- `meter_proc`

These are adapter fields to map onto the current N64 wrestler/process structures;
they are not a request to replace the port's native actor layout.

## Callback addition

`wm_arcade_react1_callbacks_t.slide_getup_meter` maps the REACT5 `METER_PROC` /
`XFERPROC ... slide_offscr` behavior. Do not replace it with an approximate HUD
hide or timer reset if the existing port already has the translated meter process.

## New semantic animation groups

- `WM_R1_ANIM_BOUNCE_OFF`
- `WM_R1_ANIM_BOUNCE_OFF_DIZZY`
- `WM_R1_ANIM_BACKHAND_HEAD_HIT`
- `WM_R1_ANIM_EARSLAP_HEAD_HIT`
- `WM_R1_ANIM_GET_BUZZ`
- `WM_R1_ANIM_WRES_SLAVE`

They remain distinct because the source points at different animation tables or
specific shared sequences.

## Stage 2 integration

Wire `wm_arcade_react_callbacks_t.good_run_hit` to:

    wm_arcade_react5_good_run_hit_callback

The cumulative reaction callback after this chunk is initially
`wm_arcade_react12345_reaction_callback`; Stage 9 provides the final cumulative
name for this bundle.
