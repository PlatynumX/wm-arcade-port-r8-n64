# WWF WrestleMania Arcade Combat Port — Stage 2 Source Audit

This stage continues directly from Stage 1.  No gameplay behavior here is an
approximation.  Renderer/N64 integration points that are not yet translated are
callbacks, not substitutes.

## Directly translated in Stage 2

### REACT1.ASM — `wrestler_hit`

- `AMODE_RUN` is gated by `good_run_hit` *before* WHOIHIT/WHOHITME changes.
- WHOIHIT / WHOHITME assignment.
- `LAST_HIT_TIME` uses full 32-bit `PCNT`.
- `damage_values[56]`, including full/reduced damage pairs.
- Normal repeat-damage window: reduced damage when victim `LAST_DAMAGE != 0`
  and low-word `PCNT - LAST_DAMAGE <= 50`.
- `NEXT_DAMAGE` normal-hit rules exactly preserved:
  - zero base damage does not inspect or consume it;
  - expired values are cleared;
  - a value larger than ordinary damage is not used but is cleared;
  - otherwise it replaces damage and is cleared.
- Source offense/defense tables (all offense +35% / all defense +0%).
- 8.8 x 8.8 multiplication order and final negation.
- Blocking changes nonzero damage to -1 except `AMODE_BSTOMP` and
  `AMODE_BLBOWDROP`.
- `AMODE_BSTOMP2` is deliberately *not* a block exception; the source check is
  commented out.
- RISK clearing / high-risk `DAM_MULT=4` / `any_hits` behavior.
- Turnbuckle-reaction override, excluding puppet/puppet2/puppet-hdgrab/hitcheck.
- Exact 56-entry hit/reaction dispatch mapping.
- Reaction happens before health and can rewrite `hit_damage_pending` and
  `new_victim_movedir` through the callback.
- First unblocked >=2-damage hit sets `DAM_MULT=2`, `any_hits=1`, and invokes
  award/message hooks.
- `adjust_health` is called only when post-reaction pending damage is nonzero.

### REACT1.ASM — safe, unambiguous part of `hit_stuff`

- HITCHECK skips all cleanup.
- KOD -> NO_KO, clears KOD, PTIME=1.
- stars/debris/combo/shadow-trail/attack-image cleanup and palette restore.
- reciprocal attachment cleanup and partner breakout classification.
- SMART_TARGET / SMART_ATTACK cleanup.
- running/bouncing getup-meter hook.
- RUN_TIME clear.

The final REACT1 attachment tail (source lines 809-817) is *not* guessed.  It
branches as if a nonzero pointer should skip the block, but the fall-through
comment says "attached" and then dereferences that pointer after prior code has
already cleared it.  It is explicitly deferred to the attachment/grapple audit.

### ANIM.ASM combat opcodes

- opcode 6 `ANI_ATTACK_ON`
- opcode 7 `ANI_ATTACK_OFF`
- opcode 14 `ANI_ATTACK_ON_Z`
- opcode 47 `ANI_CLR_DAMAGE` (source is currently a no-op)
- opcode 56 `ANI_DAMAGE`
- opcode 58 `ANI_CLR_STATUS`
- opcode 66 `ANI_DAMAGEOPP`
- opcode 68 `ANI_WAITHITOPP`

`ANI_DAMAGEOPP` remains a separate path because the arcade does not use normal
`wrestler_hit` rules:

- repeat window is 30 ticks, not 50;
- no offense/defense modifier step;
- no one-damage block reduction;
- live `NEXT_DAMAGE` overrides unconditionally and is not cleared here;
- it invokes `adjust_health` even when damage is zero.

## Explicit merge hooks, not approximations

- individual hit reaction routines from REACT1..REACT9;
- `adjust_health` from ADJUST.ASM;
- `good_run_hit`;
- award/message process creation;
- renderer-specific `DMAWNZ` control reset;
- `xxx_goto_stand_anim` and `xxx_aborted_attach_anim`;
- `ditch_getup_meter`.

## Next combat chunk

Port the actual reaction handlers (`hit_punch`, `hit_kick`, block reactions,
stomps, clothesline, run, puppet reactions, specials, etc.) from
REACT1.ASM..REACT9.ASM.  That removes the largest callback boundary currently
between accepted collision and visible combat response.  After that, port the
attachment/grapple/puppet state machine as one coherent subsystem.
