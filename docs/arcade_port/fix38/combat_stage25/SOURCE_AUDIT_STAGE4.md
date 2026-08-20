# Source Audit — Combat Stage 4 (`REACT2.ASM`)

Primary arcade source:

- `REACT2.ASM` — Jamie Rivett, initiated 8/8/94
- `MACROS.H` — `FACETBL` and `SETMODE` semantics used by REACT2

Source repository:
`historicalsource/wwf-wrestlemania`

This stage is a direct semantic translation. It does **not** invent replacement
animations, damage values, audio IDs, physics constants, or state transitions.
Hardware/asset-specific operations remain callbacks to the existing N64 port.

## Implemented REACT2 routines

### `hit_uprcut` — `AMODE_UPRCUT` / reaction `WM_RXN_UPRCUT`

Translated behavior:

- Blocking victim -> existing REACT1 `block_hit_flail` path.
- If victim health is nonzero, `SETMODE NORMAL` (while preserving `MODE_DEAD`,
  exactly as the source macro does).
- Uppercut sound family.
- Per-wrestler `fall_back_tbl` animation selection.
- `flash_white` call.
- `ROLL_POS = 0`.
- Existing Stage 1 `set_getup_time` call.
- Exact launch-Y source special cases:
  - attacker wrestler 3 -> `+15.0`
  - ordinary attacker -> `+13.0`
  - attacker wrestler 2 + victim has live teammates -> `+11.0`
  - attacker wrestler 2 + no live teammates -> `+18.0`
- Defender X velocity is exactly `+/-2.0` away from attacker.
- Wrestler collision disabled.

### `hit_combo_uprcut` — `AMODE_UPRCUT2` / `WM_RXN_COMBO_UPRCUT`

Translated behavior:

- Blocking victim -> ordinary `block_hit`, not flailing block.
- Same health/normal-mode rule, uppercut sound, `fall_back_tbl`, white flash,
  `ROLL_POS = 0`, and get-up call.
- `ANIMODE = MODE_UNINT | MODE_NOAUTOFLIP | MODE_OVERLAP` exactly.
- Reads `WHOHITME->RPT_COUNT` as the source does:
  - `RPT_COUNT == 1` -> Y velocity `+7.0`
  - otherwise -> Y velocity `+3.0`
- X velocity is exactly `+/-1.5` away from attacker.
- Wrestler collision disabled.

`wm_arcade_actor.rpt_count` was added solely as a merge adapter for this exact
source field. Stage 2 establishes `WHOHITME` before dispatching the reaction.
The portable code contains a defensive null guard for malformed host state;
valid arcade flow has a usable `WHOHITME` pointer here.

### `hit_lbowdrop` — `AMODE_LBOWDROP` / `WM_RXN_LBOWDROP`

Translated behavior:

- If victim is `MODE_NORMAL` or `MODE_BLOCK`, the attack is ignored:
  `hit_damage_pending = 0`, then wrestler collision is disabled.
- Otherwise:
  - elbow-drop sound family
  - `triple_sound(0x33)`
  - collision off
  - per-wrestler `hitonground_tbl` animation
  - collision off **again**, matching the source order

### `hit_blbowdrop` — `AMODE_BLBOWDROP` / `WM_RXN_BLBOWDROP`

Translated behavior:

- `DO_SCREAM` semantic sound hook.
- Flying-kick sound family, then elbow-drop sound family.
- Collision off before mode routing.
- Victim already on ground/dead -> `hitonground_tbl`, collision off again.
- Otherwise source `SETMODE NORMAL` semantics are applied.
- Height test is exactly `(OBJ_YPOSINT - GROUND_Y) < 20`.
  - below 20: `set_getup_time`, per-wrestler knockdown table, collision off.
  - 20 or more: `triple_sound(0x43)`, `fall_back_tbl`, collision off,
    X velocity `+/-3.0`, Y velocity `-3.0`.

The per-wrestler `knockdwn` table remains a semantic animation group. The N64
merge layer must resolve it to the original wrestler-specific sequences and
must preserve the source's null entries rather than substituting art/animation.

### `hit_grabhold` — `AMODE_GRABHOLD` / `WM_RXN_GRABHOLD`

The routine body is commented out in the checked-in arcade source. The label
therefore falls through to the immediately following no-op `hit_grabfling`.
Stage 4 intentionally treats it as a no-op rather than resurrecting the
commented implementation.

### `hit_grabfling` — `AMODE_GRABFLING` / `WM_RXN_GRABFLING`

The checked-in source is a bare return. Stage 4 preserves that no-op.

### `hit_gutpush` — `AMODE_GUTPUSH` / `WM_RXN_GUTPUSH`

- Blocking victim: source clears victim X velocity, then calls
  `block_hit_flail` (which writes the final block push velocity).
- Non-blocking victim joins the common push body.

### `hit_push` — `AMODE_PUSH` / `WM_RXN_PUSH`

Translated common push behavior:

- source `SETMODE NORMAL` semantics on victim
- attacker's X velocity cleared
- per-wrestler lose-balance animation
- victim X velocity exactly `+/-8.0` away from attacker
- push sound family
- source `get_health(PLYRNUM)` check: **only when health equals 1** is
  `hit_damage_pending` cleared to zero
- collision off

That 1-health test is deliberately done before Stage 2 applies pending damage,
matching the arcade reaction-hook ordering.

## Shared helpers made explicit

Stage 4 exports the already translated REACT1 helpers:

- `wm_arcade_react1_block_hit`
- `wm_arcade_react1_block_hit_flail`

REACT2 calls those helpers directly in the arcade source. Exporting them avoids
creating a second interpretation of block pushback/sound/animation behavior.

## New cumulative reaction bridge

Use:

`wm_arcade_react12_reaction_callback`

as Stage 2's `wm_arcade_react_callbacks.reaction` callback after this merge.
It routes REACT2-owned IDs to Stage 4 and all other IDs to the Stage 3 REACT1
implementation. Unsupported later reactions still reach `unhandled_reaction`.

## New/extended adapter hooks

`wm_arcade_react1_callbacks_t` now also accepts:

- `get_health` — map to the port's equivalent of arcade `get_health`
- `victim_has_live_teammates` — map to `ck_live_teammates`
- `flash_white` — exact effect hook; do not substitute an unrelated effect
- `triple_sound` — receives the literal arcade sound selector (`0x33`/`0x43`)

The sound enum adds exact semantic families for uppercut, elbow-drop, and push.
The animation enum adds the REACT2 per-wrestler knockdown table.

## Still deliberately not implemented

No `REACT3.ASM` or later reaction routines are translated in this stage.
Do not guess them from attack names or from later Midway games.
