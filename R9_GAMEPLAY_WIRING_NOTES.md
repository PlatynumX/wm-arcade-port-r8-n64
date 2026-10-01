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

## The attract text blit: NOT reachable yet, so do not flash for it

`src/platform/n64/rd7text.c` compiles for mips64 against real libdragon headers
-- `wm_n64_link` now does that, where before it skipped the platform layer
entirely -- and every position it draws at is checked headlessly.

IT IS ALSO CALLED BY NOBODY, and items 9-12 below CANNOT HAPPEN until that
changes. They are written down as what to look for once the chain is
connected, not as something a flash today would show. Five layers sit between
"the text is computable" and "the text appears":

  - `wm_rd7text_draw` has no caller.
  - `wm_text_build_draw_list` is called only by tests.
  - `wmania_attract_adapter` -- 160 lines with a full callback set, including
    `show_copyright` -- is built by nothing in the live app.
  - `show_copyright`'s port status is NOT_STARTED, so
    `wm_attract_call_is_translated` skips it out of the attract flow.
  - `main.c`'s render switch has four cases and none for copyright.

An earlier version of this section promised these checks outright. That was
wrong and is corrected here rather than quietly dropped: a note that promises
behaviour the build cannot produce sends someone to a TV for nothing.

WHEN THE CHAIN IS CONNECTED, these are the things to look at. A compile is not
a picture, and the combiner mode in particular is either right or completely
wrong with nothing in between.

9. **The copyright screen has words on it.** Nine centred lines, 12 pixels
   apart, starting 110 from the top, then a second page of ten. Before this
   they were nine exact positions with nothing drawn at them.

10. **The text is one flat colour, not coloured per glyph.** The source draws
    it `DMACNZ` -- "write constant on non-zero data" -- so every character is
    stencilled in the single colour the caller passes. If the glyphs come out
    multicoloured, or in the font artwork's own palette, the combiner is
    wrong: it should take RGB from the primitive colour and alpha from the
    texture, and nothing else.

11. **Three lines will be MISSING, and that is correct.** The two
    "(C) 1995 ..." lines and the "(P) 1993" one contain parentheses, whose
    widths this port cannot establish (see `src/generated/rd7font.c`). The
    engine refuses a line it cannot measure rather than drawing it wrong, so
    those three do not appear at all. A partly-drawn copyright line would be
    the bug; a missing one is the intended refusal.

12. **Nothing should be clipped or doubled at the glyph edges.** Each glyph
    uploads as a small I8 texture whose row pitch is padded to 8 bytes, with
    the blit width set to the glyph's real width. If padding columns are being
    sampled, characters will show a smear to their right.

## The rope bounce: items 13-16, and these ARE flashable

`ANI_END` (ANIM.ASM:2497) ORs `MODE_END` into the mode word. The port's
animation VM set an internal `ended` flag instead and never that bit, so
every `btst MODE_END_BIT` in the game read a bit nothing wrote --
including the one in all nine dispatchers' `mode_bouncing` (BRET.ASM:2232
and its counterparts), which is the only way out of `PLYRMODE BOUNCING`.

The measured effect, two Bret drones over 20000 ticks, before and after:

| | before | after |
|---|---|---|
| actor-ticks in BOUNCING | 39845 / 40000 | 448 / 40000 |
| actor-ticks in NORMAL | 59 | 19482 |
| actor-ticks in BLOCK | 0 | 19057 |
| distinct animations played | 5 | 8 |
| of those, carrying an `ATTACK_ON` | 2 | 4 |
| damage taken by either man | none, on any seed | lands |

13. **A wrestler who runs into the ropes comes off them.** He should rebound
    once, turn round and run the other way. If he sticks to the ropes and
    stays there for the rest of the round, this is back.

14. **The wrestlers fight.** Two drones left alone must throw punches, kicks
    and blocks, and the life bars must move. Before this they ran back and
    forth between the ropes for the full match and neither bar ever moved.

15. **Nobody is permanently uninterruptable.** `MODE_END` is retired by the
    next `change_anim1a` (ANIM.ASM:4543 `clr a0 / move a0,*a13(ANIMODE)`),
    which the port had also been missing. If a wrestler freezes mid-move and
    stops responding, that clear is the suspect, not this set.

