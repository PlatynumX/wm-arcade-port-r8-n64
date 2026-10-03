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

(Since established -- see "The quiet bout was six one-way doors".) One thing NOT fixed and worth watching: one bout in twenty (Bret v Bret
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

## The credit screen: items 31-33, and the last attract screen

ATTRACT.ASM:793 `creditscreen`, a thin wrapper around AUDIT.ASM:998
`CRD_SCRN2`. **`CRD_SCRN2` was in the dump all along** -- in `AUDIT.ASM`. The
note here previously said its body was "a `JSRP CRD_SCRN2` into a file nobody
has read yet", which was true only in the sense that nobody had looked.

**This screen is different in kind from every other attract screen**, and that
difference is the whole of what is worth knowing about it. The others display
DATA, which the port either already had or could extract. This one displays
the cabinet's **coin state** -- and this port has none of it: no credits, no
coin inputs, no CMOS adjustments, no pricing tables, no DIP bank, no tamper
audit. `credits_string` does not read its text out of a table; it *evaluates*
it, and every branch is chosen by a setting that does not exist here.

So the port shows exactly one state: a factory-set cabinet with nothing in it,
which is precisely the state it is in. The five lines were **derived** from the
arcade's own `FACTORY_TABLE` (AUDIT.ASM:2905) with zero credits, and the whole
derivation is written out in `wm/arcade/wmania_attract_data.h` so it can be
checked rather than trusted -- there is no coin subsystem here to check it
against, which is exactly why it is written down.

31. **A credit screen comes up twice per attract loop and holds five
    seconds.** After each of the two gameplay demos. Five centred lines:

        CREDITS : 0
        1 CREDIT / 1 COIN
        2 CREDITS TO START
        2 CREDITS TO CONTINUE
        INSERT COINS

    271 ticks (5.1s). Text appears at tick 6 and any attract button cuts it
    short from tick 60 -- not before, because `CRD_SCRN2` does not sample
    buttons during its three `SLEEPK 2` or its `SLEEP 1*TSEC`.

32. **It says INSERT COINS, not PRESS START, and CREDITS : 0.** Both follow
    from there being no coin subsystem: zero credits divided by the factory's
    two-credits-to-start is zero, so the routine takes `#not_ready`. If you
    ever see "PRESS  START", "READY FOR 1 PLAYER", "FREE  PLAY" or any credit
    count but zero, something is now inventing cabinet state.

33. **Both plurals are plural.** "2 CREDITS TO START", not "2 CREDIT TO
    START". The source picks the singular form only when the count is exactly
    one, and the factory value is two. Getting this wrong would be the
    cheapest possible tell that the suffix is hard-coded rather than selected.

### What this screen never shows, and why

Every line below is real source text, extracted and not invented -- it is
simply never *selected*, because selecting it needs state the port has not
got. None of it is drawn:

| line | needs |
|---|---|
| `FREE  PLAY` | `ADJFREPL` non-zero, i.e. an operator menu |
| `PRESS  START` | credits ≥ credits-to-start |
| `READY FOR 1 PLAYER` and its 2/3/4 variants | the same |
| ` (MAXIMUM)` | credits ≥ `ADJMAXC` (50) |
| any credit count but `0` | a coin input |
| any pricing line but `1 CREDIT / 1 COIN` | `ADJPRICE` ≠ 1, a pricing table |
| the dollar-bill message | UJ2 switch 1, a DIP bank |

Also absent: `slateBMOD`'s background, and all five palettes the lines ask for
(`GOLD`, `SGMD8GLD`, `GREENPAL` and the rest), so the page is white on black
like every other screen.

### Two source documentation errors found reading this

- **`TAMPEREDP` states its own polarity backwards.** ADJUST.ASM:946 says "THIS
  IS NON-ZERO IF ANY OF THE 1ST 6 COIN PARAMETERS HAVE BEEN ADJUSTED", but the
  routine returns `GET_ADJ(ADJ1ST6)` and `ADJ1ST6` is documented at
  AUDIT.ASM:2932 as "NON-ZERO MEANS 1ST 6 UNTOUCHED". Non-zero means **not**
  tampered. The factory value is 1, so a fresh cabinet shows the pricing line
  and an operator who has edited the coinage hides it -- the opposite of what
  the header leads you to expect.
- **`crd_updatetxt` runs twice** (AUDIT.ASM:1032 and :1057), and the second
  call opens with `KILALL` and `obj_delc`, so it deletes the text the first
  call drew and draws it again. The port draws once; the result on screen is
  the same.

### One path that is unreachable from the attract loop

`creditscreen` passes `a10 = 1`, so `#ck2`'s `move a10,a10 / jrnz KILL_CRD2`
always returns to the caller. The `10*TSEC` second wait loop and the
`KILL_CRD` exit that re-`CREATE`s `attract_mode` belong to the `CRD_SCRN`
entry point (`a10 = 0`), which nothing in the shipped attract loop calls.

## The music: items 34-37, and the port gets quieter on purpose

Four places in the dump hand a wrestler's theme to the DCS board. The port
reached **none** of them, and the one command it *did* send was one the arcade
would have muted.

| # | what changed | where |
|---|---|---|
| 34 | the DCS logo's bang is gated on ADJMUSIC, not just the loop count | `ATTRACT.ASM:3004-3009` |
| 35 | the bio screens play the wrestler's theme | `ATTRACT.ASM:2290-2306` |
| 36 | `@SOUNDSUP` is written by the game instead of only by a test | `ATTRACT.ASM:669` |
| 37 | the winner's theme and the pregame's reach the board, through `@MUSIC_HAP` | `LIFEBAR.ASM:2933`, `PROGRESS.ASM:2709` |

### ADJMUSIC ships at 1, and 1 means OFF

`AUDIT.ASM:2927` is `.word 1 ;ADJMUSIC 17 ;attract mode music = off`. The
comment is the arcade's own and it settles both halves at once: non-zero means
attract music **off**, and a cabinet nobody has been into ships at 1.

`src/core/app.c` said the opposite, in as many words -- *"ADJMUSIC is not
exposed yet; this frontend's default is enabled"* -- and played DCS_LOGO's
command 1005 on the first two attract loops of every eight. On hardware, with
the exact recovered DCS asset behind it. **A factory cabinet plays no bang at
all.** So this change makes the port quieter, and that is the fix, not a
regression.

Turn ADJMUSIC to zero -- what an operator who wants attract music does -- and
both the bang and the eight bio themes come back. The test checks both
settings.

### The gate is TWO tests, and ATTRACT.ASM writes it out three times

```
	MOVE	@AMODE_LOOPS,A0
	CMPI	2,A0
	JRGE	no
	ADJUST	ADJMUSIC
	JRNZ	no
```

at `:3004-3009` for the bang, at `:2294-2299` for the bio theme, and with the
branches inverted at `:670-675` inside `TURN_SOUNDS_OFF_IF_NEED`. The port made
only the loop-count half of the first, made none of the second, and had the
third translated **twice** in `wmania_attract_core.c` and called by nothing.
One translation now, in `wm/arcade/wm_arcade_music.h`, with the two published
attract-core predicates delegating to it.

### Three tune tables, two shapes, one set of numbers

| table | row | indexed by |
|---|---|---|
| `LIFEBAR.ASM:3016` | `5,2,1,7,6,4,8,0,3` | the winner's `WRESTLERNUM` |
| `PROGRESS.ASM:2862` | `5,2,1,7,6,4,8,0,3` | the human's select index |
| `ATTRACT.ASM:2817` | `5,2,1,7,6,4,8,3` | the attract's bio counter |

The third has **eight** entries: the same list with Adam Bomb's cut slot 7
**removed** rather than zeroed, so every row after it has moved down one. Read
either shape with the other's index and every wrestler past Adam Bomb gets the
wrong man's music. The values are raw board commands, which is why his 0 is not
a gap in the table -- 0 is the board's own silence command
(`DCSSOUND.ASM:2466-2467`).

The port had the nine-entry row transcribed twice: in `pregame.c` as
`which_music_source`, read only by a `(void)` cast to keep the compiler quiet,
and again in `wm_arcade_match_end.c`. Each source table is now transcribed
once, and a test asserts that the first two are identical rather than assuming
it.

### `@MUSIC_HAP` was a field on the wrong struct, written by nobody

`LIFEBAR.ASM:110` `BSSX MUSIC_HAP,16` -- one word, `.ref`'d by `ATTRACT.ASM`,
`PROGRESS.ASM` and `WRESTLE.ASM`. The port had it as `uint16_t music_hap` on
`WmAttractState`, with no writer and no reader. It could not have worked there
even filled in: the attract is the one of the three files that **only clears**
it.

| | where |
|---|---|
| cleared | `ATTRACT.ASM:157` (attract begins), `WRESTLE.ASM:1679` (match setup), `LIFEBAR.ASM:2853-2854` (`DO_WAIT`) |
| set | `LIFEBAR.ASM:3007`, inside `DO_RIGHT_MUSIC2` |
| read | `PROGRESS.ASM:2709` (before `WHICH_MUSIC`), `LIFEBAR.ASM:2979` (before the catch-up create) |

The pregame's own `SNDSND` does **not** set it. That is what makes the latch
mean "the winner's theme has already been started" rather than "some music is
playing".

### The race the latch exists for

`#end` (`LIFEBAR.ASM:2933`) `CREATE`s `DO_RIGHT_MUSIC`, which is
`DO_RIGHT_MUSIC2` behind a `SLEEP 55` -- two entry points one line apart, and
that sleep is the only difference between them. The tail then waits for the
award bar and for up to `TSEC*3` that a button press cuts short, and only then
reads `MUSIC_HAP` and creates `DO_RIGHT_MUSIC2` (`:2981`) if it is still clear.

So the second create is a **catch-up** for a player who skipped the wait in
under 55 ticks. Nothing kills the first process, so a skip that beats the sleep
leaves both armed and the same command goes out twice. That is modelled, not
smoothed over.

The port instead assigned `tune` at the tip phase and played nothing, with
`WM_MATCH_MUSIC_SLEEP 55` defined beside it and used by nobody.

### `@SOUNDSUP` was written only by a test

`wm_sound_state_t.suppressed` is the flag `triple_sound` and `SNDSND` both
refuse on, and nothing in the live app ever set it -- so
`TURN_SOUNDS_OFF_IF_NEED`'s whole purpose, muting the gameplay demo, did
nothing at all. It is wired now at `show_gameplay`'s two call sites
(`ATTRACT.ASM:607` and `:616`), cleared at the eighth loop's reset
(`:268-270`, where one register goes to both `AMODE_LOOPS` and `SOUNDSUP`), and
cleared at the coin drop (`AUDIT.ASM:457-458`, *"TURN SOUNDS ON. (A-MODE SOUND
SUPRESSOR)"*), for which this port's Start-on-title bridge stands in as it
already does for PSTATUS.

At factory settings ADJMUSIC alone satisfies the gate, so the **first** gameplay
demo mutes the board and it stays muted until the eighth loop. **Bell
included** -- which is why `test_sound_procs` now checks both settings instead
of asserting the bell is always heard. A probe puts the first suppression at
tick 2850 and the reset at tick 65856.

### A name of mine nearly certified a routine

The refusal helper was first called `wm_music_sndsnd_allowed`. The coverage
collector resolves a source routine by any identifier **ending** in its name,
so `SNDSND` -- honestly ledgered `hardware`, because the two byte writes and
the `poll_sirq` waits are the board's protocol -- began reading as implemented
and its verdict silently became a stale excuse.
`test_the_ledger_cannot_shadow_real_code` caught it before the commit. Renamed
`wm_music_board_call_allowed`, which is also what it actually is: the caller's
half of `SNDSND`, which the ledger's own note already claimed was translated.

### Two ledger rows were wrong rather than stale

Both are deleted with the translation.

- **`DO_RIGHT_MUSIC`** was ledgered `display` with the note *"Picks the music
  for the round about to start and hands it to SNDSND."* It is not display, and
  it plays the **winner's** theme after the match is **over** -- not the round
  about to start.
- **`DO_RIGHT_MUSIC2`** read *"The same for the second half of a match."* It is
  a second entry point into the same five instructions, one line later.

### What is still absent here, on purpose

- **No sample data for commands 1 through 8.** The N64 backend's default branch
  logs an untranslated DCS command rather than playing something else
  (`src/platform/n64/audio_backend.c`), so queuing the real command number
  guesses nothing. That was the half of `pregame.c`'s old refusal that was
  right; the other half had it dropping the command instead of sending it.
- **No operator-settings system.** ADJMUSIC is one field holding
  `FACTORY_TABLE`'s value, the same arrangement as ADJVOLUME's
  `WM_SOUND_ADJVOLUME_DEFAULT` and the credit screen's figures.
- `ATTRACT.ASM:2303` reads its table row as `move *a10,a3,L` out of a `.word`
  table, so A3 comes back with the **next** row in its high half.
  `DCSSOUND.ASM:2530-2531` masks A3 to sixteen bits before splitting it into
  the two bytes it sends, so the upper word is discarded unread and the port
  reads one row. Written down so the `,L` is not later "fixed" into a bug.

## The palettes: items 38-40, and Doink's buzzer finally has a colour

PAL.ASM's allocator was translated, unit-tested, and **owned by nobody**. The
only `wm_pal_state` anywhere in the tree was a file-static in
`tests/arcade_port/pal/test_pal.c`.

| # | what changed | where |
|---|---|---|
| 38 | the app owns one PALRAM, with DIAGP as colour map 0 | `UTIL.ASM:130`, `PAL.ASM:116-118` |
| 39 | `pal_transfer` drains the queue once a frame | `MAIN.ASM:719` |
| 40 | `set_position` resolves `DNKBLU_P` into `SKELETON_PAL` | `DNKSEQ3.ASM:493` |

### An unwired seam, and a missing call behind it

Three notes in this tree had explained why `skeleton_pal` stayed zero, each
with a different wrong reason:

1. *"there is no palette allocator here to ask"* — false. `PAL.ASM:236`
   `pal_getf` is translated, slot reuse, free-slot scan, `pal_clean` fallback
   and all, and `DNKBLU_P` is registered by name in
   `src/generated/palettes.c`.
2. *"nothing in the LIVE app supplies `env->pal_getf`"* — true, and fixed: the
   app owns one `wm_pal_state` and `match.c` fills the seam from it.
3. Neither note mentioned the part that would have left it zero anyway.
   `anim_code.c`'s `set_position` did only the `MY_PAL = OBJ_PAL` half of
   `DNKSEQ3.ASM:476` and **never attempted** the `pal_getf` at `:493`. The
   resolve had no call site to be unwired at.

`set_position`'s name is a lie worth recording: every position write in it is
commented out (`:486`, `:489`, `:492`), so it moves nobody. What is live is

```
	movi	DNKBLU_P,a0
	calla	pal_getf
	move	a0,*A13(SKELETON_PAL)
	move	*a13(OBJ_PAL),a0
	move	a0,*a13(MY_PAL)
```

One global `SUBR`, `.ref`'d by eight other wrestlers' SEQ files — every
wrestler's `get_buzz_anim` calls it, and it asks for `DNKBLU_P` in all of them,
because the buzzer is Doink's and the victim turns Doink-blue.

### 84 KB does not go in a struct four tests put on the stack

`wm_pal_state` is 84 KB — FADERAM alone is 80 x 256 words, three times the whole
of the rest of `wm_app`. So it is a file-static in `app.c` reached through
`wm_app::pal`, which `wm_arcade_pal.h` asks for in as many words, and which
matches the arcade: PALRAM and FADERAM are fixed RAM regions and COLRAM is the
hardware's, none of them per-game state.

### `pal_init` runs where the source runs it

`calla pal_init` is inside `WIPEOUT` (`UTIL.ASM:130`), and `attract_mode`'s
first two instructions are `calla display_blank / calla WIPEOUT`
(`ATTRACT.ASM:142-143`). So every entry into attract mode puts PALRAM back to
DIAGP-and-nothing-else, and slot 0 stays permanently taken — `pal_clean` starts
at slot 1 — which is what makes 0 safe as "no palette" for every caller of the
by-name bridge.

The nine other `calla WIPEOUT` sites in `ATTRACT.ASM` are not modelled, and the
reason is measurable rather than a shrug: this port's attract screens allocate
no palettes for them to free. The only two by-name requests in the whole tree
are made during a match.

### Draining the transfer queue is not cosmetic

`MAIN.ASM:719` `calla pal_transfer ;Copy new PALs` sits inside `DIRQ` (`:592`,
"Display IRQ"), once a frame. `pal_getf` claims a slot **only if** `pal_set`
found a free transfer cell, and there are `WM_NUMPALT` (60) of them — so a queue
nobody empties fills up and then every allocation fails for good. Nothing read
COLRAM and nothing drained the queue. The port runs it beside `snd_update`,
which the same interrupt runs.

### What is still absent here

- **`in_use` and `ignore_this_pal`**, `pal_clean`'s questions to OBJLST and
  BAKLST. `wm_obj_pool` carries the `palette` field for exactly that and has no
  instance in `src/` either — a second subsystem with no owner. `pal_clean` is
  also only reached when all 80 slots are full, which three palettes cannot do.
- **The step after the allocator.** A slot gives you a palette *number*;
  turning its COLRAM colours into something this renderer can blit is still
  missing, which is what keeps the AAMA screen's two colours and the high-score
  highlight undrawn. Both of those notes used to blame `pal_getf` and have been
  corrected to blame the right step.

## The seam audit could not see half the seams

`tools/seam_audit.py` exists so that leaving a callback seam empty has to be a
decision somebody wrote down. It keyed on `port_coverage.CALLBACK_DECL`:

```
\(\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*\(
```

which is the **inline** field form, `RET (*name)(...)`. A field declared
through a typedef matches nothing at all — the regex matches the typedef's
*type* name instead:

```c
typedef bool (*wm_pal_in_use_fn)(void *user, uint16_t dma_pal);  /* matches */
    wm_pal_in_use_fn in_use;                                     /* invisible */
```

Eleven such fields exist. Six were already filled, which is why nothing else
had gone wrong. **Five were called through a NULL check in `src/` and assigned
by nobody there**, so they sat outside the ledger the tool enforces. Seams
counted: 162 → 173.

Found the way the tool's first blind spot was: by hand, while wiring something
else.

### It is not only the count

Three places read the same declaration set, and one of them is the **seam
carve-out** — the rule that stops a seam's name from certifying the source
routine it is named after. So a typedef'd seam named after a source routine
would have certified it, which is the exact failure the carve-out exists to
prevent. Nothing moved when this was fixed, and that was checked rather than
hoped: none of the five currently matches a source routine name, and
`docs/ROUTINE_COVERAGE.md` is byte-identical across the change.

### The five, and what they are

| seam | verdict | why |
|---|---|---|
| `width_fn` | `unreached` | `wm_string_state` has no instance in `src/` |
| `draw_fn` | `unreached` | the same — nothing makes a state to print with |
| `set_image` | `display` | the rope's object image, DMA work with no renderer |
| `in_use` | `deferred` | `pal_clean`'s OBJLST walk; `wm_obj_pool` has no owner |
| `ignore_this_pal` | `deferred` | the same, behind a second gate nothing sets |

**A third unowned subsystem.** `wm_string_state` — STRING.ASM's whole text
engine, translated and tested — has no instance in `src/` at all, beside
PAL.ASM's allocator (which this cycle gave an owner) and DISPLAY.ASM's object
pool (which still has none). The live attract text goes through
`wm_text_build_draw_list`, a different path.

Their checks name the engine's two **constructors**, `wm_string_init` and
`wm_string_setup_message`, rather than its printer. The inner entry points all
call each other inside their own file, so *"nothing in `src/` calls
`wm_string_print`"* is false while telling you nothing. Nothing in `src/` makes
a `wm_string_state`; that is the premise, and either constructor being used
trips a check.

**`width_fn` is not a fallback**, which its own header's wording suggests. The
artwork genuinely does not answer every glyph, and the gap is measured in
`tests/arcade_port/text/test_string.c` rather than asserted in prose:

| font | glyphs | answered by the art |
|---|---|---|
| font9, font9a, font18, wgsf24, wsf10, wsf14 | 69/69/65/65/67/68 | all |
| osgemd | 78 | 45 |
| osgmd8 | 78 | 46 |
| win_ascii | 75 | 10 |
| ogmd10 | 75 | **0** |

With the seam empty those glyphs measure 0 and advance the cursor by the
inter-character spacing alone — which matters not at all today, because nothing
drives the engine, and would matter immediately if anything did.

### Two more the heuristic flagged, both explained rather than wired

- **`fn`** — declared twice, assigned once, and both halves are regex
  artifacts. The second declaration is the ANI_CODE registry row's own field,
  and that table is three hundred static initialisers filling it
  **positionally**, so the `.fn =` the assignment regex looks for never
  appears.
- **`random_range`** — its unfilled copy is on the hiscore *renderer* adapter,
  the same shape as `play_sound` beside it; the live initials entry drives
  `WmHsEntryState` directly, and that copy is filled.

## The link line had one right parser and three wrong ones

`WRESTLE.CMD` is the only authority on what is in the game, and four tools read
it. Three matched `name.obj` at the start of a line with no knowledge of C
comments, and the file's lines 50-51 are

```
/* robo.obj
coll2.obj */
```

So `ROBO.ASM` was excluded only because `/*` happens to sit in front of it on
its line, and `COLL2.ASM` — starting the next line **inside the same comment** —
was counted as shipped by `port_coverage`, `citation_check` and `wlanim`.
`link_resolve.py` stripped comments correctly, and its docstring named exactly
those two objects; the manifest even recorded its answer, 99. The right number
was in the tree and written down, and three tools disagreed with it.

### What that bought: a false `implemented`

`COLL2.ASM`'s one routine, `collisions`, sat in the routine accounting as in
scope and resolved **`implemented`** — by `wm_arcade_check_wrestler_collisions`,
which ends with the same word and translates `COLLIS.ASM`'s `check_collisions`,
a different routine in a different file. Two errors cancelling into a plausible
number. Its only caller is `ROBO.ASM`'s `CREATE COLL_PID,collisions`, and
`ROBO.ASM` is the other half of the same comment.

All three now call `link_resolve.linked_objs`. Routines 2607 → 2606,
`implemented` 2042 → 2041, open still 0, and no generated file changed.
`test_every_link_line_parser_agrees_with_the_linker` holds all four to one
answer; each parser mutated back to its old behaviour fails it.

### And it answered the object-list question

`COLL2`'s `collx` builds the collision lists from OBJLST. It was the **only**
reader of the object list in the dump that is game logic rather than
presentation — and it is not in the game. Every reader that *is* in the game
draws something:

| reader | what it does |
|---|---|
| `process_dispatch` (MPROC) | the inlined `obj_yzsort` — draw order |
| `do_win_streaks`, `SHIFT_BARS_IN_Z` (LIFEBAR) | scrolling plates, bar Z order |
| `STOP_ALL_OBJS`, `MOVE_ALL_OBJS_UP`, `DELETE_ANY_OFF_TOP` (HSTD) | the high-score scroll |
| `blink_rndper` (SELECT) | blinking text palettes |
| `CREATE_TEXT_LINE` (SPECIAL), `scrn_rel_off` (PROGRESS) | text, screen-relative offsets |
| `move_pu_bars_on` (AWARD), `show_operatormsg` (ATTRACT) | bars sliding in, the operator screen |
| `pal_clean` (PAL) | unreached at this port's palette demand |

So `wm_obj_pool` is **not** given an owner, deliberately. With no renderer that
draws *from* it, an instance would be filled by nothing and asked by nothing,
and filling `pal_clean`'s `in_use` seam from an always-empty list would make a
ledger row disappear while changing no behaviour at all — a hollow wiring,
which is worse than an honest `deferred`. It gets an owner when something draws
from it.

## A constant's name was certifying its routine

The coverage tool resolves a source routine by any port identifier that **is**
its name or **ends with** it. An object-like `#define` or an enumerator ends
with a routine's name as readily as a function does, and twenty-one routines
were counted `implemented` on the strength of a value's name alone.

Two were plainly false. **`show_operatormsg` and `show_time_date` read
`implemented`**, certified by `WM_ATTRACT_SHOW_OPERATORMSG` and
`WM_ATTRACT_SHOW_TIME_DATE` — the attract enum's own values — while the
manifest said `not-started`, `app.c`'s dispatch had no case for either, and
`skip_untranslated_calls` walked straight past them. The coverage report and the
manifest contradicted each other, and nothing compared them.

The tool now drops object-like `#define`s and enumerators from the identifier
pool (function-like macros stay, being code). The twenty that fell open each got
a verdict read off the source:

| routine(s) | verdict | the real translation |
|---|---|---|
| ten attract screens | `renamed` | the `tick_*` function `app.c`'s dispatch sends their enum to |
| `show_operatormsg`, `show_time_date` | `hardware` | none — CMOS message; DIPs and the RTC |
| five `print_string*` | `renamed` | arms of `wm_string_print_method` |
| `set_zvel` (HRTSEQ3) | `renamed` | `set_zvel_lead` — it had been certified by an *opcode* |
| `set_wrestler_xflip` | `renamed` | `set_wrestler_xflip_code` (translated three times) |
| `BLOCK_WOOSH` | `renamed` | `wm_wrsnd_label`'s FIXED `0x16` row |
| `flare_anim2` | `data` | the frame index `WM_FW_FLARE_ANIM2` — honest, for a label |

`SHAKER` fell to `dead`, measured: PROGRESS.ASM `.ref`s it and nothing calls it.
`implemented` 2041 → 2020, every one of the 21 accounted for elsewhere, open
still 0.

`test_no_constant_certifies_a_routine` measures constants with its **own**
patterns, not the tool's. With the shared helper, a mutation that stopped
collecting `#define`s survived — the guard and the thing it guards agreeing
that nothing was wrong.

The ledger is deliberately exempt. Four BRET.ASM rows (hrt_grab_toss_air,
hrt_hdhold_ddt, hrt_hdhold_faceslam, hrt_roll_uppercut) name a monitor-id
enumerator as their symbol, because the body lives in that id's `case` in
wm_arcade_bret_fire_monitor. A ledger row is a reviewed verdict, so its
symbol only has to be declared in code. `port_constants()` gives the ledger
check that pool. The suffix rule, which no one reviews, still does not get it.

## The quiet bout was six one-way doors

The drone-opponent fix left one bout in twenty with no damage at all
(Bret v Bret, seed 1). These notes called it "worth watching" and
"NOT established". It was a defect, and under it were five more. Each
one left a live attract bout stuck in a single state for the rest of
the match. All six were found by watching per-mode occupancy over the
same twenty bouts and following the longest run back to its cause.

1. **`drn_ontb` threw away its stick.** DRONE.ASM:2677's "Push into
   turnbuckle" `drone_seekxz` was called with the result discarded. But
   `drone_seekxz` ends `move a0,*a13(DRN_JOY)` (:3078), so the result
   *is* the stick. Both Bret drones walked to the corners and stood in
   NORMAL for 20000 ticks.
2. **A flung wrestler was never handed back.** WRESTLE2.ASM:3433
   `start_run_flung` calls `#x_flip` (:3505) and `#ok2` (:3488), and
   neither was translated. `#ok2` is what puts him in WAITANIM with
   `#dorun_flung` waiting in CODE_ADDR and halves his slide. The
   Undertaker slid from x=1142 to x=4888 and lay ONGROUND for 18000
   ticks. `#dorun_flung` is now a CODE_ADDR token in both backends.
3. **A torso SETMODE wrote the body's mode word.** ANIM.ASM:371 writes
   `*a10(OANIMODE)`, and the secondary channel's a10 points at ANIMODE2
   (:84). A turn animation on the torso cleared MODE_UNINT the tick a
   grab connected. The attacker walked off and left Razor a puppet for
   515 ticks, then ONGROUND. The opcode's STATUS_FLAGS half
   (`SF_CLEAR_BITS`, PTIME) was missing from the VM too.
4. **Animations could kill in attract mode.** `_ani_damage` and
   `_ani_damageopp` called `adjust_health` with attract mode off.
   LIFEBAR.ASM:1578 says "if we're in attract mode, don't die!". Shawn
   lay DEAD for 15464 ticks while the attract refill gave his life back.
   Both opcodes now go through the match's own adapter.
5. **The drone layer read INRING the wrong way round.** The actor's
   `in_ring` is 1 inside, but `drone_main` and `drn_enterring` tested
   it the source's way, so the drone *inside* was sent to enter the
   ring. `drn_ontb`'s `#ering` and `#ny` had been called unreachable
   before wrestlers could leave the ring. Both are translated now.
6. **Every bout was treated as the first ladder rung.** That caps
   DRN_MODE at "No aggressive" (DRONE.ASM:216). ATTRACT.ASM:596-602
   never picks the first rung, so attract drones could not restart a
   bout that had drifted apart.

Over the twenty bouts: damage events went 1248 → 7104, and zero-damage
bouts 1 → 0. The longest ONGROUND run went 18000+ → 1586 and the longest
PUPPET run 515 → 139. DEAD ticks in attract went 15464 → 0. The furthest
anyone got from ring centre went 3814 → 913. `test_quiet_bout.c` pins
each cause; 17 mutations, 17 caught.

**Since translated.** `drone_chkrun`'s `#out` and `drn_run`'s
out-of-ring arm are covered in the next section.

**Not established.** A drone outside the ring at a corner. Seed 3 Bam v
Doink leaves Doink at (809, 962). `drn_enterring` aims him at the ring's
left end exactly as DRONE.ASM:2778-2795 does, and the mat-edge confine
holds him in the corner, outside `ck_climb_in_top`'s window. Whether the
arcade drone sticks there too can't be settled from the code.

## A running drone only thought about hitting you at the ropes

`drn_run` (DRONE.ASM:2382) steers a running drone. Its in-ring arm does
`cmpi RING_X_CENTER+210,a3 / jrlt #rpok ;Won't hit R rope?`, so a runner
who is clear of the rope goes to `#rpok` and considers a strike. Only
`#chkopp`, at the rope, can break the run off. The port had this inside
out: the clear runner went to `#rsk`, which only steers. A running drone
in the ring considered a strike only when it was about to bounce. And for
a standing opponent within 180 it skipped `#oprun`'s Z test.

The two outside arms are translated too, so wrestlers can now run outside
the ring. `drone_chkrun`'s `#out` skips the run buttons when running
would hit the crowd or the ring's side. `drn_run`'s outside arm taps away
and jumps to `drn_enterring` or `drn_seek` (`#ering`, `#brkseek`), or
breaks off.

Over the twenty bouts: the quietest bout went from 1 damage event to 263,
and every bout now draws blood. The longest unbroken run went 3776 → 194
ticks for RUNNING and 19746 → 4617 for NORMAL. `test_drone_running.c`
pins each path; 8 mutations, 8 caught.

## Deliberately still absent

- Nothing about `pal_getf` any more: the allocator is owned, the seam is
  filled, and `skeleton_pal` is written from `DNKBLU_P` -- see the palette
  section above. What is still missing is the step AFTER it, turning a colour
  map's COLRAM contents into something this renderer can blit.
- `screen_flash`, `shadow_trail`, `impact`, `flash_white` and
  `restore_hit_render_state` — object/palette/DMA work with no renderer behind it.
- Two ATTRACT.ASM screens whose scheduler is translated and whose bodies are
  not, and both will stay that way: `show_time_date` wants a CMOS real-time
  clock, and it and `show_operatormsg` sit behind operator DIP switches there
  is no bank to read -- and `show_operatormsg` has no source text to translate
  in any case, because it reads the message a cabinet operator typed into CMOS
  (`RC_BYTEI` over `CUSTOM_MESSAGE`, ATTRACT.ASM:692). **Every other attract
  screen is now translated.** This list was ten: `show_copyright` and
  `aama_message` in `c8e6e54`; `DO_HINTS`, `show_gen_tips`, `show_bios` and
  `show_bios_tips` in `b805dec`; `show_hstd` in `f7cc208`; `creditscreen`
  below.
- Every object on the four screens below, and all of their colour. The arcade
  picks SGMD8YEL, SGMD8RED, RUBYPAL, BLUE and WSF_Y_P out of IMGPAL.ASM and
  resolving a TMS palette index needs the allocator `pal_getf` would be, so
  every line is drawn white; and four source fonts (osgemd, ogmd10, wsf14,
  wsf10) collapse to RD7FONT, the only one whose widths and masks have been
  recovered from the artwork.
- The DCSSOUND family is the largest unfinished system: `snd_update`,
  `do_tune_commands`, `announcer_sound`, `END_MATCH_SPEECH` and `nosounds` are
  partial. `play_wrestler_tune` is no longer on that list and its note was
  wrong twice over: it named `LIFEBAR.ASM:3003 DO_RIGHT_MUSIC`, but the seam's
  only two call sites are the BIO screens and they read a **different table**
  (`ATTRACT.ASM:2817`, eight entries, by bio counter -- not the nine-entry one
  by WRESTLERNUM the note quoted); and it claimed the blocker was that the
  adapter "has no music sink", when `SNDSND`'s own header calls its argument a
  "sound code (0-1ff)" (`DCSSOUND.ASM:2517`) and `wm/audio.h`'s queue has been
  carrying exactly those since the DCS logo work. What is actually true is that
  `wm_attract_run_cycle`, the function that reads the adapter, **has no caller
  anywhere** -- so the row is `unreached`, and the bio theme is played by the
  live sequencer in `app.c` instead. `pal_getf` is the ledger's one remaining
  deferred row.
- Unifying the two attract sequencers. `src/core/arcade/wmania_attract_adapter.c`
  is a second one that nothing drives -- not src/, not tests -- and it is where
  `play_wrestler_tune` lives. Collapsing it into `app.c`'s is a refactor, not a
  translation.

Anything still source-missing stays marked as such. Provisional behaviour is not
claimed as source-exact.
