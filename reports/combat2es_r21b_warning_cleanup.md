# Combat2ES R21B warning cleanup

Base: `fix39-v13e-combat2es-r21-remaining-smove-closure`

Removed two unused helper wait-step arrays left by the R21 monolithic closure pass:

- `smv_tow_tow_skick`
- `smv_down_tow_kick`

This is a warning-only cleanup. It does not change the active SMOVE manifest, gate kinds, labels, or translated fire bodies.
