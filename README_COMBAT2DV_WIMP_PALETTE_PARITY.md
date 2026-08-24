# Combat2DV — WIMP palette mapping parity

Combat2DU correctly stopped on `bam:B4FK4F10`: CI8 index 64 was paired with a 64-entry palette. DV does not clamp the pixel or pad the palette. Instead it removes the old assumption that WIMP raw palette IDs are always contiguous.

The character source converter now derives the palette-ID convention from each original `.IMG` container. It tests the source-observed candidate index conventions against every image in that container and accepts a mapping only when every CI8 index is legal for the mapped source palette. If multiple different mappings remain source-valid, conversion fails closed as ambiguous.

This keeps frame pixels and palette data entirely source-derived and is specifically designed to recover sparse palette-ID containers such as the Bam Bam frame that stopped DU. A regression reproduces the `max index 64 / palette 64` class and proves that no padding/clamping fallback exists.
