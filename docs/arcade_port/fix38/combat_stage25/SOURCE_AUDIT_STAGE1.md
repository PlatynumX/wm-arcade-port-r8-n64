# WWF WrestleMania Arcade -> N64 combat port: Stage 1 source audit

Date: 2026-08-19

This package is intentionally source-only. It does **not** build or modify an N64 ROM.
It is a clean C semantic translation of a first combat slice from the original arcade
source, designed to be merged onto the latest N64-port tree in a separate conversation.

## Original source used

Repository: `historicalsource/wwf-wrestlemania`

- `COLLIS.ASM`
  - `overlap_collision` (player body separation), source lines 53-251
  - `set_collision_boxes`, source lines 255-349
  - `check_collisions`, wrestler offensive pass, source lines 359-472
  - `check_collis`, source lines 479-653
  - `set_xyz`, source lines 658-705
  - `wres_collis_off`, source lines 712-717
- `GETUP.ASM`
  - `set_getup_time` and `hit_table`, source lines 29-179
- `PLYR.EQU`
  - player modes, attack modes, status bits
- `ANIM.EQU`
  - animation mode flags (`MODE_CHECKHIT`, `MODE_NOCOLLIS`, `MODE_STATUS`, etc.)
- `GAME.EQU`
  - move directions and `STAY_TIME = 270`
- `DISPLAY.EQU`
  - horizontal flip flag (`M_FLIPH = 0x10`)
- `DAMAGE.EQU`
  - attack damage constants, reduced-damage expressions, AI attack-type IDs

Raw source base:
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/`

## Exact behaviors preserved in Stage 1

1. Hurt box Y and X are taken from the current animation frame metadata; Z depth is
   mode-dependent: normal `[-30,+30]`, on-ground `[-15,+15]`, running `[-5,+5]`.
2. Horizontal flip mirrors both hurt and attack boxes using the arcade equations.
3. Attack-box edge contact is a hit candidate; the source rejects only strict gaps.
4. `MODE_NOCOLLIS`, immobilization, smart-target, combo-target, dead/live-team,
   zombie, pinned, ring-side, push-vs-air, and push-immunity gates are preserved.
5. Only flykick, turnbuckle stomp, and turnbuckle elbow-drop can hit a victim while
   that victim's `B_PUSH` status is set.
6. `MODE_WAITHITOPP` is cleared on accepted hit and both animation counters are zeroed.
7. Block state is sampled into the attacker's `HITBLOCKER` before the hit callback.
8. Hit side is derived from attacker-victim X and full Z positions and written to both.
9. Successful offensive collision sets attacker's `MODE_STATUS` before `wrestler_hit`.
10. The collision pass alternates attacker iteration direction each round tick, scans
    defenders forward, and exits the whole pass after the first accepted hit.
11. Body overlap separation reproduces the arcade axis choice, `0x3d` Z-glitch guard,
    mode exceptions, and +/-3 orthogonal nudge.
12. Get-up time is not overwritten if already nonzero. Flykick, entry-10/hiptoss,
    big boot, and big knee set `STAY_TIME` (270). The original MAYBE_GIDDUP hook mask
    is also preserved separately.
13. Damage expressions remain integer expressions so truncation matches the assembler.

## Deliberately NOT claimed as ported yet

This stage does not claim the whole combat system. In particular it does not yet port:

- `wrestler_hit` damage/reaction dispatcher itself
- animation command interpreter combat opcodes (`ANI_ATTACK_ON`, `ANI_DAMAGE`,
  `ANI_DAMAGEOPP`, attach/detach/puppet/slave commands, etc.)
- input-to-move selection and special-move recognition
- grapple/headhold/master/slave state machines
- wrestler-specific move scripts and finishers
- combo scoring/activation logic
- pin/referee/round-end logic
- ring ropes/turnbuckles/ring-out behavior
- combat sound/debris/sweat/shake side effects
- drone/AI decision logic
- special-object collision lists from the latter part of `COLLIS.ASM`

Those should be ported from source in later stages rather than approximated.

## Important merge note

`wm_arcade_actor_t` is an adapter containing only fields needed by this translated slice.
It is **not** meant to replace the current port's wrestler structure. During merge, map the
latest port fields to these semantics or transplant the functions directly onto the existing
structure. Do not guess missing field meanings: compare them against `PLYR.EQU` first.
