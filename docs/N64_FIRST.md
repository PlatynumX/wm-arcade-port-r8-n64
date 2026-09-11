# N64-first constraints

- N64/libdragon is the primary playable target.
- Portable C is the source-of-truth for translated gameplay behavior.
- Design for the stock 4 MiB machine; Expansion Pak is optional headroom, not a dependency unless a later subsystem proves otherwise.
- Keep large wrestler/art assets streamable or regenerable rather than assuming everything should permanently occupy RAM.
- Use RDPQ for sprite composition and avoid CPU framebuffer blitting for normal gameplay.
- Keep the synchronous CI8 render path until hardware proves a faster queueing strategy is stable with multiple large layered wrestlers.
- Preserve original source frame names/labels in generated data so failures can be traced back to ASM and WIMP assets.

## Measured sprite budget (2026-09, re-measured)

The port now emits the full roster as DragonFS payload, so this is the
real number rather than an estimate of one: **5,126 distinct source
frames, 8 wrestlers, 35.7 MiB** of CI8 pixels and palettes. That is
wrestler art alone, before backgrounds, audio or code, and it needs a
64 Mbit-class cartridge. It is comfortable in ROM and impossible in
RAM, which was always the shape of the problem.

| wrestler | frames | palettes | payload |
|---|---|---|---|
| Bam Bam | 696 | 3 | 5.29 MiB |
| Yokozuna | 603 | 2 | 4.59 MiB |
| Undertaker | 608 | 7 | 4.57 MiB |
| Bret | 619 | 2 | 4.43 MiB |
| Shawn | 659 | 3 | 4.23 MiB |
| Razor | 647 | 2 | 4.23 MiB |
| Lex | 653 | 2 | 4.16 MiB |
| Doink | 641 | 2 | 4.15 MiB |

**Holding a whole wrestler resident is still out.** Two fighters is
8.3–9.9 MiB, over the stock 4 MiB machine and past an 8 MiB Expansion
Pak before a framebuffer or the code.

### The streaming unit, answered

The earlier version of this note ended by saying the unit had to be
finer than a wrestler — "per animation, or a frame cache with
eviction" — and left it open. It is a **frame cache with eviction**:
`src/platform/n64/streamed_character_art.c` keeps eight frames and
their pixels in RDRAM, LRU, and pulls the rest out of ROM through
DragonFS as the animation asks for them. Eight frames of two fighters
is tens of kilobytes, not megabytes.

Palettes are cached separately and never evicted, and that is a
correctness requirement rather than a cache-tuning choice: a cached
frame's sprite POINTS AT its palette, so dropping one out from under a
frame still in the cache would recolour it with another wrestler's
TLUT. There are 23 palettes across the whole roster, so 32 slots —
16 KiB — covers everything at once and the question cannot arise.

### 2.5 MiB that was not art

The first version of the payload wrote a full 512-byte 256-colour TLUT
into every frame file. The roster has **23 distinct palettes across
5,126 frames** — Bret has 619 frames and two palettes, the Undertaker
608 and seven — so that spent 2.5 MiB of cartridge writing 23 palettes
out 5,126 times. They are stored once each now, with a four-byte
header on each frame naming its own, which took the payload from
39,987,436 to 37,395,204 bytes for no loss of anything at all.

`src/generated/bret_sprites.c` remains the one in-ROM resident bank,
and is now the exception rather than the model: it exists because
Bret's hand-built `wm_visual_sequence` tables predate the streamed
path.
