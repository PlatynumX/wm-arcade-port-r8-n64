# Combat2EC strict source-proof ledger

Rule: **same purpose is not a port**.  A game-visible behavior is marked PROVEN only when the original Midway source/data establishes that behavior.  Platform-specific N64 substitutions are allowed only at the hardware boundary.

## Proven / directly source-backed

- **Main RNG update**: WRESTLE.ASM main loop rotates RAND, mixes HCOUNT and SP, and writes RAND each frame. Existing RNG port/tests retain this source order.
- **ANIM primary/secondary restart semantics**: ANIM.ASM `change_anim1/change_anim1a` and `change_anim2/change_anim2a` use independent ANIMODE/ANICNT channels, avoid restarting the same unfinished script, reset count to 1, and immediately animate after a restart. The live bridge is required to retain those semantics.
- **WRESTLE movement idle path**: source zeroes MOVE_DIR/XVEL/ZVEL and calls `set_rotate_anim` then `change_anim1`; directional paths call `set_velocities` and `change_walk_anim`.
- **Attract gameplay authority**: ATTR.ASM owns normal gameplay-demo selection and launches a real match; presenter/demo screen coordinates are not gameplay-world authority.
- **Bam temporary palette change**: BAMSEQ2.ASM `set_pal` explicitly calls `pal_getf(BAMBLU_P)`, saves OBJ_PAL to MY_PAL, and installs the returned palette. This proves palette selection is a game-code operation separate from image selection.
- **DRONE generated data requirement**: live generated DRONE tables/scripts/services must come from historical DRONE.ASM; committed placeholder headers cannot be accepted by a release build.

## Not proven and therefore blocked / fail-closed

- **WIMP 1..N -> 0..N-1 pixel normalization**: no original routine has been established that subtracts one from image indices. Removed from live conversion in EC.
- **WIMP palette-window inference from undocumented palette-record words**: useful research hypothesis only; not allowed in live conversion.
- **WIMP concatenation of following palette directory records into a 256-entry bank**: not established by original loader code; not allowed in live conversion.
- **`live_no_teammates() == 0` shortcut**: contextually plausible for 1v1 attract but not a translated teammate routine. Source-proof gate rejects treating it as complete parity.
- **Any unresolved reachable ANI_CODE or DRONE command-5 native service**: must remain visible/unresolved until its original routine is translated; no silent success/no-op may count as parity.
- **`keep_onscreen`**: WRESTLE.ASM calls it, but the previously audited historical payload did not establish its body. Do not replace it with inferred coordinate clamping.

## Requires direct source proof before “complete combat port” claim

- RNDPER exact comparison/boundary semantics used by the live runtime.
- Full teammate/query behavior used outside the current 1v1 attract context.
- Every reachable DRONE command-5 target and every reachable wrestler ANI_CODE target.
- Character-specific logic for all eight shipped wrestlers, branch/table by branch/table.
- Full collision/hit side effects and WRESTLE process ordering beyond already-proven call order.
- Original LOADW/LOAD2/WIMP palette/index semantics, especially Bam frames that address index 64 or 255 with a 64-entry palette record.

Combat2EC intentionally prefers a source-conversion stop over a plausible but unproven sprite transformation.
