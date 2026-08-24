# Combat2DW — WIMP CI8 palette-bank reconstruction

Combat2DV proved that some original wrestler WIMP frames use CI8 pixel indices beyond the `color_count` of the single palette-directory record selected by the image. Bam Bam supplies the hard case: source frames use indices through 255 while the selected directory record is only 64 colors.

The previous converter treated a WIMP palette-directory record as a complete per-frame TLUT. DW separates the image's palette-directory selection from the CI8-visible palette bank. It keeps the established raw-id/minimum-id directory mapping and reconstructs the effective bank only from real consecutive source palette records, capped at the CI8 limit of 256 colors.

No colors are synthesized, padded, wrapped, or clamped. If the source directory does not contain enough real entries for a frame's highest CI8 index, generation still fails closed. The same reconstructed bank is emitted to host sprite data and DragonFS WMC1 blobs, and `pal_colors` now records the reconstructed bank size so the N64 loader validates the same ABI.

This matches the arcade display model exposed by the original source: image DMA selects a palette separately while the image payload is 8-bit indexed. A 64-color directory fragment therefore cannot itself be the complete address space for a source frame that legitimately contains index 255.
