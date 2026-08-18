# r9 source-engine pass

r9 intentionally groups shared work instead of shipping another one-screen revision.

## Landed in this pass

- Original 53 Hz game tick separated from 60 Hz N64 video presentation.
- Shared cooperative source-process scheduler.
- Generic decoder/generated lookup for original packed BMOD records.
- Mixed WORD/LONG source animation runtime with fail-closed unsupported commands.
- Whole-tree source call/process dependency IR and `start_match` frontier.
- Whole-tree typed animation-data IR.
- Portable `SELECT.ASM` mechanics/data core: grid, roster rewiring, attributes, player metadata, legal cursor movement and random-select wander/home behavior.

## Next source blockers

1. Complete BMOD header + palette + original art object construction so every source background module can render through one backend.
2. Animation symbolic-reference relocation, branches/calls and the remaining original ANI command semantics.
3. Credit/start process state and full `select_screen` object/text/mug rendering on the shared backends.
4. Translate the `start_match` dependency closure: player process creation, wrestler process state, collision/ring/rope systems and match lifecycle.
5. Replace Bret-only asset binding with generated per-wrestler source banks.
6. Translate original DCS command/audio backend rather than approximating sound behavior.

The bring-up combat sandbox remains test-only. Normal arcade execution never falls through to it.
