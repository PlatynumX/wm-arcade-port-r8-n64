# Animation translation frontier

Generated mechanically from the historical ASM tree. WORD/LONG packing is preserved; symbolic pointers and unknown source forms are not guessed.

- Source routines scanned: **5109**
- Routines containing typed data: **3548**
- Animation-like typed routines: **2767**
- WORD items preserved: **156090**
- LONG items preserved: **78102**
- Unresolved non-data forms inside typed routines: **5844**
- Executable lines following typed data: **58426**

## Most common `ANI_*` source tokens

- `ANI_SETMODE`: 2387
- `ANI_SETSPEED`: 2059
- `ANI_ZEROVELS`: 1784
- `ANI_END`: 1678
- `ANI_SETPLYRMODE`: 1143
- `ANI_CODE`: 1117
- `ANI_WAITHITGND`: 962
- `ANI_SET_YVEL`: 943
- `ANI_ATTACK_OFF`: 730
- `ANI_SETFACING`: 670
- `ANI_CHANGEANIM`: 656
- `ANI_ATTACK_ON`: 651
- `ANI_STARTATTACK`: 616
- `ANI_OFFSET`: 581
- `ANI_FACEDOWN`: 532
- `ANI_IFNOTSTATUS`: 528
- `ANI_SET_XVEL`: 472
- `ANI_XFLIP`: 445
- `ANI_SET_WRESTLER_XFLIP`: 444
- `ANI_GOTO`: 443
- `ANI_SET_ZVEL`: 375
- `ANI_SHAKER`: 373
- `ANI_REPEAT`: 349
- `ANI_SHAKEALL`: 325
- `ANI_DEBRIS`: 312
- `ANI_DETACH`: 301
- `ANI_ZERO_XZVELS`: 295
- `ANI_WAITROLL`: 286
- `ANI_ADD_MOVE`: 267
- `ANI_FRICTION`: 260
- `ANI_IFSTATUS`: 221
- `ANI_IFBLOCKED`: 209
- `ANI_FACEUP`: 196
- `ANI_WAITHITOPP`: 194
- `ANI_ATTACHZ`: 188
- `ANI_IFBUTTONS`: 177
- `ANI_SUPERSLAVE2`: 173
- `ANI_SLAVEANIM`: 171
- `ANI_SOUND`: 158
- `ANI_CREATEPROC`: 154
- `ANI_CLR_BUTCOUNT`: 144
- `ANI_SET_RPTCOUNT`: 125
- `ANI_ATTACK_ON_Z`: 123
- `ANI_DEC_RPTCOUNT`: 122
- `ANI_IF_BUTCOUNT_LT`: 122
- `ANI_DAMAGEOPP`: 110
- `ANI_BOUNCE`: 109
- `ANI_CLEAR_COMBO`: 105
- `ANI_IF_RPTCOUNT`: 104
- `ANI_INC_COMBO`: 103

## Policy

Anything not represented as an explicit WORD/LONG item remains on the translation frontier. The runtime must implement the original command semantics before such routines are executable.
