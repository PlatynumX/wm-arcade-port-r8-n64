# N64-first constraints

- N64/libdragon is the primary playable target.
- Portable C is the source-of-truth for translated gameplay behavior.
- Design for the stock 4 MiB machine; Expansion Pak is optional headroom, not a dependency unless a later subsystem proves otherwise.
- Keep large wrestler/art assets streamable or regenerable rather than assuming everything should permanently occupy RAM.
- Use RDPQ for sprite composition and avoid CPU framebuffer blitting for normal gameplay.
- Keep the synchronous CI8 render path until hardware proves a faster queueing strategy is stable with multiple large layered wrestlers.
- Preserve original source frame names/labels in generated data so failures can be traced back to ASM and WIMP assets.

## Measured cartridge budget (2026-09)

The art had been measured; the code never had. `src/generated` is
200,000-odd lines of tables and nobody knew what they weighed, so
`tools/n64_rom_budget.py` now measures it -- it compiles the ROM's own
source list for mips64 (the same list `n64_link_check.py` links), sums
the sections that occupy cartridge space, adds the DragonFS payload and
compares the total with the budget. It runs as the `wm_n64_budget`
ctest and fails past 85%, so the first warning is not also the
emergency.

| | bytes | |
|---|---|---|
| code (executable) | 327,496 | 0.31 MiB |
| read-only data | 6,044,076 | 5.76 MiB |
| relocated read-only data | 2,921,800 | 2.79 MiB |
| writable data | 54,848 | 0.05 MiB |
| **code subtotal** | **9,348,220** | **8.92 MiB** |
| DragonFS payload (5,150 files) | 37,395,966 | 35.66 MiB |
| **total** | **46,744,186** | **44.58 MiB** |
| budget | 78,000,000 | 74.39 MiB |

**59.9% of the cartridge, with 31 MB spare.** The biggest single
contributors are all generated: `anim_programs.c` 2.46 MiB,
`progress_wrestlers.c` 2.42, `bret_sprites.c` 1.75,
`select_sprites.c` 0.69.

`.bss` is **698,480 bytes (0.67 MiB)**, and that is the number worth
watching rather than the cartridge one: it is RAM on a machine with 4
MiB of it, alongside framebuffers and the frame cache.

### Why the first measurement was 2.8 MiB light

The tool's first version matched section *names* -- `.text`, `.rodata*`,
`.data` -- and reported 6.11 MiB. It was wrong, and wrong about the
single biggest thing in the ROM. `anim_programs.c` is 2.4 MiB of
pointer tables, and GCC puts those in `.data.rel.ro.local`: read-only
once relocated, so not `.rodata` by name, and write-flagged, so not
matched as `.data` either. It is as cartridge-resident as anything
else. The tool classifies by ELF flags now -- ALLOC with contents is
cartridge, ALLOC and NOBITS is `.bss`, anything else is link metadata
-- because a list of names goes stale the moment the compiler invents
a section and a flag test does not.

Two smaller corrections from the same pass. The payload is **35.66
MiB, not the 46 MB an earlier note gave**: that came from `du`, which
block-rounds 5,150 small files and charges a megabyte and a half of
tail padding DragonFS does not pay. And the total is an estimate in
both directions, not a floor: mergeable string sections are summed per
object and the linker deduplicates them, and `--gc-sections` discards
what nothing references, while the figure omits libdragon itself, the
DFS header and per-file overhead, and padding to a power of two.

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
