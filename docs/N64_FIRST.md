# N64-first constraints

- N64/libdragon is the primary playable target.
- Portable C is the source-of-truth for translated gameplay behavior.
- Design for the stock 4 MiB machine; Expansion Pak is optional headroom, not a dependency unless a later subsystem proves otherwise.
- Keep large wrestler/art assets streamable or regenerable rather than assuming everything should permanently occupy RAM.
- Use RDPQ for sprite composition and avoid CPU framebuffer blitting for normal gameplay.
- Keep the synchronous CI8 render path until hardware proves a faster queueing strategy is stable with multiple large layered wrestlers.
- Preserve original source frame names/labels in generated data so failures can be traced back to ASM and WIMP assets.

## Measured sprite budget (2026-09)

The animation programs the port emits name **4,474 distinct source
frames**, and the CI8 pixels behind them come to **30.3 MiB** — far more
than any N64 RAM configuration, though comfortable in a cart ROM.

Per wrestler, from the frames each `.LOD` owns:

| wrestler | frames | CI8 |
|---|---|---|
| Bam Bam | 601 | 4.48 MiB |
| Undertaker | 523 | 3.89 MiB |
| Yokozuna | 510 | 3.80 MiB |
| Bret | 547 | 3.72 MiB |
| Doink | 580 | 3.71 MiB |
| Razor | 574 | 3.68 MiB |
| Shawn | 574 | 3.53 MiB |
| Lex | 565 | 3.49 MiB |

**This rules out holding a whole wrestler resident.** Two fighters is
7.2–9 MiB, over the 4 MiB stock machine and at or past an 8 MiB
Expansion Pak before a framebuffer, the code or anything else. Loading
per wrestler — the obvious reading of "keep large wrestler/art assets
streamable" above — is still too coarse a granularity.

So the streaming unit has to be finer than a wrestler: per animation,
or a frame cache with eviction, sized against what a single exchange can
reach. Establishing that bound is its own measurement — the corpus test
(tests/arcade_port/anim/test_corpus.c) already plays every program and
could report the distinct frames each one touches, which is the natural
place to get it.

`src/generated/bret_sprites.c` is the one bank that exists, and it is
1.72 MiB rather than 3.72 because it is bundled from Bret's hand-built
`wm_visual_sequence` tables (303 frames) rather than from the animation
programs (547). tools/bret_bundle.py now takes `--prefix` and
`--only-mapped` so it can bundle any wrestler from that wrestler's own
`.LOD`; what it cannot yet do is decide what a runtime should hold.
