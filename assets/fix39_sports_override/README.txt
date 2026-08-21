Fix39 V13e-c2c Midway Sports repurpose override

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
