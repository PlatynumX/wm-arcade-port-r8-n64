# Combat2ES R18B Razor slasher/rug compile fix

Base: `fix39-v13e-combat2es-r17b-bret-smove-finish-compile-fix`

R18B fixes the R18 insertion-order compile bug: R18 appended the Razor
input array and body functions after declarations that required them to
be inserted before. This pass applies the same intended Razor source
translation cleanly.

Translated active Razor SMOVE monitor bodies:

- `rzr_charge_slashes`
- `rzr_sliding_rug`

Expected strict movement: Razor unresolved `8 -> 6`.
