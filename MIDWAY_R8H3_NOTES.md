# r8h3 Midway Sports logo test

This revision targets the corrupted Midway Sports logo seen in the r8h2 frontend testing.

- Source artwork: `IMG/SPORTLO8.IMG`
- Required source objects: `SPRTLG01` through `SPRTLG17`, in original `ATTRACT.ASM::LOGO_LIST` order
- Common arcade object anchor: `(200,118)`
- Per-piece placement: original WIMP `xani/yani` hotspots, never hand-authored offsets
- N64 transform: 400x256 source frontend coordinates to 320x240 using RDPQ scale around each preserved hotspot
- Portable state timing: 32-tick lead-in, 0.5 s before button skip, 8 s wait window at 60 Hz

Expected hardware test for this revision: the 17-piece Midway Sports logo should appear assembled correctly on a black field. `SPORTBKBMOD`, the original slogan font, and the preceding DCS logo are intentionally not replaced with approximations in this isolated logo test; they remain separate exact-source frontend translation steps.
