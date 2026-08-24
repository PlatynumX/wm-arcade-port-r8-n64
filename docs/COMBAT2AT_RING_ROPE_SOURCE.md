Combat2AT ring/rope source translation stage

- Base: hardware-good Combat2AQ.
- Retains AQ DragonFS character cache coherency fix.
- Activates the already-translated ROPES.ASM image-symbol adapter in wm_fix39_runtime.
- Converts every physical image directory entry from original the shipped live rope containers ROPESTUF.IMG and ROPESHAD.IMG (selected by MISC.LOD / MAIN.LOD; SIDEROPE.IMG is explicitly deleted by the source tree DELETE.BAT) into DragonFS CI8 + RGBA5551 TLUT payloads.
- Duplicate side/shadow WIMP directory names are preserved as the source a/b object halves used by ROPES.ASM.
- Does NOT substitute rectangles or invented rope art.
- This stage intentionally does not claim the arena/ring renderer is complete: the original object projection/depth path must consume these assets before placeholder draw_ring_* can be removed source-faithfully.
