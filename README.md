# WWF WrestleMania Arcade -> N64 source port (r9 source-engine pass)

**Primary target:** Nintendo 64 / libdragon.  
**Porting rule:** when the arcade source implements it, translate that implementation and use its original data/assets. Do not replace it with invented N64 behavior.

r9 is the first broad **shared-engine** pass. It is not a claim that the complete arcade game is already playable. The point of this revision is to replace one-off bring-up code with reusable source-driven systems that whole parts of the original program can execute on.

## What r9 actually translates

- **53 Hz arcade source clock.** `DISPLAY.EQU`'s source tick rate runs independently of 60 Hz N64 presentation, so translated sleeps, timers, processes and animation cadence are no longer accelerated to 60 Hz.
- **Cooperative source-process scheduler.** Shared CREATE/sleep/kill-by-PID behavior is used by translated frontend processes instead of hand-coded screen timers.
- **Packed BMOD records.** The original 64-bit background block records are decoded generically; CI extracts `NTITLESCBMOD` and `SPORTBKBMOD` directly from `BGNDTBL.ASM`.
- **Typed animation stream.** Mixed WORD/LONG operands are preserved. Implemented commands execute source operand widths and direction rules; unknown commands fail closed instead of being guessed.
- **Whole-source dependency IR.** CI scans every `SUBR`/`SUBRP`, static call/process edge and unresolved dynamic edge, with frontiers rooted at `attract_mode` and `start_match`.
- **Whole-source animation IR.** CI mechanically preserves typed WORD/LONG animation data and W/L packing macros across the historical ASM tree.
- **Source selection core from `SELECT.ASM`.** The 2x4 cursor grid, exact crouton coordinates, source BMOD placement, player start/mug/sound data, `scramble_table` roster mapping, attributes, legal four-way movement, START+UP random-selection gate, random wander/home rules, 15-second source selection timer and 30-tick final wait constants are translated in portable C.
- **Source-driven attract execution.** Initial 8-tick blank, active source routine order, DCS/Midway/title timing, Midway background movement and title lava process scheduling remain source-driven.
- **Original artwork paths already proven on N64.** Midway Sports uses the original 17 WIMP pieces; title art is derived from original artist/BMOD data; Bret remains the first wrestler with the current visual backend.

## What is deliberately *not* called a port yet

- `show_gameplay` / `start_match`: the old combat sandbox remains a **harness only** and cannot be entered by normal arcade flow.
- Full credit/buy-in/start/select orchestration: the selection mechanics/data core is translated, but original credit processes, text, mug objects and final screen rendering still need their shared backends.
- The complete 131-command animation interpreter, branch/pointer relocation, all wrestler visual banks and wrestler-specific process logic.
- Full background header/palette object construction, fonts/text, fades and remaining frontend routines.
- Match process graph, collision/ropes/turnbuckles, grapples, damage/pins/finishers, original CPU behavior and DCS audio backend.

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
