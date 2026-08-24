# Combat source-parity audit — 2026-08-22

This checkpoint changes the standard for Fix39 combat work: original Midway arcade source is authoritative. A host test is not evidence of correctness merely because it agrees with the current N64 reimplementation.

## Confirmed source facts

- `TABLES.ASM` WRESTLERNUM IDs are Bret=0, Razor=1, Undertaker=2, Yokozuna=3, Shawn=4, Bam Bam=5, Doink=6, Adam Bomb=7 (unfinished slot), Lex=8.
- `GAME.EQU` uses the source 8-way direction bit values (`UP=1`, `DOWN=2`, `LEFT=4`, `RIGHT=8`; diagonals are ORs). The translated match reset faces P1/P2 toward one another with source-facing values 9 and 6.
- `COLLIS.ASM` normal/ground/running hurt-box Z offsets/depths are -30/60, -15/30, and -5/10. Attack X mirroring is driven by object horizontal flip state. Source overlap rejects only strict separation, so touching edges count as overlap.
- `REACT1.ASM:wrestler_hit` handles `AMODE_RUN` through `good_run_hit` before committing WHOIHIT/WHOHITME.
- `ATTR.ASM:show_gameplay` chooses a source wrestler id using `RNDRNG0(7)`, remaps result 7 to 8, sets ladder/match state, and creates `start_match`. It does not simply reuse frontend selection-slot ids.

## Major parity gap found

The current attract integration still advances `wm_demo_tick()` as a second gameplay harness and then calls `wm_fix39_match_sync_presenter_pose()` with `demo.p*.screen_x/screen_y`. Those values are presentation-space coordinates, while translated Fix39 actors begin in source world X/Z around the ring's ~1000-range coordinate system.

`wm_fix39_match_tick()` runs translated DRONE/movement and then, when presenter pose is valid, writes the presenter coordinates back into `actor.x_int/z_int` immediately before collision. This means the translated match runtime does **not** currently own position at the collision boundary, despite comments saying it does.

That seam makes collision/facing/contact results untrustworthy until presentation and source-world coordinates are separated correctly. Do not paper over it by changing tests to match current behavior.

## Rule going forward

When a host test and the reimplementation disagree, inspect the original arcade source first. Only change the test when the original source supports the new expectation. Keep presentation adaptation downstream of source-owned match state; never feed screen-space positions back into source-world combat state without a documented source-backed transform.

## Primary source files

- https://github.com/historicalsource/wwf-wrestlemania/blob/main/ATTR.ASM
- https://github.com/historicalsource/wwf-wrestlemania/blob/main/COLLIS.ASM
- https://github.com/historicalsource/wwf-wrestlemania/blob/main/TABLES.ASM
- https://github.com/historicalsource/wwf-wrestlemania/blob/main/GAME.EQU
- https://github.com/historicalsource/wwf-wrestlemania/blob/main/ANIM.EQU
- https://github.com/historicalsource/wwf-wrestlemania/blob/main/REACT1.ASM
- https://github.com/historicalsource/wwf-wrestlemania/blob/main/WRESTLE.ASM
