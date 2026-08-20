# Source Audit — Combat Stage 12 (attachment / puppet animation integration)

Primary arcade sources:

- `https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/ANIM.ASM`
- `https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/WRESTLE.ASM`

Stage 12 closes the shared attachment state needed by the puppet/grapple reactions
ported earlier.  It is intentionally an adapter layer, not a replacement for the
N64 port's process or animation structures.

## `wres_slave_anim`

`wm_arcade_anim_enter_slave_idle()` reproduces the semantic result of:

    ANI_SETMODE MODE_UNINT|MODE_NOAUTOFLIP|MODE_NOGRAVITY
    ANI_ZEROVELS
    ANI_SETSPEED 100h
    ANI_END

The adapter also applies the source `ANI_SETMODE` side effects: clear
`SF_CLEAR_BITS`, and change nonzero PTIME to 1.

## Animation attachment commands

Ported semantic handlers:

- command 10 `_ani_detach`
- command 80 `_ani_setoppmode`
- command 81 `_ani_clroppmode`
- command 105 `_ani_setopp_plyrmode`
- command 106 `_ani_xflip_opp`
- command 110 `_ani_setoppvels`
- command 113 `_ani_set_attach`

Fidelity notes:

- detach clears the caller link first; it clears the other link only when the links
  match.  PUPPET/PUPPET2/ATTACHED victims are then forced ONGROUND. HEADHELD is
  deliberately excluded because the source disabled that check for Shawn's
  Frankensteiner path.
- commands 80/81 only require the two `ATTACH_PROC` fields to be non-null; they do
  **not** compare the reciprocal pointer for equality.  The port preserves that.
- commands 105/106 require reciprocal attachment equality.
- command 105 refuses to overwrite `MODE_DEAD`.
- command 110 uses the reciprocal attached wrestler when valid, otherwise falls
  back to WHOIHIT. X/Z velocity signs are relative to the attacker's facing bits.
- command 113 creates the reciprocal pair from WHOIHIT.

## Per-tick attachment positioning

`wm_arcade_master_keep_attached()` ports the WRESTLE.ASM master path:

- requires both attachment fields to be non-null (source does not compare equality);
- if the attached victim is grounded and not GHOST, clamps the master's Y to the
  attachment-implied floor;
- zeroes victim Y velocity;
- positions victim from the master's 16.16 X/Y/Z plus ATTACH offsets;
- X offset sign is selected by master facing.

`wm_arcade_keep_attached()` ports the inverse slave path using the master's offsets
and facing direction.
