# R37N14 — COLLIS.ASM readiness retranslation

Base: R37N13 commit `7f6d7986aa71bb212d2e57516f5a9f35870b9654`.

## Hardware evidence
R37N13 no longer immediately deadlocks: wrestlers move and fight, but ordinary hits do not register and grapple/headhold acquisition is not observed.

## Source finding
`COLLIS.ASM::check_collisions` does **not** require every wrestler to have a valid collision box before any combat can run. For each offensive process with `MODE_CHECKHIT`, it calls `set_xyz`, which builds the attack box from the attacker's `OBJ_ATT*` fields. It then tests each defensive process's already-populated `OBJ_COLL*` box independently.

The R37N13 bridge instead wrapped the entire wrestler/special collision pass in `if (live_collision_boxes_ready_for_active())`. A single unavailable portable frame-geometry snapshot therefore disabled combat for everybody on that tick. This is stronger than Midway's contract and can suppress both strike contact and `ANI_WAITHITOPP`/STATUS-driven grapple acquisition.

## Translation change
- Preserve `collision_boxes_ready` only as an all-active diagnostic.
- Do not use it as a combat gate.
- Build the attack box for every active `CHECKHIT` attacker regardless of whether that attacker has a current hurt-box snapshot.
- Consider each defender only when that defender has current translated `OBJ_COLL*` geometry.
- Preserve Midway's even-tick forward / odd-tick reverse attacker walk.
- Preserve forward defender walk and stop after the first successful wrestler hit.
- Run SPECIAL object collision first against the same individually valid defender surface.
- Do not touch R37N13 wrestler scheduler/process ordering.

## Acceptance
1. CI must remain green.
2. Hardware attract/demo combat should begin registering visible hit reactions/damage again.
3. Head-grab attempts should be able to set STATUS/clear WAITHITOPP and proceed into the existing attachment animation path.
4. If either still fails, instrument the now-isolated `CHECKHIT -> attack box -> defender hurt box -> acceptance reason` path; do not rewrite scheduler.
