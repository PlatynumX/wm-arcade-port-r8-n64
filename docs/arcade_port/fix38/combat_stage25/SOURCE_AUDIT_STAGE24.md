# Stage 24 source audit — special-object / projectile combat direct port

Stage 24 ports the combat-relevant special-object system directly from the original arcade source. It does not replace it with a generic projectile framework.

## Arcade source boundaries

Primary source files:

- `COLLIS.ASM`
  - `object_collisions`
  - `objlist2player_collis`
  - `objlist2objlist_collis`
  - `set_spobj_boxes`
- `REACT1.ASM`
  - `wrestler_hit_special`
  - shared `hit_stuff`
- `SPECIAL.ASM`
  - three special-object lists
  - list insertion/deletion
  - `doink_pie`
  - `bam_fireball`
  - `und_spirit_pull`
  - `und_spirit_push`
  - `yok_salt_spray`
  - `special_hit`
  - combat-relevant animation opcodes/timing for spirit/reaper/salt
- `SPECIAL.EQU`
  - special-process data layout
- `MPROC.ASM` / `MPROC.EQU`
  - `GETPRC` / process creation semantics used to audit recycled process data

Repository: `https://github.com/historicalsource/wwf-wrestlemania`

## Direct-port implementation

New Stage 24 files:

- `wm_arcade_special.c`
- `wm_arcade_special.h`
- `test_combat_stage24.c`

Shared code extended only where the arcade itself shares behavior:

- `wm_arcade_combat.h`: `USR_VAR2` and `PLYR_SIDE`-equivalent adapter fields
- `wm_arcade_react.c/.h`: shared `hit_stuff` can now receive an opaque projectile-process identity instead of pretending the projectile is a wrestler
- `wm_arcade_react1_core.h`: exact special-hit reaction animation groups

## Exact collision-list behavior

The arcade uses three lists:

1. player-1 special objects
2. player-2 special objects
3. neutral special objects

`object_collisions` performs these operations in this exact order:

1. calculate boxes for P1, P2, neutral lists
2. collide P1-special list against P2-special list
3. collide P1-special list against wrestlers
4. collide P2-special list against wrestlers
5. collide neutral-special list against wrestlers

Each object-to-wrestler list scan stops after its first successful wrestler hit. The source comment mentions checking the projectile's "same side", but the executable routine contains no side/team exclusion there. Stage 24 therefore does not invent one.

The X collision box is mirrored exactly from `SP_OBJCONTROL`'s flip bit. Y/Z box construction and inclusive overlap comparisons follow `COLLIS.ASM`.

## Live collision IDs

The source `splat_tbl` contains three explicit IDs:

- `0`: Undertaker spirit
- `1`: Undertaker reaper
- `2`: Yokozuna salt

Taker and Yoko explicitly write those IDs in their constructors.

### Pie/fireball recycled-ID quirk

`doink_pie` and `bam_fireball` insert their process in the same special-object collision lists but do **not** write `SP_ID`.

The Midway `GETPRC` allocator unlinks a recycled process block and initializes its wake state/ID, but does not clear the process data block. `process_init` builds the free list without clearing every PDATA field. Therefore an unwritten `SP_ID` can retain the prior contents of that process slot.

Stage 24 preserves that behavior:

- `wm_arcade_special_obj_init()` represents cold initialization of a process slot.
- later pie/fireball constructor calls preserve the slot's existing `id` because the source does not overwrite `SP_ID`.
- spirit/reaper/salt constructors overwrite `id` because the source explicitly does so.

This is intentionally not simplified to "pie/fireball are always ID 0".

## Unchecked `special_hit` table access

`SPECIAL.ASM::special_hit` deletes each colliding projectile from its list and indexes `splat_tbl` directly from `SP_ID` with **no range check**.

This is materially different from `REACT1.ASM::wrestler_hit_special`, whose non-salt splat path does range-check and falls back to the first splat entry.

Portable C cannot safely reproduce an arbitrary out-of-bounds code-pointer fetch from adjacent original arcade memory. Stage 24 therefore preserves all known side effects up to that source hazard and reports it explicitly:

- the first object is deleted before the lookup, matching source order;
- valid IDs 0..2 use the exact three splat entries;
- an out-of-range ID sets `source_unchecked_splat_id` / `unresolved_unchecked_splat_id` and **does not invent a substitute animation**.

