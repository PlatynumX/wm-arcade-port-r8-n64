# N64-first constraints

- N64/libdragon is the primary playable target.
- Portable C is the source-of-truth for translated gameplay behavior.
- Design for the stock 4 MiB machine; Expansion Pak is optional headroom, not a dependency unless a later subsystem proves otherwise.
- Keep large wrestler/art assets streamable or regenerable rather than assuming everything should permanently occupy RAM.
- Use RDPQ for sprite composition and avoid CPU framebuffer blitting for normal gameplay.
- Keep the synchronous CI8 render path until hardware proves a faster queueing strategy is stable with multiple large layered wrestlers.
- Preserve original source frame names/labels in generated data so failures can be traced back to ASM and WIMP assets.
