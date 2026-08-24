Fix39 V13e-c2e Midway Sports repurpose override

Authoritative input: WrestleMania_Midway_Logo_Replacements_BUILD_READY.zip

Runtime-consumed source overrides:
- SPORTLO8.IMG -> the 17 live SPRTLG01..SPRTLG17 foreground objects
- SPORTBK.IMG  -> the background WIMP source including the MIDWAY tile

Round-trip companion files retained for source fidelity / future tooling:
- SPRTBK.BDD
- SPRTBK.BDB

No attract timing, movement, object placement, or screen ordering is changed.
The existing source asset converters consume these files in place of the
historical originals when this override directory is present.

C2e hardware note:
- The MIDWAY object already contains the PLATYNUMX replacement pixels and its
  29-color WIMP palette.
- The stock N64 sports background converter substitutes the historical BGNDPAL
  palette, which made the replacement logo nearly black on hardware.
- C2e routes this override only through fix39_sports_background_bundle.py so the
  exact round-tripped WIMP palette is preserved.