16. **A puppet still hangs limp.** `wres_slave_anim` ends on
    `MODE_UNINT+MODE_NOAUTOFLIP+MODE_NOGRAVITY` then `ANI_END`, and the bit
    is ORed in, so all four survive. A carried wrestler who starts falling
    out of his carrier's arms means the OR became an assignment.

## The drone's opponent: items 17-20, and these ARE flashable

`drone_main` (DRONE.ASM:121) opens by reading `CLOSEST_NUM` and indexing
`process_ptrs` to find who it is fighting. The port read `smart_target`
instead -- the SMART_ATTACK lock, which `ani_attack_off` zeroes on every
attack end. It was NULL on 38136 of 40000 actor-ticks (95.3%), so the AI
switched itself off from the first completed attack onward while the last
stick direction kept being re-committed.

Five Bret-v-Bret bouts, 80000 actor-ticks, before and after:

| PLYRMODE | before (ticks / longest run) | after |
|---|---|---|
| NORMAL | 63561 / 8000 | 51570 / 1764 |
| RUNNING | 1018 / 65 | 16998 / 70 |
| INAIR | 0 | 1090 / 73 |
| ONGROUND | 0 | 2256 / 228 |
| ONTURNBKL | 7854 / 7854 | 0 |
| BLOCK | 7093 / 7057 | 1238 / 40 |
| damage, 20 bouts | 10 events | 1248 events |

17. **A wrestler who climbs the turnbuckle comes off it -- and this one
    needs a HUMAN to test.** He should jump or climb down within a second
    or two; standing up there for the rest of the round is the defect
    returning, and that was 7854 consecutive ticks.
    Watching attract mode will NOT show it either way. Measured after the
    fix, across 25 bouts and 400000 actor-ticks, the drones enter
    PLYRMODE ONTURNBKL exactly ZERO times -- down from 39265 before it.
    That is consistent with the source's gate (ck_climb_out_top needs
    `stick_val_cur` to equal UP_LEFT or UP_RIGHT exactly while at the
    rope, and a drone steering toward an opponent does not hold a pure
    diagonal there), but it also means the turnbuckle path is now
    UNEXERCISED in drone-versus-drone play. It is not proven working by
    anything in this repo; only a player climbing it will say.

18. **The drones keep fighting all round, not just at the start.** The
    old behaviour was normal play for roughly the first fifteen seconds
    (tick 927 is where it died) and then two men drifting with the stick
    stuck wherever it last was. If the match opens well and goes inert,
    look here and not at the collision code.

19. **Wrestlers get knocked down and get up again.** PLYRMODE INAIR and
    ONGROUND never occurred at all before this; a bout with no knockdowns
    means attacks are not landing.

20. **A blocking wrestler stops blocking.** Block was held for 7057
    consecutive ticks; it should break within a second. Note the drones
    still block each other a great deal -- roughly half the time at equal
    skill -- so frequent blocking is expected and a permanent block is
    not.

What this DID make reachable for the first time: mode_waitanim's
CODE_ADDR continuation, the deferral every ring climb goes through
(WRESTLE2.ASM:200 climb_turnbuckle, :578 ck_climb_out_side, :702
ck_climb_in_side). PLYRMODE WAITANIM had occurred ZERO times in 400000
actor-ticks before this and now occurs 16 times, with `code_addr`
non-zero on exactly those same 16 -- the token stored and consumed in
step rather than left dangling. 16 in 400000 is rare, which fits: it
takes a wrestler walking into the side ropes facing away.

Still at zero, and expected to be: the final battle's zombie promotion
(wm_arcade_final_battle.c:230, which also gates on MODE_END). It needs
the last rung of the championship ladder, not a plain bout, and is
covered by tests/arcade_port/collis/test_final_battle_queue.c instead.

One thing NOT fixed and worth watching: one bout in twenty (Bret v Bret
on seed 1 of the test's own feed) still produces no damage at all, and
Lex v Razor is reliably the quietest pairing. Two equally skilled drones
can spend a round blocking each other, so this may be correct. It is not
established either way, and the test asserts the aggregate rather than
pretending otherwise.

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
