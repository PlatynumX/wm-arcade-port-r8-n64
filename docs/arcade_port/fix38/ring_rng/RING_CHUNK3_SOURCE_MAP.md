# Ring Chunk 3 source map

Ring Chunk 3 corrects an earlier audit mistake: these routines are not
missing shared-library functions. Their bodies are present in the WWF source.

## WRESTLE2.ASM

Directly translated in `wmania_ring_climb.*`:

- `climb_turnbuckle`
- `ck_climb_out_bot`
- `ck_climb_in_top`
- `ck_climb_out_top`
- `ck_climb_in_bot`
- `ck_climb_out_side`
- `ck_climb_in_side`
- `idiot_check`
- `any_opp_outside`
- `face_turnbuckle`
- `climb_anims`
- all six climb-in/climb-through animation tables
- `rollthru_top_anims`

### Exact climb quirks retained

- `idiot_check` tolerates 2–3 calls per PCNT tick by refusing to advance
  when `CLIMB_LAST == PCNT`.
- `IDIOT_COUNT = 21`.
- Zombie `ck_climb_out_top` bypasses the normal mode/position/button checks.
- Running `ck_climb_in_side` bypasses the normal interruptibility/mode
  restrictions and button requirement, but still rejects nonzero GETUP_TIME.
- Turnbuckle near misses force Z to exactly `RING_TOP`.
- Turnbuckle occupancy uses strict left/right comparisons around
  `RING_X_CENTER`.
- Side entry requires the exact external `calc_line_x` result to be within
  10 pixels of the selected collision edge.

### `ck_climb_out_side` source bug/quirk

Current source:

```asm
callr any_opp_outside
jrnc  #done
move  *a0(CLIMBING_THRU),a0
CMPI  1,A0
JREQ  #done
```

`any_opp_outside` explicitly documents `a0` as trashed and uses it as the
post-incremented `process_ptrs` cursor. It does not return a wrestler
process pointer in `a0`.

The portable port therefore does NOT silently replace this read with the
opponent's CLIMBING_THRU or the current player's CLIMBING_THRU.

`WmRingSourceQuirkReadFn` asks the merge runtime to provide the literal word
read from the source-equivalent address after the found opponent slot.

## SPECIAL.ASM

Directly translated in `wmania_ring_out.*`:

- `do_ringout_dufus`
- `kill_when_hit_ground`
- `ARE_WE_IN_RING`

### `ARE_WE_IN_RING` exact behavior represented

- HALT early return
- DEAD special RING_TIME handling
- closest-opponent-dead early return
- signed inside/outside RING_TIME transitions
- create `kill_when_hit_ground` when crossing outside with ring-outs enabled
- `do_ringout_dufus`
- seven-TSEC outside damage threshold
- damage only on every eighth PCNT
- opponent RING_TIME gate
- sleeping-drone `0x7fff - PTIME` adjustment
- `adjust_health(-1)`
- death by ring-out
- living/zombie teammate disqualification suppression
- `CREATE_DISQUAL`
- `announce_rnd_winner`

### `kill_when_hit_ground`

The helper waits until:

`GROUND_Y == OBJ_YPOSINT`

then performs:

`adjust_health(-150, PLYRNUM, a10=0)`

and dies.

The portable function exposes this as a one-tick readiness/apply pair so the
N64 scheduler can preserve the original asynchronous process behavior.

### `do_ringout_dufus`

The source message condition is intentionally odd:
- it exits when `ring_out_on != 0`
- self must have been outside more than 4*TSEC
- every active opponent from another side must have RING_TIME > +4*TSEC
- message number is 3 ("Get back in dummy")

No dead-opponent filter is added because the source does not have one.

## Still unresolved

`keep_onscreen`

A continued repository audit checked:
- WRESTLE.ASM (reference only)
- WRESTLE2.ASM
- SPECIAL.ASM
- ADJUST.ASM
- ANIM.ASM
- COLL2.ASM
- COLLIS.ASM
- DRONE.ASM
- GETUP.ASM
- backup WRESTLE2/SPECIAL passes performed earlier

No definition has yet been found.

Do not substitute inferred fence/clamp behavior for it.
