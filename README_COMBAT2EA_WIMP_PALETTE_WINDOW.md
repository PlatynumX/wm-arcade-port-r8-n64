# Combat2EA WIMP palette-window reconstruction

The original LOAD2 pipeline stores image and PAL pointers separately. Previous conversion treated WIMP `color_count` as a dense palette from CI8 index 0, which is contradicted by Bam Bam frames whose CI8 indices exceed a 64-color record. EA treats the WIMP palette record as a source palette window. It examines only source metadata words contained in the palette record and chooses a base index only when exactly one encoded base value makes all nonzero texels legal. It preserves texel indices exactly and fails closed on ambiguity.
