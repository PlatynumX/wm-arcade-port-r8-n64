# Source Audit — Stage 15: Razor Ramon Character Combat Layer

Primary arcade source:

- `RAZOR.ASM` — historicalsource/wwf-wrestlemania
  https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/RAZOR.ASM

Stage 15 is the second wrestler-specific combat translation, layered on the
cumulative Stage 1–14 combat package. It does not build a ROM and does not
replace the N64 port's native wrestler/process structures.

## Ported in Stage 15

### Razor character dispatch/state layer

`wm_arcade_move_razor()` preserves the source `move_razor` control seam:
secret-move checking first, then dispatch by `PLYRMODE` through the 26-entry
mode table. Source-shared modes remain callbacks/services instead of duplicated
character-local implementations.

Character-local executable mode behavior includes the normal, running,
bouncing, turnbuckle, block, headhold and headheld paths, with attachment,
master, puppet, inair2, dead, wait-animation and choking paths handed to the
shared adapter callbacks where the arcade source does likewise.

### Exact 32-entry attack/action routing

The five arcade attack bits remain the existing cumulative button values and
index the same 32-way action selection used by `RAZOR.ASM`.

The translated decisions include Razor-specific close/far and opponent-mode
branches for:

- punch/headbutt/ground offense;
- super-punch uppercut, upward slash and downward slash paths;
- kick/knee/stomp/running big-boot paths;
- super-kick and super-knee behavior;
- running attack conversion and flying-kick behavior;
- headhold up/down slash sequences and headhold uppercut damage setup;
- block push-out behavior.

### Secret input descriptors

`wm_arcade_razor_secret_patterns[6]` exports the literal sequence records for
the non-charge secret inputs so a shared input-history matcher can consume them.
The source charge-flying-kick process remains executable release logic rather
than being falsely converted into a static sequence.

Covered secret handlers include neck grab, grab fling, hip toss, the running
variants and down slash.

### Persistent special-move processes

`wm_arcade_razor_monitor_patterns[9]` and `wm_arcade_razor_fire_monitor()` cover
the Razor-specific persistent monitor layer:

- charged repeating slashes;
- headhold piledriver;
- headhold combo 1;
- Razor's Edge;
- headhold rug shake;
- airborne grab/toss;
- headhold combo 2;
- sliding rug;
- two source-conditional finisher entries.

Shared `std_walk_fast` and `std_taunt` stay shared and are not reimplemented as
Razor-only behavior.

### Source timing/behavior preserved

- Flying-kick release threshold: 85 ticks.
- Repeating slash charge threshold: 100 ticks.
- Recent neck-grab window: 2 seconds / 120 source ticks.
- Headhold power-move target immobilization: 15 ticks.
- Razor's Edge bonus message ID: 33.
- Piledriver bonus message ID: 7.
- Rug-shake bonus message ID: 6.
- Running close-vs-up-slash split is kept as source logic.
- Super-knee directional comparison uses `NEW_FACING_DIR & 0x0c` literally.
- The source flying-kick routine's `GETUP_TIME` reload quirk before the
  `MODE_ONGROUND` / `MODE_DEAD` comparisons is intentionally represented in the
  regression expectations rather than silently "fixed".

### Movement/animation resolver tables

`wm_arcade_razor_tables.c` carries the exact source-label resolver tables for
standing rotation, leg/walk direction and torso direction. These are names for
native animation resolution; they are not replacement animation assets.

## Deliberately outside Stage 15

- Native N64 input-history / `WAITSWITCH_DWN` scheduler implementation.
- Native animation pointer/address resolution and animation data import.
- Sound/effect asset integration.
- Shared pin, health, rope, attachment, puppet, dead and other common services
  already represented by callbacks or earlier stages.
- CPU/drone decision logic.
- The remaining six wrestler-specific character modules.
- Special-object/projectile combat integration.

Do not approximate any of these when merging. Map the portable adapter seams to
the existing N64 systems or translate the corresponding arcade source first.

## Verification

`test_combat_stage15.c` covers Razor normal/running routing, super-punch and
super-knee branches, 85/100-tick charge thresholds, the source flying-kick
quirk, neck-grab timing, Razor's Edge/piledriver/rug-shake monitor behavior,
headhold reversal routing, airborne toss selection, secret/monitor descriptors,
and exact movement/animation table labels.

The cumulative package passes Stages 1–15 under strict C11 flags and under
UndefinedBehaviorSanitizer. See `TEST_RESULTS.txt`.
