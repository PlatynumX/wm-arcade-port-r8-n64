# Combat2EK — WRESTLE.ASM parity phase 1

Baseline: Combat2EH `40cc4dd45409b8e26fc8ff7340d1699c62a914a5`.

Combat2EK deliberately does not include the unfinished Combat2EI 17-piece
Sports-logo replacement. The Sports screen remains at the exact Combat2EH
asset state.

## Ported in this phase

### 1. All-eight ANI_INIT animation dispatch

`WRESTLE.ASM::ani_init` dispatches through `#init_addr` to:

- `bret_ani_init`
- `razor_ani_init`
- `taker_ani_init`
- `yoko_ani_init`
- `shawn_ani_init`
- `bam_ani_init`
- `doink_ani_init`
- `lex_ani_init`

Combat2EH only called the dedicated Bret and Razor routines, then relied on a
primary-animation fallback for the other six. Combat2EK adds the missing six
entry points to their dedicated wrestler modules and dispatches all eight.

The source test is `FACING_DIR & PLAYER_RIGHT_BIT`: right-facing wrestlers
start `*_stand2_anim` + `*_torso2_anim`; the other branch starts
`*_stand4_anim` + `*_torso4_anim`. The old fallback incorrectly tested the UP
bit; that is corrected.

The common WRESTLE `ani_init` state is also applied before dispatch:
`ANI_SPEED=0x100`, `I_WILL_DIE=0`, `ATTIMG_CUR_FRAME=0`, and current
`hyper_speed_on` behavior (`0` in this runtime) into `WALK_FAST`.

The source-created `TAUNT_PID/do_taunt` process on the player-2 branch is not
claimed complete in this phase. It belongs with the upcoming process/smove
scheduler work.

### 2. Direct `calc_closest` / `calc_closest2`

The previous `refresh_distances()` used a Chebyshev approximation:
`max(abs(dx), abs(dy), abs(dz))` and hardwired the only other actor as target.
That is removed.

Combat2EK adds source-owned `CLOSEST_NUM` state and a direct target service
that preserves:

- inactive/self/friendly rejection;
- source two-stage integer square-root distance;
- live > dead > zombie target priority;
- running ahead/behind target filter;
- ONGROUND distance penalty;
- WHOIHIT target bonus;
- INRING mismatch penalty;
- previous-closest bonus;
- combo target lock;
- Z-distance weighting;
- exact X/Y/Z component distances;
- `calc_closest2` forced rescan when current target is dead;
- `calc_closest2` four-PCNT-phase scheduling.

The accepted `CLOSEST_NUM` is bridged to `smart_target` so existing portable
character and DRONE modules consume the source-selected target rather than an
invented target.

### 3. Correct live placement of closest calculation

The duplicate Combat2EH pre-DRONE `refresh_distances()` calls are removed.
`update_newfacing` now consumes the target selected by the previous source
closest pass, and `calc_closest2` runs after the second confinement/animation
phase immediately before `move_wrestler`, matching `wrestler_main` ordering.

### 4. EJ grounded confinement retained

Combat2EK also carries the source-safe grounded/in-ring confinement slice from
Combat2EJ so the hardware-demo walk-through-the-rope regression is not
reintroduced while the complete `confine_wrestler` port is being built.

## Deliberately still pending

The next WRESTLE phases remain:

1. complete `confine_wrestler`, `fix1`, `fix2`, and `final_confine` including
   climb, attached-pair, outside-arena, gate-crash, bounce and zombie paths;
2. finish exact `wrestler_main` ordering around collision/attachment/xflip;
3. `wrest_joystat`, `update_joystat`, `update_joy_dtime`, held buttons, RUN and
   charge/release state;
4. `init_smoves`, secret-move watchdog processes and TAUNT process scheduling;
5. `auto_pin_check` and exact `move_wrestler` WRESTLE layer;
6. match timer / rounds / winner / timeout / reset lifecycle;
7. expand process topology beyond the temporary two-actor runtime;
8. classify and port every reachable unresolved native `ANI_CODE` service.

No behavior in those pending areas is marked source-complete by Combat2EK.
