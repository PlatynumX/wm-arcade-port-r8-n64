# Combat2CI source-vs-port combat audit

This revision is a corrective audit pass over the live combat runtime after Combat2CH.

Source findings applied:
- `wm_source_anim_services_t` existed but was never bound to either live animation runtime. This made ANI_CODE, RNDRNG0/RNDPER, DAMAGEOPP reaction services, change-other-animation, force-other-frame, and timing service calls silently inert.
- `WRESTLE.ASM` GETUP_TIME logic does not force a stand animation or directly change PLYRMODE to NORMAL. The animation script (`ANI_WAITROLL` / `ANI_GETUP_WAIT` / `ANI_CHANGEANIM`) owns recovery. The old forced-stand shortcut is removed.
- `ANIM.ASM` owns attack windows via `ANI_ATTACK_ON`, `ANI_ATTACK_ON_Z`, and `ANI_ATTACK_OFF`. The frame-name attack reconstruction in the live tick duplicated and could override those commands, so it is removed. WIMP frames remain authoritative for IANI3 hurt boxes.
- `change_anim1a` / `change_anim2a` immediately execute animation until the first sleep. Live source animation starts now tick immediately to match that behavior.
- `ANIM.ASM::_ani_zip` is a no-op/continue path, not ANI_END.
- `_ani_waitroll` resolves `I_WILL_DIE` and enters the shared dead animation instead of holding forever.
- High-frequency stateful ANI_CODE callbacks used in shipped wrestler recovery/combat sequences now have source-translated live handlers (`am_I_dead`, `ckzpos`, `no_bk_xvel`, `choose_2or4`, `make_norm`) and source event dispatch is live for the remaining native routine boundary.

The earlier exact DRONE.ASM seek patch, execute_walk/set_velocities patch, wrestler_main ordering patch, full streamed ANIM.ASM VM, ring/arena/camera work remain in the build pipeline.
