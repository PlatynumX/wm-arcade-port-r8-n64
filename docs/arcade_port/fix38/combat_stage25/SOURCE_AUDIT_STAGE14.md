# Source Audit — Stage 14: Bret Hart character combat

Stage 14 begins the wrestler-specific combat phase and ports Bret Hart's character
control/input layer from `BRET.ASM`.  It is cumulative with Stages 1–13 and does
not build a ROM.

Primary source:
- `BRET.ASM` — historicalsource/wwf-wrestlemania
- `GAME.EQU` — button/direction values
- `MACROS.H` — `FACE24` and shared macro semantics

## Ported in this stage

### Character dispatcher and live modes
- `bret_ani_init` facing-dependent stand/torso initialization.
- `move_bret` 26-entry player-mode dispatch table.
- Bret-owned implementations of normal, running, attached, bouncing,
  turnbuckle, block, waitanim, master, headhold and headheld behavior.
- Source no-op modes remain no-ops: in-air, on-ground, dizzy, opponent-overhead,
  climb-turnbuckle, grapple, slave, puppet2 and chokehold where `BRET.ASM`
  contains only `rets` (or commented-out code).
- Shared/external modes (`mode_dead`, `mode_puppet`, `mode_inair2`,
  `mode_choking`) remain explicit adapter callbacks rather than recreated code.

### Normal/run action selection
- Exact 32-entry Bret action table.
- Punch/headbutt/ground-punch proximity routing.
- Block and block-time behavior.
- Super-punch/uppercut/butts/ground hair-pickup/shooter routing.
- Kick/knee/stomp/turnbuckle routing.
- Super-kick/knee-fall proximity and mode routing.
- Punch+kick start-run behavior.
- Running velocity, hyper-speed shift, braking, Z drift, running DDT/ground
  punch/flying-kick selection.
- Rope-bounce state transition.
- Turnbuckle leap and block-push handling.
- Bret headhold/headheld action behavior and bozo reversal hooks.

### Secret moves
Literal recognizer descriptors are exported for the eight data-driven records:
- neck grab
- grab fling
- hip toss
- quick grab-fling/hip-toss chords
- face rake
- jump kick
- supercut

`charge_ddt` is intentionally NOT represented as a fake sequence record; the
source implements it as executable button-release/power-duration code, so it is
ported as `wm_arcade_bret_try_charge_ddt`.

### Persistent special-move processes (`hrt_smove_table`)
Exact input descriptors and post-match handler behavior are exported for:
- roll uppercut
- headhold combo punch
- headhold combo kick
- headhold piledriver
- headhold DDT
- headhold faceslam
- airborne/close-range grab toss
- finish move 1
- finish move 2

The headhold power-move handlers preserve the source reversal split:
- when Bret owns the hold, target `WHOIHIT`, award 35/16/20 bonus, immobilize
  target 15 ticks;
- when Bret is `HEADHELD`, reject `I_WILL_DIE`/immobilized state, invoke the
  reversal hooks, target `WHOHITME`, and immobilize that target 15 ticks.

The two combo processes require the shared `CHECK_COMBO_GO` callback before the
matched move can queue.

Charge flying-kick and charge face-rake release bodies are also ported.

### Movement/animation tables
- Exact 8-way Bret velocity table (`0x3a000` / `0x31000`).
- Exact source-label 4x4 rotation table.
- Exact source-label 8x8 leg-walk animation table.
- Exact source-label 4x4 torso-turn table.

The string tables are resolver keys naming original source symbols.  They are not
replacement assets or invented animation IDs.

## Integration seam

New files:
- `wm_arcade_bret.h`
- `wm_arcade_bret.c`
- `wm_arcade_bret_tables.h`
- `wm_arcade_bret_tables.c`
- `test_combat_stage14.c`

The N64 merge must map native player/process fields into the added Stage-14
adapter fields and resolve animation/sound tokens to already-ported/native
assets.  Do not replace the native N64 wrestler struct with `wm_arcade_actor_t`.

The shared input-history/`WAITSWITCH_DWN` scheduler is intentionally not cloned
inside Bret.  It should consume `wm_arcade_bret_secret_patterns` and
`wm_arcade_bret_monitor_patterns`, then invoke the corresponding Bret handler.
Likewise `std_walk_fast` and `std_taunt` remain shared standard processes; they
are entries in Bret's source special-move process table, not Bret-specific
implementations.

## Not claimed by Stage 14
- Razor/Taker/Yoko/Shawn/Bam/Doink/Lex character modules.
- `wrestler_hit_special` special/projectile-object combat.
- Full shared input-history/process scheduler implementation.
- Full animation VM or presentation systems.

No approximation/recreation is intended by those boundaries.
