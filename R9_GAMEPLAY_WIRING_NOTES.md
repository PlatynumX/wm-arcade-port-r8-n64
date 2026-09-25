# r9 gameplay wiring — expected hardware test

`MIDWAY_R8H3_NOTES.md` and `MIDWAY_R8H4_NOTES.md` each end by naming what a
console should show for that revision. `MIDWAY_R8H6_NOTES.md` does not, and
neither does `R9_SOURCE_ENGINE_NOTES.md` -- so the practice lapsed at r8h6 and
has been gone for two revisions. The four r9 commits below are the worst
possible ones to leave unnamed: none of them adds a feature, and
none of them changes a screen. Every one takes behaviour the port had already
translated and *connected* it, so the whole visible difference is that things
which used to do nothing now happen.

Nothing in the 124-test suite can see any of it. The tests prove each value is
computed and delivered; they run headless, at no frame rate, against no TV.
What follows is what a person holding a controller should be able to tell.

## What changed

### `e5f97a1` — the special-move bonus (LIFEBAR.ASM:3302 `BONUS_MESS`)

`wm_smove_fire_t` carries seven fields and the monitor loop read four. The
dropped one was `bonus`, the `movi <n>,a10` that precedes a `BONUS_MESS`
creation, and it is live at 23 sites (22 `CREATE MESSAGE_PID,BONUS_MESS`,
plus YOKO.ASM:803's `CREATE0 BONUS_MESS`).

Past its first-time test, `BONUS_MESS` does three things on every path:
`movk 2,a1 / move a1,@DAM_MULT` (LIFEBAR.ASM:3383), `RND_AWARD a8,HIGH_RISK_AWD`,
and `MOVI 0BBH,A0 / CALLA triple_sound` — the guitar sting. Only the *words*
are gated on the per-message first-time flag; the damage and the award are not.

`DAM_MULT` is 2. The message object it creates is `xDAMAGE`, which is an
external symbol -- it is referenced at LIFEBAR.ASM:3408, :3847 and :3906 and
defined nowhere in the checked-in tree, so what it renders as is not something
this note can state. Expect a damage-multiplier message; do not expect exact
wording from here.

### `1f1a8bd` — three animations and a sound, all reported and dropped

- `mode_dead`'s `#dobuck` convulse (REACT1.ASM:1856 `hitonground_tbl`) and the
  pinner's buckoff (DOINK.ASM:3235 `#buckoff_tbl`), both refused on grounds that
  had stopped being true once the roster anim registry landed.
- The gate crash (WRESTLE.ASM:3695) applied velocities, mode, damage and sound
  and played no animation. Its three-second rule is `cmpi TSEC*3,a14 / jrge #bnc`:
  three seconds **or more** since the last hit bounces off the gate; a more
  recent hit falls back instead.
- The Undertaker's choke loop never stopped. `FIND_AND_KILL_ENDLESS`
  (DCSSOUND.ASM:4223) takes no arguments — it reads `@ENDLESS_SOUND`, stops that
  channel and clears the global — so the comment claiming the backend "has no
  sound queue" to call it with was describing a parameter the routine does not have.

### `7a0aa95` — two powerups delivered to nothing

BLOCKING OFF and HYPER MATCH were computed by `wm_get_powerups`, accepted by the
code-entry screen, reconciled across both players, and handed to no one. Three
consumers, three carriers, none supplied.

- `blocking_off` is tested for nonzero only (`jrnz`), so its `2020h` bit value
  carries through unchanged. Eight dispatchers refuse the block attempt.
- `hyper_speed_on` is **normalised to exactly 1** at AWARD.ASM:2276
  (`movi 1,a8`), and it is a *shift count*, not a flag. Eight wrestler files
  do `sll a14,a0` on run velocity (BRET.ASM:1943 and its seven siblings);
  ANIM.ASM:157 does `srl a14,a1` on the animation frame hold.

  At the shipped value of 1 that is exactly double run speed and exactly half
  the frame hold. Nine read sites in the source, nine in the port.

Both bits are in `BOTH_P_MASK`: they need both players to have entered them.

### `d590158` — one source field, two port fields

`HITBLOCKER` had five translated writes, all to `hit_blocker`. `ANI_IFBLOCKED`
(ANIM.ASM:83), its only reader, read a second field named `hitblocker` that
nothing wrote. Every attack animation asking "was I blocked?" was answered no.

`PLYR_DIZZY` had the same split. It is harmless in the shipped game only because
`check_dizzy` is commented out at WRESTLE2.ASM:1370, so the real field is always
zero and the two halves agreed by accident.

`_ani_immobilize` (ANIM.ASM:3933) was also missing its three velocity clears,
under the source's own comment "clear his velocities too".

## Expected hardware test for this revision

1. **Double damage on a secret move.** Land any of the 23 bonus-carrying special
   moves and watch the opponent's life bar. The bar should drop about twice as
   far as the same move dealt before, the `xDAMAGE` message should appear, and a
   guitar sting should play. The damage and the sting happen **every time**; the
   move-name words appear only the first time that message fires in a match.

2. **Bodies move on the ground.** A wrestler killed while down should convulse
   rather than lie still, and a pinner being bucked off should animate out of the
   pin rather than snap to standing.

3. **The gate crash animates.** Throw someone into the ringside gate more than
   three seconds after their last hit — they should play a bounce animation, not
   slide into it with no frames. Hit them first and throw them immediately and
   they fall back instead; that is the source's rule, not a bug.

4. **The Undertaker's choke stops.** Start the choke, break it. The looping
   choke sound should end. Before this, it ran until the match did.

5. **HYPER MATCH is unmistakable.** Both players must enter it. Wrestlers should
   run at exactly double speed with animation at double rate. This is the single
   most likely thing on this list to look wrong — if it is subtle, or if it is
   wilder than 2x, the shift count is being read wrong somewhere.

6. **BLOCKING OFF.** Both players enter it; the block button should do nothing
   for anyone.

7. **Blocking registers at all.** This has no single dramatic tell and it is the
   widest change of the four — `ANI_IFBLOCKED` went from answering "no" to every
   attack in the game to actually answering. Attacks landing on a blocking
   opponent should now branch differently. If combat *feel* has changed and you
   can't point at which move, this is why.

8. **Immobilised opponents stop dead.** A victim who gets immobilised should stop
   moving, not keep sliding along their last velocity.

## Deliberately still absent

- `pal_getf`, a runtime palette allocator, so `skeleton_pal` stays unwritten and
  Doink's electrocution buzzer has no palette. Inventing an index would make it
  look deliberate and wrong; zero keeps it visibly unfinished.
- `screen_flash`, `shadow_trail`, `impact`, `flash_white` and
  `restore_hit_render_state` — object/palette/DMA work with no renderer behind it.
- Ten ATTRACT.ASM screens whose scheduler is translated and whose bodies are not
  (`show_hstd`, `creditscreen`, `DO_HINTS`, `show_gen_tips`, `show_bios`,
  `show_bios_tips`, `show_operatormsg`, `show_time_date`, `show_copyright`,
  `aama_message`). Two of those are additionally hardware-gated and will stay
  that way: `show_time_date` wants a CMOS real-time clock, and it and
  `show_operatormsg` sit behind operator DIP switches there is no bank to read.
- The DCSSOUND family is the largest unfinished system: `snd_update`,
  `do_tune_commands`, `announcer_sound`, `END_MATCH_SPEECH` and `nosounds` are
  partial, and `play_wrestler_tune` (LIFEBAR.ASM:3003 `DO_RIGHT_MUSIC`) is the
  ledger's one remaining deferred row.

Anything still source-missing stays marked as such. Provisional behaviour is not
claimed as source-exact.
