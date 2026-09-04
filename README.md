# WWF WrestleMania Arcade -> N64 source port (r9 source-engine pass)

**Primary target:** Nintendo 64 / libdragon.  
**Porting rule:** when the arcade source implements it, translate that implementation and use its original data/assets. Do not replace it with invented N64 behavior.

r9 is the first broad **shared-engine** pass. It is not a claim that the complete arcade game is already playable. The point of this revision is to replace one-off bring-up code with reusable source-driven systems that whole parts of the original program can execute on.

## What r9 actually translates

- **53 Hz arcade source clock.** `DISPLAY.EQU`'s source tick rate runs independently of 60 Hz N64 presentation, so translated sleeps, timers, processes and animation cadence are no longer accelerated to 60 Hz.
- **Cooperative source-process scheduler.** Shared CREATE/sleep/kill-by-PID behavior is used by translated frontend processes instead of hand-coded screen timers.
- **Packed BMOD records.** The original 64-bit background block records are decoded generically; CI extracts `NTITLESCBMOD` and `SPORTBKBMOD` directly from `BGNDTBL.ASM`.
- **Original BLIMP title artwork.** `BIGWWF.BDB`/`BIGWWF.BDD` are decoded into their 40 CI8 blocks and six RGB555 palettes, cross-validated against `NTITLESCBMOD`, and rendered from the original packed block records rather than a flattened WIMP crop.
- **Original title sparkle artwork/process lifetime.** All 69 WIMP frames named by `MISC.LOD` from `SPARKLE.IMG` are generated as source sprites. `show_title` creates/kills the source `FLASH_PID` and `ATTRACT_ANIMPID` processes alongside `cycle_lava`. The shared `SPRINKLE_GLINTS`/`RANDOM_SPARKLE` helper bodies and `WHERE_WRESTLMANIA_SPARKLES` table are absent from the checked-in WWF tree, so placement/random cadence is clearly marked provisional rather than source-exact.
- **Typed animation stream.** Mixed WORD/LONG operands are preserved. Implemented commands execute source operand widths and direction rules; unknown commands fail closed instead of being guessed.
- **Whole-source dependency IR.** CI scans every `SUBR`/`SUBRP`, static call/process edge and unresolved dynamic edge, with frontiers rooted at `attract_mode` and `start_match`.
- **Whole-source animation IR.** CI mechanically preserves typed WORD/LONG animation data and W/L packing macros across the historical ASM tree.
- **Source selection core from `SELECT.ASM`.** The 2x4 cursor grid, exact crouton coordinates, source BMOD placement, player start/mug/sound data, `scramble_table` roster mapping, attributes, legal four-way movement, START+UP random-selection gate, random wander/home rules, 15-second source selection timer and 30-tick final wait constants are translated in portable C.
- **Source-driven attract execution.** Initial 8-tick blank, active source routine order, DCS/Midway/title timing, Midway background movement, title lava scheduling, and the source title sparkle process lifetime remain source-driven.
- **Original artwork paths already proven on N64.** Midway Sports uses the original 17 WIMP pieces; title art is derived from original artist/BMOD data; Bret remains the first wrestler with the current visual backend.
- **`show_gameplay`'s attract-mode `start_match` creation path.** `ATTRACT.ASM::show_gameplay` is just `WRESTLE.ASM::start_match` run with `PSTATUS==0`; the `#0plyr` branch's `RNDRNG0(7)`-skip-7 wrestler draw, `LIFEBAR.ASM::init_life_data` (`LIFE_MAX`=163), and PLYRNUM/PSIDE wrestler-actor creation are translated (`wm/match.h`), on the real `SLEEP 3*60` + `wait_on_butn 10*TSEC` timing. See the boundary below for what this does not yet do.
- **Bret's live control layer and a real visual backend for it.** Whichever actor draws `WM_ROSTER_BRET` runs the actual `wm_arcade_move_bret` dispatcher every tick, not a harness. `wm/bret_backend.h` resolves `WM_BRET_ANIM_STAND2/4`/`TORSO2/4` and 6 of Bret's light/power punch/kick animation ids to their real `wm_visual_sequence` data and drives two `wm_visual_state` tracks (body + torso) from it -- the merge adapter `wm/arcade/wm_arcade_bret.h` itself calls for. Every other animation id, and every other wrestler, still resolves to nothing.
- **Real movement physics, and a real walk cycle to go with it.** `wm/movement.h` translates `WRESTLE.ASM::convert_facing` and `set_velocities` exactly (per-direction velocity table, `walk_fast`/opponent-ONGROUND-or-DEAD 1.5x boost, backward-facing 0.9x reduction) and wires `execute_walk`'s `MOVE_DIR`/`OBJ_CONTROL` (`M_FLIPH`) dispatch as Bret's real `execute_walk` callback. `wm/bret_backend.h` also now runs `change_walk_anim`'s leg-cycle half: all 12 `hrt_walkM_fF_anim` sequences `hrt_leg_anims_table` (`BRET.ASM:2897`) can select are extracted and wired, so a Bret actor with real input genuinely walks with the correct animation, not just a position change under a frozen sprite. The torso half, and idle-turn animation, are not translated -- see the boundary below.
- **A real, human-playable single-player match for Bret.** `WM_APP_MODE_MATCH_INIT` (after select/pregame) is no longer a dead end: `wm_match_start_selected` translates `start_match`'s PSTATUS!=0 `#1plyr` path using whatever the select screen actually chose, and `wm/human_input.h` maps real controller state onto the same `but_val_cur`/`stick_val_cur` fields `DRONE.ASM`'s commit path writes for CPU wrestlers, so a human playing Bret drives the real `wm_arcade_move_bret` dispatch, real movement and walk animation, and the real visual backend above with actual button/stick presses.
- **Real attack-box activation for Bret's 6 wired attacks.** Each one now fires `ANIM.ASM`'s real `ATTACK_ON`/`ATTACK_ON_Z`/`ATTACK_OFF` (`wm_arcade_ani_attack_on(_z/_off)`, real and ctest-verified since fix38) at the exact frame hand-traced from its own `HRTSEQ2.ASM` source -- not guessed from the flattened `wm_visual_sequence` frame data, which (like every `wlanim.py --slice` extraction) linearly concatenates a source routine's branchy paths regardless of which one a real hit/block/miss actually takes. This sets a real `attack_mode`/`attack_xoff`/.../`WM_MODE_CHECKHIT` on the actor.
- **Hits actually connect.** The original `ANI3` hurt-box header (`SYS.EQU`'s `IANI3X`/`Y`/`Z`/`ID`) is confirmed unrecoverable from the checked-in tree: its `bretimg.tbl`/`bret.seq`/`imgtbl.glo` compile output is absent, and the WIMP `.IMG` directory format itself never carried it either (only `xani`/`yani`/`width`/`height`). `wm_bret_hurt_box_for_frame` (`wm/bret_backend.h`) substitutes each frame's own real WIMP image bounds instead -- real numbers, from a new geometry-only generated table (`tools/bret_geometry_bundle.py` -> `src/generated/bret_frame_geometry.c`) deliberately kept separate from `wm_bret_sprite_find`'s pixel/palette data, so hit-box math is engine logic that doesn't depend on which artwork happens to be linked in (host `ctest` included) and keeps working once stock art is replaced. `wm_bret_backend_tick` sets a real `hurt_box` from it every tick, and `wm_match_tick` now calls the real, ctest-verified-since-fix38 `wm_arcade_check_wrestler_collisions()`/`wm_arcade_wrestler_hit()` (`REACT1.ASM`) every tick via `wm_arcade_wrestler_hit_collision_callback`, with a real `adjust_health` translated from `LIFEBAR.ASM:1547-1591` (life clamped to `[0, LIFE_MAX]`, the 20+pt near-death "fudge", and "attract mode never dies"). A wired attack whose real attack box overlaps a real hurt box now genuinely reduces the victim's life.
- **Ordinary play actually throws a real punch.** `wm/arcade/wm_arcade_closest.h` translates `WRESTLE.ASM::calc_closest`'s per-opponent delta/distance computation (`closest_xdist`/`closest_ydist`/`closest_zdist`/`closest_dist` against `actor->smart_target`), and `wm_match_tick` now calls it every tick before `wm_arcade_move_bret`. `BRET.ASM`'s `do_punch` (`near(a,50,45)`) used to see a permanent `closest_xdist`/`closest_zdist` of 0 -- nothing computed them -- so every punch resolved to the unwired `WM_BRET_ANIM_BUTT2/4` headbutt regardless of range. With real distances, a human punching from realistic range now gets a real, wired punch animation and its real `ATTACK_ON` window through ordinary human-input play, not just direct backend calls.

## What is deliberately *not* called a port yet

- The rest of `start_match`: `INIT_LADDER_TABLE`/`CURRENT_LADDER`/`NUM_OPPS` multi-drone team selection (both `#0plyr` and `#1plyr` fall back to one placeholder opponent), ring/rope/crowd/timer process creation, collision/pin/finisher lifecycle, and the `#2plyr` (two-human) path.
- The rest of the already-ported combat core's connection to a live match: `wm_arcade_move_ported_wrestler`'s generic 8-wrestler dispatch isn't used yet (only Bret is wired, directly); the other seven wrestlers have no visual backend, movement, or human control at all -- and their CPU opponent still runs the same idle-only drone described next. `DRONE.ASM`'s AI script/range data (`wnshort_t`/`wnmed_t`/`wnlong_t`, `blkbase_t`/`blkatk_t`/`sklhhdly_t`/`sklhrdly_t`) is emitted by a source macro (`SKLM`) that is not present in the checked-in tree, so it is not guessed -- a CPU-controlled wrestler (attract-mode or the human's opponent) never actually throws a button/stick input, so it never moves or attacks.
- `change_walk_anim`'s TORSO reselection (`hrt_torso_anims_table`) and `set_rotate_anim`/`change_anim1` (the `#zip`/idle-turn case, needing `hrt_rotate_anims_table`'s 12 turn-transition sequences plus `hrt_stand6/8_anim`). Also, real `FACING_DIR` tracking needs a shared "compute `NEW_FACING_DIR`" routine this port hasn't located yet, so the leg-cycle selector substitutes `FACING_DIR=MOVE_DIR` while moving -- correct for walking in a straight line, the only case reachable today, but not source-exact once strafing (facing one way while moving another) exists. `CAN_MOVE_DIR` ring-boundary redirection (`execute_walk`'s down-left/down-right special case) is also not translated, since it depends on the unported ring-boundary system.
- `wm_arcade_calc_closest`'s own scope: it only computes real deltas/distance against the one fixed `smart_target` opponent this port ever has. `calc_closest`'s multi-candidate ranking loop (`biased_range`, `ONGROUND`/`WHOIHIT`/`INRING`/previous-closest/combo/Z-distance biasing, the running-behind-us skip rule, and `CLOSEST_NUM` selection itself) is not translated -- moot until a real ladder/teammate system exists to choose among more than one opponent -- and `calc_closest2`'s every-4th-tick recompute throttle (a pure CPU-cost optimization) isn't either, since recomputing one distance every tick is trivial. `wm_bret_backend_callbacks` also doesn't set `adjust_health` (only `wm_match_tick`'s own collision wiring does, not `wm_arcade_move_bret`'s per-actor reaction callbacks such as `teammate_pin`/`can_pin`). There's still no win condition, no `MODE_DEAD` transition, and no LIFEBAR combo/drone-count/speed-adjustment damage scaling.
- Full credit/buy-in/start/select orchestration: the selection mechanics/data core is translated, but original credit processes, text, mug objects and final screen rendering still need their shared backends.
- The complete 131-command animation interpreter, branch/pointer relocation, all wrestler visual banks and wrestler-specific process logic.
- Full background header/palette object construction, fonts/text, fades and remaining frontend routines.
- Collision/ropes/turnbuckles, grapples, damage/pins/finishers, original CPU behavior and DCS audio backend.

