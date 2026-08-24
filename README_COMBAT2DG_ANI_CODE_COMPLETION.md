# Combat2DH — ANI_CODE native routine completion

Combat2DH continues from Combat2DF and targets the live source-animation VM's native `ANI_CODE` callback gap observed in attract combat. The previous VM preserved unknown callback labels for diagnostics, but many state-changing routines were therefore inert.

This pass directly translates and binds 26 additional native routines from the supplied arcade ASM payload: `get_leap`, `adjust_facing`, `adjust_taker_facing`, `clear_link`, `inc_loop`, `face_inside`, `tbukl_flip`, `set_xdrift`, `hit_nearest`, `set_tbukl_airmode`, `set_my_pal`, `set_pinable_bit`, `set_opp_xy`, `guy_is_up`, `guy_is_in`, `is_guy_up`, `is_he_in`, `stand_wrestler`, and `dizzy_wrestler`, `attach_victim`, `x_flip`, `setup_run`, `dead_or_dying`, `get_xvel`, `choose_dir`, and `ck_flip`.

It also corrects the earlier partial `check_xvel` translation to follow the source's opponent-in-ring and ring-side checks rather than using facing direction as a substitute.

No fallback invents behavior for native labels whose implementation is not present in the supplied source payload. Those remain explicit diagnostics instead of fabricated gameplay.
