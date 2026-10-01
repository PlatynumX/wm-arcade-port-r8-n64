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

## The attract text blit: items 9-12, NOW reachable and flashable

This block used to say items 9-12 COULD NOT HAPPEN, and listed the five
layers between "the text is computable" and "the text appears". All five
are now joined: show_copyright and aama_message are partial-source so
the app stops skipping them, each has its own tick function with the
source's sleep boundaries, and main.c's attract switch renders both
through wm_text_build_draw_list and wm_rd7text_draw.

Reaching them takes EIGHT attract loops -- `advance_call`'s
`amode_loops & 7` -- which is about 23500 source ticks, roughly seven
and a half minutes of attract mode. Leave it running.

 9. **The copyright screen has words on it, in two pages.** Nine centred
    lines 12 pixels apart from y 110, then a second page of ten. Each
    page holds about three seconds after a settle and any button pages
    forward. Measured call length is 362 source ticks for the pair.

10. **The AAMA advisory appears after it**, one page of six lines, held
    about four seconds -- a second longer than each copyright page,
    because the source says `wait_on_butn 4*TSEC` where copyright says
    3. Measured 236 ticks.

11. **THREE of the nineteen copyright lines are MISSING, and that is
    correct.** The two "(C) 1995 ..." lines and the "(P) 1993" one
    contain parentheses whose widths this port cannot establish, so the
    engine refuses the whole line rather than drawing it short. 7 of
    page one's 9 and 9 of page two's 10 draw. A partly-drawn copyright
    line would be the bug. All six AAMA lines draw.

12. **The text is one flat colour, not coloured per glyph.** DMACNZ is
    "write constant on non-zero data", so each character is stencilled
    in one colour. Multicoloured glyphs, or glyphs in the font artwork's
    own palette, mean the combiner is wrong.

### What these screens are still MISSING, on purpose

- The SMWWF2 logo on the copyright page, its fade_up process, and
  `do_the_grad_thang`'s gradient behind the AAMA text. Those are the
  object and palette layers, the same seam pal_getf and screen_flash
  wait on. Both pages come up on flat black at full brightness. The
  fades' 20-tick settles ARE in the timing, so the pages hold for
  exactly as long as the arcade's do.