Anything still missing stays absent or is reported in the generated frontiers. Harness code does not substitute for the game.

## N64 controls

The N64 layer maps controller state into portable arcade actions. Current bring-up mapping is:

| N64 | Portable action |
|---|---|
| Analog stick / D-pad | movement / frontend direction |
| A | run |
| C-Left | light punch |
| C-Up | power punch |
| C-Right | light kick |
| C-Down | power kick |
| R | block |
| Start | arcade attract/start input path |
| Z | N64-only debug HUD |

The final frontend/gameplay input behavior follows source routines as they are translated; the harness mapping is not treated as authoritative arcade behavior.

## Host verification

```sh
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Release
cmake --build build-host --parallel
ctest --test-dir build-host --output-on-failure
./build-host/wm_headless
```

## N64 build

With libdragon configured:

```sh
export N64_INST=/path/to/n64_toolchain
make -j2
```

Output:

```text
wm_arcade_r9.z64
```

GitHub Actions fetches the historical WrestleMania source directly, regenerates translated tables/frontier reports and required artwork, builds the ROM, uploads the `wm-arcade-r9-build` Actions artifact, and publishes the ROM plus reports to the `rom-build` branch.

## Source/regeneration tools

```sh
sh scripts/fetch_original.sh
sh scripts/regenerate_source_data.sh
sh scripts/inventory_original.sh
sh scripts/build_source_ir.sh
sh scripts/build_animation_ir.sh
sh scripts/prepare_bret_sprites.sh
sh scripts/prepare_frontend_assets.sh
```

The historical tree under `original/` is a build input and is not bundled into this repository/package.

## Coverage

`port/translation_manifest.json` is canonical. CI regenerates `docs/PORT_COVERAGE.md` from it. `partial-source` means only the stated original subset is translated. `harness-only` means test/bring-up code that is forbidden from normal arcade execution.
