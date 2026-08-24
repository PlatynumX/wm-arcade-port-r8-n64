# Combat2EC — WIMP image-level CI8 index bias

Combat2EA reached the real Bam Bam source data and showed two different 64-colour image-index conventions in the same source corpus: ordinary `0..63` frames and frames whose actual source CI8 bytes occupy `1..64` with index 0 absent.

Combat2EC handles the latter **per image**, not per wrestler or per container. A one-based view is accepted only when the source byte range itself proves the exact `1..N` shape for an N-colour record. Those bytes are normalized to `0..N-1` while preserving the source palette ordering. Dense `0..N-1` frames stay byte-identical. Any wider/unexplained range—including index-255 cases—continues to fail closed.

The same normalized CI8 bytes are used by host verification and DragonFS packaging so both runtimes see the same frame/palette pair.
