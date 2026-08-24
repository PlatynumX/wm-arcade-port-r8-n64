# Combat2DT — DRONE executable-body expansion + streamed visual safety

Combat2DR hardware showed two independent faults: the CPU advanced about one animation frame and then parked, while the second wrestler could become a corrupted CI8/TLUT silhouette. DS keeps DQ/DR source ownership and ATTR identity mapping unchanged.

## Streamed wrestler visual safety

Every DragonFS character-frame blob is now versioned with a `WMC1` header carrying the arcade wrestler id, dimensions, palette count, pixel byte count, and FNV-1a frame-name hash. The N64 loader validates that metadata and exact file length before returning a sprite, so stale or cross-wrestler payloads fail closed rather than being drawn with unrelated metadata.

RDPQ rendering is asynchronous. DS fences the RDP and waits for queued RSP/RDP work before an LRU character-cache slot is freed/reused. This prevents queued CI8/TLUT draws from reading memory that the CPU has already overwritten with another wrestler/frame.

## DRONE.ASM executable services

The conservative source-body translator now handles additional literal TMS34010 forms used by the recovered DRONE service windows: local A-register ADD/SUB, SLL/SRL, BTST, and the source-distinct `rnd` / `rndrng0` calls. Unsupported actor/world-memory operations still fail closed; DS does not invent them. The exact translated/required coverage is printed immediately after source-body generation in every build.
