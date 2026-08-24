# Combat2DR — ATTR.ASM runtime identity / actor-chain parity

Hardware result from Combat2DQ proved the corrected build graph was active, but exposed two runtime interface faults: the first gameplay demo could present Bret/Bret frozen in stance, while a later demo could display a garbled wrestler and move outside the ring.

Combat2DR keeps DQ ownership unchanged and hardens the actual ATTR.ASM identity chain instead:

* ATTR.ASM source wrestler IDs are mapped fail-closed. An unmappable source ID no longer silently aliases to frontend slot 0 / Bret.
* The renderer-only `wm_demo` fighter records are synchronized to the exact `WmAttractDemoPlan` source IDs at match creation. They remain presentation storage and do not tick or own gameplay.
* Immediately after `wm_fix39_match_begin`, both live actor `wrestler_num` values are checked against the exact ATTR.ASM plan before CPU-vs-CPU execution begins.
* The N64 runtime sprite override no longer falls through to stale presenter/Bret visual state when a streamed source frame is absent. It falls back only to the same live wrestler's base sprite, preventing cross-wrestler CI8/TLUT pairing.
* A build-time DRONE executable-service coverage diagnostic now prints translated service-body coverage. Historical DRONE command #5 is a real CALL; missing handlers remain explicit parity work and are never treated as successful execution.

This pass intentionally does not clamp ring coordinates or invent AI behavior. The next runtime work should use the reported DRONE service coverage plus hardware behavior to finish any still-untranslated source CALL bodies rather than masking them.
