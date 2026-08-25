# Combat2ES R20 - Yoko finish pass

Base branch: `fix39-v13e-combat2es-r19-razor-finish-pass`

Translated active YOKO.ASM SMOVE monitor bodies:

- `yok_hdhold_combo1`
- `yok_hdhold_scissor`
- `yok_hdhold_suplex`
- `yok_salt_throw`
- `yok_grab_toss_air`
- `yok_hdhold_combo2`

Source-shape notes:

- `yok_hdhold_suplex` and `yok_hdhold_scissor` preserve the shared HEADHOLD/HEADHELD
  reversal/BONUS_MESS/SMRTTGT/immobilize/FIND_AND_KILL_ENDLESS/SPECIAL_MOVE_ADDR flow.
- `yok_hdhold_combo1` and `yok_hdhold_combo2` require HEADHOLD, CHECK_COMBO_GO,
  target WHOIHIT, call FIND_AND_KILL_ENDLESS, and queue the combo animation.
- `yok_salt_throw` uses the source down/toward/punch input chain, calls
  FIND_AND_KILL_ENDLESS, queues `yok_4_salt_anim`, and keeps the 120-tick
  post-fire sleep.
- `yok_grab_toss_air` preserves the in-air / AT_LEAPING hiptoss2 split and
  the `0x6c` closest-distance gate for normal hiptoss.
