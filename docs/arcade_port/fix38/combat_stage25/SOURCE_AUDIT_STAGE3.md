# WrestleMania Arcade Combat Port — Stage 3 Source Audit

## Scope

Stage 3 ports the **hit-table reaction routines physically defined in `REACT1.ASM`**, plus the shared block reactions and the universal `hit_ontbukl` override. It does not build a ROM and does not recreate missing animation art or sound.

Arcade source:

- `REACT1.ASM`: https://github.com/historicalsource/wwf-wrestlemania/blob/main/REACT1.ASM
- raw: https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/refs/heads/main/REACT1.ASM

## Directly translated routines

- `block_hit`
- `block_hit_flail`
- `hit_punch`
- `hit_fire_punch` (same entry body as `hit_punch`)
- `hit_hdbutt`
- `hit_hdbutt2`
- `hit_urn` (also used by `AMODE_HDBUTT3` through Stage 2's table)
- `hit_hdbutt_stay`
- `hit_tomb`
- `hit_super_kick` + its intentional fall-through into kick behavior
- `hit_kick`
- `hit_flykick`
- `hit_bigknee`
- `hit_grabthrow` (source is a bare `rets`; kept as a supported no-op)
- `hit_ontbukl`

## Preserved source behavior worth calling out

### Blocking

`block_hit` pushes the victim at exactly `+/- [4,8000h]` (4.5 in the source's 16.16 velocity representation), plays the block sound path, chooses `hitblock_tbl`, changes animation, and disables wrestler collision checking.

`block_hit_flail` is the same pattern with exactly `+/- [6,8000h]` (6.5) and `hitblock_flail_tbl`.

### Punch chain

A non-blocked punch:

1. triggers `CALL_FACE_HIT`;
2. stops if `GETLIFE` is zero;
3. plays the punch sound;
4. sets `MODE_NORMAL`;
5. if the victim is >= 20 units above `GROUND_Y`, uses `fall_back_tbl` and +/-3 X velocity;
6. otherwise increments `CONSECUTIVE_HITS`;
7. exactly on hit 6, resets the counter to zero and uses the wrestler-specific lose-balance table unless the attacker is currently in combo mode (`WHOHITME->COMBO_COUNT != 0`);
8. otherwise uses `head_hit_tbl`.

### Flying kick

The source changes the **attacker** before checking for block. Stage 3 keeps that order:

- arithmetic-halves attacker X velocity;
- takes magnitude;
- if magnitude is >= `20000h`, forces magnitude to `40000h`;
- reverses its sign relative to the incoming velocity;
- sets attacker Y velocity to `40000h`;
- only then checks block.

Blocked flying kicks use `block_hit_flail` and return the source's "aborted" state through `last_flykick_aborted`.

Lex Luger's `lex_flying_kick_anim` and `lex_super_kick_anim` use `CALL_MID_HIT`; all other flying kicks use `CALL_DROP_KICK`. This is exposed as an exact merge callback rather than guessed from a new animation ID.

### Tomb reaction

The source checks the victim's pre-reaction mode. `MODE_ONGROUND` and `MODE_DEAD` route to `hitonground_tbl`. Otherwise the victim goes normal, with the same >=20-height fall-back test used by several face-hit reactions.

### Turnbuckle override

`hit_ontbukl`:

- plays flying-kick reaction sound;
- uses `fall_back_tbukl_tbl`;
- sets `MODE_INAIR`;
- sets the `DEAD_ANIM` status bit;
- applies +/-4 X velocity away from the attacker;
- applies +6 Y velocity;
- disables wrestler hit collision checking.

## Animation integration boundary

The original source selects concrete wrestler animations through `FACETBL` and `FACE24TBL`. Stage 3 does **not** invent numeric N64 animation IDs. It emits semantic source table groups:

- `HITBLOCK`
- `HITBLOCK_FLAIL`
- `HEAD_HIT`
- `HEAD_HIT2`
- `BODY_HIT`
- `FALL_BACK`
- `HIT_ON_GROUND`
- `FALL_BACK_TBUKL`
- `LOSE_BALANCE`

During merge, resolve each group to the already-ported/original per-wrestler animation table. If one of those source animations has not yet been translated into the N64 animation system, leave that specific animation hook unresolved rather than substituting another animation.

## Audio / hit-effect integration boundary

Likewise, Stage 3 emits the original semantic calls (`CALL_FACE_HIT`, `CALL_MID_HIT`, `CALL_DROP_KICK`, block/punch/headbutt/kick/flying-kick sound families, `DO_SCREAM`) through callbacks. Do not replace them with approximate effects during merge.

## Actor adapter additions

`wm_arcade_actor_t` gains semantic fields needed by these routines:

- `x_vel`, `y_vel`, `z_vel` — original 16.16 velocity values
- `ground_y`
- `roll_pos`
- `usr_var1`
- `consecutive_hits`
- `life` — map this to the value tested by the original `GETLIFE` macro in the current port

These fields are a merge adapter, not a mandate to replace the existing N64 wrestler structure.

## Deliberately not in Stage 3

- `wrestler_hit_special` projectile/special-object path at the top of `REACT1.ASM`
- reaction routines referenced from `REACT2.ASM` through `REACT9.ASM`
- grapple/puppet ownership implementation beyond what Stage 2 already safely handles
- actual per-wrestler animation sequence translation/data
- DCS sound implementation
- renderer effects
- ROM integration/build

## Next source chunk

Continue with `REACT2.ASM`: `hit_uprcut`, `hit_lbowdrop`, `hit_grabhold`, `hit_grabfling`, `hit_push`, `hit_blbowdrop`, `hit_combo_uprcut`, and `hit_gutpush`, together with any helpers those exact routines require.