- "- MILD" on the AAMA screen should be a DIFFERENT COLOUR from the
  advisory above it (the source's >0606 against >1111). It is not.
  Both source colour words are kept in wm_attract_aama_lines, but
  resolving a TMS palette index to RGB needs the palette this port does
  not allocate. Two invented colours would look deliberate and be
  wrong.

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

## Four more attract screens: items 21-25, and these ARE flashable

ATTRACT.ASM:1416 `show_gen_tips`, :3390 `DO_HINTS`, :2067 `show_bios` and
:2070/:2588 `show_bios_tips` / `show_wres_tips`. Four screens whose text all
comes out of the source's own pointer tables. The extracted text table goes
from 25 rows to 164, and 141 elements reach a screen:

| screen | new text rows | drawn |
|---|---|---|
| `show_gen_tips` | 9 | 9 |
| `DO_HINTS` | 61 | 59 |
| `show_wres_tips` | 49 | 49 |
| `show_bios` | 19 | 24 |

plus one shared `blank` spacer row, which is why the rows total 139. The bios
draw MORE than they have rows because the height and weight are composed at
draw time: 8 hometowns plus 16 number-and-unit runs. What does not reach a
screen is exactly the `blank` spacer wherever it occurs, the two dead `#HNT_7`
lines, and the eight brace-wrapped quotes.

This is the second half of the same join `c8e6e54` opened. The text extractor,
RD7FONT and the stencil blit were already there and already correct; what these
four needed was (a) their text, which the extractor could not reach because
they do not number their lines the way `show_copyright` does, (b) a tick
machine, and (c) a port status other than `not-started`, without which
`skip_untranslated_calls` walked past them whatever else was true.

Two real defects turned up on the way, neither of them in code I had written:

**The hint table was the superseded file's.** `wm_attract_hints` held five
records and `WM_ATTRACT_ACTIVE_HINTS` was 5. That is ATTR.ASM:3482's
`NUM_HINTS`. The shipped ATTRACT.ASM:3386 says 10 and `WHICH_HINT` holds ten.
The two files' orders also diverge after index 2 -- ATTR runs HNT_2,4,3,7,5 and
ATTRACT runs HNT_2,4,3,9,7,5,8,1,6,A -- so the old table was naming the WRONG
hint at two live indices as well as missing five. Task #107 re-anchored this
file's citations and missed the table itself.

**One hint stores two lines the arcade has never drawn.** `#HNT_7` ("SECOND
WIND", index 4) declares 6 in its count word and lists eight pointers, and
`NEXT_HINT` (:3545) uses the count as its `DSJS` counter. `#HNT_7G` and
`#HNT_7H` are in the ROM and have never been on a screen; the second reads
"...DOES NOT WORK IF" / "IF YOU DIE OUTSIDE THE RING.", a doubled IF that went
unnoticed because nobody could see it. The port carries both counts and draws
the count word's.

21. **TONS O' TIPS comes up and holds ten seconds.** A centred title at the top
    and four tips of two lines each, with a gap between the pairs, the column
    running from about a quarter of the way down (arcade y 60) to near the
    bottom (y 210) in steps of 15. 673 ticks (12.7s) end to end: 89 of those are the
    `BLOW_0_TO_1` wipe, during which the screen is deliberately BLANK, then one
    second, then a skippable ten. Any attract button should cut it short.

22. **A hint screen comes up, and a DIFFERENT one next time round.** Title
    centred near the top, three to six body lines centred below it. The first
    one the cabinet ever shows is "COMBO MODE", not "IN-AIR PICK OFF": both
    `last_hint` and `next_bio` increment before use, so index 1 leads. Loop the
    attract mode ten times and you should see all ten titles, in this order
    from a cold boot: COMBO MODE, TURNBUCKLE LEAPS, EXCESSIVE BLOCKING, SECOND
    WIND, REVERSALS, HEAD HOLDS, OUT OF RING, HIGH RISK MANEUVERS, CHOOSE
    WISELY, IN-AIR PICK OFF. **If the THIRD screen is SECOND WIND instead of
    EXCESSIVE BLOCKING, the ATTR.ASM table has come back** -- that old table's
    order from the same cold boot would run COMBO MODE, TURNBUCKLE LEAPS,
    SECOND WIND, REVERSALS, then wrap straight back to IN-AIR PICK OFF after
    five. 957 ticks (18s) each.

23. **SECOND WIND shows SIX lines, ending "ONCE YOUR HEALTH METER RUNS OUT."**
    Not eight. If you can read "NOTE THAT THIS DOES NOT WORK IF" on that
    screen, the port is drawing `line_total` where it should draw `line_count`,
    and it is showing text the arcade never did.

24. **A wrestler bio comes up, then the SAME wrestler's SPECIAL MOVES.** Two
    consecutive screens, 513 ticks (9.7s) each. The bio shows a hometown, a
    height and a weight; the tips page shows a centred "SPECIAL MOVES" and
    three tips of two lines, and those lines are LEFT aligned near the screen
    edge -- the only line column of these four screens that is not centred,
    because `wt_line1_setup` goes through `print_string` and not
    `print_string_C`. The first bio from a cold boot is **Razor Ramon, MIAMI,
    FLORIDA, 6 FT. 7 IN., 262 LBS.** -- wrestler 1, not Bret. Over eight
    attract cycles you should see all eight in roster order ending on Bret, and
    the tips page must never show a different wrestler from the bio before it.

25. **THE BIO QUOTES ARE MISSING, and that is correct.** All eight of them.
    Every quote in `#bio_data` is wrapped in the source's own brace-quotes and
    the four `FONT7par` widths could not be established from the artwork, so
    `wm_text_build_draw_list` refuses the whole line rather than drawing it
    short -- the same refusal as the copyright's three parenthesis lines. If a
    quote appears, something is now guessing a glyph width. (Two of them,
    `#bambam_quote` and `#doink_quote`, are also missing their closing brace in
    the source.)

### What these four screens are still MISSING, on purpose

- **All the artwork.** `show_gen_tips` and `DO_HINTS` want `MVEBAR_R`,
  `SHADOW01` and the `JUDDER_SHADOW` process that shakes the bar; `DO_HINTS`
  adds `MUGBAK`, `MUGFRNT`, the tip author's name object and a `WGSF22` digit
  for the hint number; `show_bios` wants `biopageBMOD`, a wrestler logo through
  `MAKE_UP_LOGO`, a mugshot from `wrestler_mugs2`, `ATT_TXT` and four `ATTMTR`
  attribute bars. Each page is text on black.
- **The two screen transitions.** `BLOW_0_TO_1` and the `CLOSE_SCREEN_LINE` /
  `OPEN_SCREEN_LINE` pair are framebuffer and palette effects with no renderer
  here. Their DURATIONS are kept, because they are plain `SLEEP`s -- 89 ticks
  and 41 ticks each -- and dropping them would have ended every one of these
  screens early. So expect each page to appear abruptly rather than wipe in,
  but to appear at the right moment and last the right length.
- **The two-size typography on the bio's height and weight.** The arcade prints
  the number in `wsf14_ascii` and its unit suffix in `wsf10_ascii` four pixels
  lower, chaining through `@mess_cursx2`. One font and no `cursx2` here, so
  each is one string on the number's baseline: "6 FT. 7 IN." rather than the
  arcade's mixed sizes. The characters and their order are the source's.
- **The per-wrestler tune.** `#wrestler_tunes` is read and the `SNDSND` is not
  issued; that is the sound table, not this screen.

### A thing deliberately NOT fixed here

`src/core/arcade/wmania_attract_core.c` holds a second copy of the `last_hint`
and `next_bio` stepping rules, for a sequencer that only its own tests drive --
nothing in `src/core/app.c` uses it. Unifying the two attract sequencers is a
refactor, not a translation, and it is not attempted in this change. Both
copies are now correct and both are marked; if either changes, both must.

## The high-score tables: items 26-30, and these ARE flashable

ATTRACT.ASM:1443 `show_hstd`, translated as
`src/core/arcade/wmania_hstd_screen.c`.

This one is the clearest instance yet of the shape every commit in this port
keeps finding. `wmania_hiscore_present.h` already carried the row selection,
both titles, the row positions, the highlight flag and **every one of the
source's scroll timings** -- 36 and 34 ticks a step, 85 ticks of hold, 15h to
run the last rows off, half a second of settle, five seconds of final wait --
all correct. And `wm_hs_present_rows()` was called by exactly one test and by
no screen, because this routine's port status was `not-started`, so
`skip_untranslated_calls` walked straight past it. Almost nothing needed
extracting; what was missing was a consumer.

Two findings came out of reading the drawer:

**The initials do not print at the x the caller passes.** `a9` is packed
[Y,X] as the port already believed, but `HSTD.ASM:1652` adds fifty to the x
before storing `mess_cursx`. The printed rows' x of 8 lands at 58. Anything
rendering at 8 would have been fifty pixels out.

**The score position is dead on this path.** Every caller computes one --
[63,10] for the printed rows, [13BH,10] for the spawned ones -- and
`draw_beaten_table_entry` restores it with `pull a9,a10` at `:1624` and never
mentions `a10` again in its remaining 111 lines. What a row shows is initials
plus a row of defeated-wrestler icons, drawn off `a9`; there is no score text.
This is scoped: the tag, pin-speed and win-streak drawers all do use `a10`, so
the layout's score fields are live for those three screens and dead for these
two.

26. **A cold boot now opens on INTERCONTINENTAL CHAMPS.** It is the attract
    loop's first call (`ATTRACT.ASM:183`) and used to be skipped entirely, so
    the machine went straight to the DCS logo. Expect a centred title, three
    rows of initials a third of the way down, then more rows scrolling up past
    them. Then the same again as WORLD CHAMPIONS. 1891 ticks (35.7s) for the
    pair on a fresh cabinet.

27. **The rows GLIDE, they do not step.** Two pixels a tick, with a new row
    appearing from below every 35 ticks -- which is why they end up 70 pixels
    apart. If rows jump a whole row-height at a time, the velocity model has
    been replaced by a counter.

28. **Three rows scroll, then everything stops for about a second and a half,
    then three more.** The pause is 21 ticks still moving plus 85 held still.
    A continuous scroll with no pauses means `A11` is not being counted.

29. **The gap between the third row and the fourth is wider than the rest,**
    and that is correct. 112 pixels instead of 70, because the first scrolled
    row is spawned at y 294 before the velocity has been turned on. It stays
    wider as the pair travels up. Do not report this as a bug.

30. **The scroll stops well short of the bottom of the table, and that is also
    correct on a new cabinet.** The factory tables carry real entries for only
    ten of the thirty intercontinental ranks and fewer of the world ones, and
    `@not_blank` -- whose name says the opposite of what it means -- stops the
    scroll once a zero-score row has been drawn. Measured: the intercontinental
    table reaches rank 12 and the world table rank 9. Put thirty real scores
    in and it runs to rank 30 and takes 4422 ticks instead; the host test does
    exactly that to exercise the other exit.

### What this screen is still MISSING, on purpose

- **The rows show initials only.** The arcade draws up to eight
  defeated-wrestler icons per row off the same position (`which_crouton` per
  set bit of the stored bitmask, `OSGEMD_DOT` for each clear one, 48 pixels
  apart). Those are objects with no pipeline here. There is deliberately no
  score number either, because the score position is dead -- drawing a figure
  there would be inventing a layout the arcade does not have.
- **The highlight.** `WmHstdRow.highlighted` carries the source's GOLD-versus-
  BLUE pick off the `GET_AUD AUD_INTER`/`AUD_BEATEN` comparison, and resolving
  a TMS palette index needs the allocator `pal_getf` would be. So the newest
  entry does not stand apart from the others.
- **`hstd_mod`'s background, the `MVEBAR_R` bar, the `SHADOW01` shadow and the
  `JUDDER_SHADOW` process that shakes it, `BLOW_0_TO_1`'s wipe, and
  `hscore_colcyc`/`hscore_colcyc2`'s colour cycling.** The wipe's 89 ticks are
  kept even though the wipe is not drawn, so the screen is blank for that
  stretch and then simply appears.
- **`SET_PAGE` and `special_copy`**, the banked-ROM read path. The entries come
  from the translated `WmHsSystem` instead, which is where this port keeps the
  tables anyway.

## Deliberately still absent

- `pal_getf`, a runtime palette allocator, so `skeleton_pal` stays unwritten and
  Doink's electrocution buzzer has no palette. Inventing an index would make it
  look deliberate and wrong; zero keeps it visibly unfinished.
- `screen_flash`, `shadow_trail`, `impact`, `flash_white` and
  `restore_hit_render_state` — object/palette/DMA work with no renderer behind it.
- Three ATTRACT.ASM screens whose scheduler is translated and whose bodies are
  not: `creditscreen`, `show_operatormsg` and `show_time_date`. This list was
  ten; `show_copyright` and `aama_message` were translated in `c8e6e54`,
  `DO_HINTS`, `show_gen_tips`, `show_bios` and `show_bios_tips` in `b805dec`,
  and `show_hstd` below. Two of the three will stay absent: `show_time_date`
  wants a CMOS real-time clock, and it and `show_operatormsg` sit behind
  operator DIP switches there is no bank to read -- and `show_operatormsg` has
  no source text to translate in any case, because it reads the message a
  cabinet operator typed into CMOS (`RC_BYTEI` over `CUSTOM_MESSAGE`,
  ATTRACT.ASM:692). `creditscreen` has no such excuse; it is simply not done,
  and its body is a `JSRP CRD_SCRN2` into a file nobody has read yet.
- Every object on the four screens below, and all of their colour. The arcade
  picks SGMD8YEL, SGMD8RED, RUBYPAL, BLUE and WSF_Y_P out of IMGPAL.ASM and
  resolving a TMS palette index needs the allocator `pal_getf` would be, so
  every line is drawn white; and four source fonts (osgemd, ogmd10, wsf14,
  wsf10) collapse to RD7FONT, the only one whose widths and masks have been
  recovered from the artwork.
- The DCSSOUND family is the largest unfinished system: `snd_update`,
  `do_tune_commands`, `announcer_sound`, `END_MATCH_SPEECH` and `nosounds` are
  partial, and `play_wrestler_tune` (LIFEBAR.ASM:3003 `DO_RIGHT_MUSIC`) is the
  ledger's one remaining deferred row.

Anything still source-missing stays marked as such. Provisional behaviour is not
claimed as source-exact.
