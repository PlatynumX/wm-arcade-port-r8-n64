# Combat2ES R21 - remaining active SMOVE closure

Base branch: `fix39-v13e-combat2es-r20-yoko-finish-pass`

This monolithic pass covers the remaining active strict-report SMOVE bodies for:

- Shawn
- Bam Bam
- Doink
- Lex

The pass intentionally does **not** enable GAME.EQU-disabled finishers.

Translated strict-report labels:

## SHAWN

- `shn_hdhold_combo2`
- `shn_hdhold_combo1`
- `shn_charge_suplex`
- `shn_swirl_speedkick`
- `shn_sliding_kicktoss`
- `shn_hdhold_suplex`
- `shn_hdhold_frank`
- `shn_hdhold_kicktoss`
- `shn_hdhold_butts`
- `shn_flipslam`
- `shn_grab_toss_air`

## BAM

- `bam_charge_neckbreaker`
- `bam_hdhold_combo1`
- `bam_hdhold_pile`
- `bam_hdhold_pogo`
- `bam_hdhold_combo2`
- `bam_grab_toss_air`

## DOINK

- `dnk_charge_flykick`
- `dnk_hdhold_slam`
- `dnk_hdhold_combo1`
- `dnk_hdhold_pile`
- `dnk_hdhold_combo2`
- `dnk_hdhold_buzz`
- `dnk_grab_toss_air`

## LEX

- `lex_hdhold_pile`
- `lex_hdhold_elbow_face`
- `lex_hdhold_graboh`
- `lex_grab_toss_air`
- `lex_hdhold_combo1`
- `lex_hdhold_combo2`

Source-shape notes:

- Headhold throws preserve the established source flow used by Bret/Razor/Yoko:
  HEADHOLD/HEADHELD gate, reversal branch, BONUS_MESS branch, SMRTTGT,
  target immobilization, FIND_AND_KILL_ENDLESS, and SPECIAL_MOVE_ADDR queue.
- Combo bodies preserve HEADHOLD + CHECK_COMBO_GO + WHOIHIT targeting.
- Grab-toss-air bodies preserve the in-air / AT_LEAPING split and closest-distance
  gate before queueing hiptoss / hiptoss2 labels.
- Charge bodies preserve hold/release counting, minimum charge, bad-mode rejection,
  ck_ignore, and SPECIAL_MOVE_ADDR queueing.