That unresolved case must not be "fixed" during the N64 merge unless the original memory behavior is deliberately emulated from the source/ROM layout.

## `wrestler_hit_special` direct behavior

### Salt (`SP_ID == 2`)

- owner `LAST_HIT_TIME = PCNT`
- shared `hit_stuff`
- block:
  - no health damage
  - victim slides at +/-3.0 X velocity
  - block sound + block reaction
- unblocked:
  - scream
  - `-D_SALT` health change (15)
  - mode-normal rule preserves `MODE_DEAD`
  - victim Z becomes owner Z - 1
  - sand/head-hit reaction
  - `USR_VAR1 = 0`
  - delay meter = `8*60`
  - X velocity +/-1.0 by victim facing
  - Y velocity +3.0
  - victim/projectile Z velocity `0x7000`
  - projectile X velocity arithmetic-shifted right one
- projectile changes to `saltsplat_anim`; collision is disabled by that animation path rather than by an invented immediate unlink.

### Blocked-salt register quirk

The source loads `1` into `a14` but stores **`a4`** into Yoko's `USR_VAR2`.

`COLLIS.ASM::objlist2player_collis` loads projectile `SP_COLLZ1` into `a4` before calling `wrestler_hit_special`. The full `REACT1.ASM::hit_stuff` body does not modify `a4`. Therefore the executable path writes the projectile's collision-box Z1 value, not the apparent intended constant `1`.

Stage 24 preserves that executable behavior exactly.

### Spirit / reaper / non-salt

- blocked paths skip special damage and push victim +/-2.0
- ID 0 spirit immobilizes victim for 60 ticks and does no immediate health damage
- nonzero non-salt IDs execute `-3` health damage; the source comment says 2 but the executable instruction is `movi -3`
- projectile is deleted from its special list before its splat selection
- REACT1's explicit range check falls out-of-range IDs back to spirit splat
- normal grounded live victim receives body-hit reaction, Y velocity +3.0 and zero Z velocity
- spirit sets `USR_VAR1=1`, delay `10*60`, and uses the exact 0/4.0 X-velocity distance rule at `0x5c`
- reaper/default nonzero uses the exact 3.0 push-away rule

## Combat-relevant animation timing

Stage 24 does not collapse animation-script timing into immediate state changes.

### Reaper

From `reaper_grow` / `reaper_anim`:

- collision disabled (`zoff=-1000`)
- five `WL 1` grow frames
- source `set_xv` changes X velocity from +/-4.0 to +/-7.0
- reaper animation begins
- two `WL 3` frames remain collision-disabled
- only after those six additional ticks does the live `(-10..+10 depth)` collision box activate

The portable update order is:

1. `wm_arcade_special_velocity_add(obj)`
2. `wm_arcade_special_tick_source_state(obj)`

which mirrors the source projectile-process loop where velocity update occurs before sleep/animation advancement.

### Salt

- grow collision disabled
- one `WL 1` grow frame
- `set_xv`, then live salt collision box immediately begins
- live `SALT01` persists `WL 20`
- then source `ASP_ZEROVELS`, gravity=0 and collision-off box execute

Those combat-state transitions and tick counts are direct-port state in Stage 24.

## Constructors ported

Combat-relevant constructor state is ported for:

- Doink pie: X offset 86, X velocity 6, Y offset 97
- Bam Bam fireball: X offset 86, X velocity 6, Y offset 97
- Taker spirit: ID 0, X offset 32, X velocity 7, Y offset `0x36`
- Taker reaper: ID 1, X offset 2, initial X velocity 4, Y offset `0x2e`
- Yoko salt: ID 2, X offset `0x36`, initial X velocity 7, Y offset `0x5b`, gravity `0x4800`, Y velocity `0x30000`

Facing flip negates X offset/velocity exactly where the source does.

## Explicit integration seams — do not approximate

The following are not replaced with fake portable behavior:

- actual arcade image pointers / image DMA objects
- shadow object allocation/rendering
- world-scroll screen-space update code
- off-screen object destruction based on `WORLDTLX`
- visual-only image-frame playback after collision has been disabled
- visual debris / salt explosion processes
- original hardware `DMAWNZ` rendering flags beyond the collision-relevant flip bit

Those must be wired to exact ported/extracted renderer/process implementations when merged into the N64 tree. The Stage 24 combat rules, collision timing and state changes should not be rewritten while doing so.
