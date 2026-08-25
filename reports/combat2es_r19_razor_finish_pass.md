# Combat2ES R19 Razor finish pass

Base: fix39-v13e-combat2es-r18b-razor-slasher-rug-compile-fix

Translated remaining active RAZOR.ASM SMOVE bodies:

- rzr_hdhold_pile
- rzr_hdhold_combo1
- rzr_hdhold_edge
- rzr_hdhold_rug
- rzr_grab_toss_air
- rzr_hdhold_combo2

Notes:
- GAME.EQU-disabled Razor finishers remain disabled.
- RAZOR.ASM headhold throws preserve reversal/BONUS_MESS/SMRTTGT/immobilize/FIND_AND_KILL_ENDLESS flow.
- RAZOR.ASM grab-toss-air preserves in-air/leaping vs distance split and FACE24-style 2/4 animation selection.
