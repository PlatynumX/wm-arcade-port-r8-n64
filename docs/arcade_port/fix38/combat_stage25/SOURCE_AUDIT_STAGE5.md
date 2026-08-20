# Source Audit — Combat Stage 5 (`REACT3.ASM`)

Primary arcade source: `REACT3.ASM` from `historicalsource/wwf-wrestlemania`.
This stage is a semantic C translation only; animation assets, sound playback,
impact effects and the original random helper remain merge adapters.

## Implemented routines

- `hit_bigboot` (`AMODE_BIGBOOT` / `WM_RXN_BIGBOOT`)
  - victim NORMAL/other standing modes: `CALL_FACE_HIT`, FLYKICK sound,
    `DO_SCREAM`, source `SETMODE NORMAL`, `head_hit2_tbl`, collision off.
  - victim INAIR: `CALL_DROP_KICK`, LBOWDROP sound, `ROLL_POS=0`,
    `set_getup_time`, `fall_back_tbl`, X velocity +/-3.0, collision off.
  - victim RUNNING: preserves the source `RNDPER(100)` + `JRHI` split via
    the `rndper_hi(100)` callback. This is deliberately not replaced by a new RNG.
  - the source-local `#no_hit` tail has no incoming branch in checked-in
    `REACT3.ASM`; it is documented but not invented as a reachable path.

- `hit_knee` (`AMODE_KNEE` / `WM_RXN_KNEE`)
  - block -> standard REACT1 `block_hit`.
  - otherwise `CALL_MID_HIT`; if victim lives, KICK sound, SETMODE NORMAL,
    exact per-wrestler `knee_hit_tbl`, then attacker X velocity arithmetic-shifted
    right by 3. Collision is disabled at exit.

- `hit_headknees` (`AMODE_HEADKNEES` / `WM_RXN_HEADKNEES`)
  - KICK sound, victim Y velocity `0x00040000`, per-wrestler quick-knee table,
    collision off. No invented block/health/impact checks were added.

- `hit_boxpunch` (`AMODE_BOXPUNCH` / `WM_RXN_BOXPUNCH`)
  - block -> `block_hit_flail`.
  - otherwise `CALL_FACE_HIT`, FLYKICK sound; non-dead victim gets SETMODE NORMAL.
  - source `GETUP_TIME = 5*TSEC` is retained as 300 60-Hz ticks, matching the
    source family's explicit `6*60` timing convention.
  - `fall_back_tbl`, X velocity +/-4.0, collision off even on the dead-health path.

## New semantic animation groups

- `WM_R1_ANIM_KNEE_HIT`
- `WM_R1_ANIM_QUICK_KNEE_HIT`

The N64 merge must map these to the original per-wrestler source sequences;
no substitute animation should be used for null/spare table entries.

## New required source hook

`wm_arcade_react1_callbacks_t.rndper_hi(argument, user)` represents the exact
condition-code result used after arcade `RNDPER`. The Stage 5 big-boot running
branch refuses to guess if this hook is absent.

## Cumulative bridge

`wm_arcade_react123_reaction_callback` routes REACT3-owned reactions first,
then falls back to the Stage 4 REACT1+REACT2 bridge.
