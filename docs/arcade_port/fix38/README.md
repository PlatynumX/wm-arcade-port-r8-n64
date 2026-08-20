# Fix38 — cumulative source-port merge

This package merges the two supplied handoff bundles into the current WrestleMania Arcade N64 port baseline:

- Combat Stage 25 DRONE CORE (Stages 1–25 cumulative)
- Conversation-complete Ring Chunk 4 + shared RNG + hi-score + non-gameplay attract package

## What Fix38 does

- Copies every production `wm_arcade_*.c/.h` combat module into the main source tree.
- Copies every production `wmania_*.c/.h` RNG/hi-score/attract/ring/rope module into the main source tree.
- Rewrites only header include paths so the modules live cleanly under `include/wm/arcade/`; source logic is not rewritten.
- Adds every merged `.c` file to both the libdragon Makefile build and the host CMake build.
- Preserves both bundles' audits, source maps, handoffs, and tests in the repository.
- CI-builds the merged N64 ROM and downloads the successful artifact.

## Deliberately not activated in Fix38

The current main-port gameplay shell is still too thin to bind these seams without inventing behavior. Fix38 therefore compiles the translated systems into the ROM but does not fabricate adapters for:

- DRONE raw AI tables/named scripts still listed in `DRONE_SOURCE_SEAMS.md`.
- Ring `keep_onscreen`, which remains absent from the located historical source.
- Attract gameplay demos, intentionally absent from the supplied attract package.
- N64 mapping for the TMS34010 RNG's dynamic `HCOUNT` and source `SP` entropy inputs. The exact RNDRNG family is merged; the existing frontend RNG bridge is not silently replaced with a guessed platform mapping.
- Hi-score persistence device, initials UI ownership, and result hooks that depend on real match completion.
- Combat/ring renderer, animation, process, and player-state adapters that depend on the full gameplay runtime.

This is intentional. Fix38 makes the cumulative direct-port modules part of the baseline and compiles them continuously, then gameplay seams can be wired from the original source instead of recreated.
