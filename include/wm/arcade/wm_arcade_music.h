#ifndef WM_ARCADE_MUSIC_H
#define WM_ARCADE_MUSIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The music the arcade mutes, and the music it plays.
 *
 * Four places in the dump send a wrestler's theme to the DCS board,
 * and this port reached none of them. Three are gated, two of them on
 * the same pair of tests written out twice, and the fourth hangs off a
 * one-word latch in LIFEBAR.ASM's BSS that nothing in this tree ever
 * read or wrote. The tables were all here; so was the latch's field,
 * on a struct it cannot belong to; so was its 55-tick sleep, as a
 * constant with no consumer. What was missing was the joining up.
 *
 * Everything below is from the dump. Nothing here invents a music
 * subsystem: SNDSND's argument is a raw board command 0-1ff
 * (DCSSOUND.ASM:2517 says so in its header) and the tune tables hold
 * command numbers, so the port's existing audio queue -- which already
 * carries 1005 and 0 -- is the sink these four sites were always
 * missing.
 */

/* ---- ADJMUSIC, and which way round it reads --------------------- */

/*
 * AUDIT.ASM:2927, FACTORY_TABLE's own row:
 *
 *     .word   1       ;ADJMUSIC       17      ;attract mode music = off
 *
 * The comment is the arcade's, not this port's, and it settles the
 * polarity that the gates below would otherwise leave ambiguous:
 * ADJMUSIC NON-ZERO means attract music OFF, and the value a factory
 * cabinet ships with is 1. So on a cabinet nobody has been into, the
 * DCS_LOGO bang and the eight bio themes do not play at all.
 *
 * src/core/app.c used to say the opposite in a comment -- "ADJMUSIC is
 * not exposed yet; this frontend's default is enabled" -- and played
 * the bang on the first two attract loops. That was a guess, and it
 * was the wrong one.
 */
#define WM_MUSIC_ADJMUSIC_FACTORY 1

/* ---- the gate, written out three times in ATTRACT.ASM ----------- */

/*
 * `MOVE @AMODE_LOOPS,A0 / CMPI 2,A0 / JRGE no / ADJUST ADJMUSIC /
 * JRNZ no` -- the same two tests at ATTRACT.ASM:3004-3009 (DCS_LOGO's
 * 1005) and :2294-2299 (the bio theme), and the same pair with the
 * branches inverted at :670-675 inside TURN_SOUNDS_OFF_IF_NEED. Music
 * is allowed only on attract loops 0 and 1, and only when the operator
 * has turned ADJMUSIC to zero.
 *
 * AMODE_LOOPS is incremented once per base loop and reset to 0 every
 * eighth (ATTRACT.ASM:251-253 and :268-269), so "loops 0 and 1" means
 * the first two cycles of every block of eight.
 */
bool wm_music_attract_allowed(unsigned amode_loops, uint16_t adj_music);

/*
 * TURN_SOUNDS_OFF_IF_NEED (ATTRACT.ASM:669) is the same decision used
 * the other way. When music is NOT allowed it stores
 * ATTRACT.ASM:677-678 `MOVK 2,A0 / MOVE A0,@SOUNDSUP`, which
 * suppresses every SNDSND and every triple_sound rather than just the
 * themes.
 *
 * Returns the value to store in @SOUNDSUP: 2 to suppress, or the
 * current value left alone -- the routine has no "turn them back on"
 * branch. SOUNDSUP is cleared in exactly two places: the boot path
 * (UTIL.ASM:158) and the end of every eighth attract loop
 * (ATTRACT.ASM:270), beside the AMODE_LOOPS reset.
 *
 * The consequence at factory settings is worth stating because it is
 * surprising and it is what the source says: ADJMUSIC is 1, so the
 * FIRST gameplay demo of the FIRST loop already suppresses sound
 * (show_gameplay calls this at ATTRACT.ASM:607 and again at :616), and
 * it stays suppressed until the eighth loop resets it. A factory
 * cabinet's attract mode is nearly silent.
 */
#define WM_MUSIC_SOUNDSUP_DEMO 2
bool wm_music_demo_suppresses_sound(unsigned amode_loops,
                                    uint16_t adj_music);

/* ---- SNDSND's own two refusals (DCSSOUND.ASM:2519) -------------- */

/*
 * `move @SOUNDSUP,a0 / jrnz sendx` then `move a3,a3 / jrn sendx`
 * (:2523-2527). Suppressed, or a negative code, and the call is
 * dropped before either byte reaches the board.
 *
 * Note what is NOT refused: code 0. `CLR A3 / CALLA SNDSND` is the
 * arcade's own "silence the music board" (DCSSOUND.ASM:2467's comment,
 * inside nosounds), so zero is a command like any other -- which also
 * means that once SOUNDSUP is set, nosounds cannot silence the board
 * either. Only its clear_sound_ram half still runs.
 *
 * NOT named after SNDSND, deliberately, and the first draft of this
 * was: port/routine_map.json ledgers SNDSND as `hardware` -- the two
 * byte writes to the sound latch with a poll_sirq wait after each are
 * the board's protocol and an N64 has no ADSP-2105 to write to -- and
 * the coverage collector resolves a routine by any identifier ending
 * in its name. A function called wm_music_sndsnd_allowed would have
 * made SNDSND read as implemented and turned its honest hardware
 * verdict into a stale excuse, which is the exact failure the project
 * found twelve times in one pass and wrote
 * test_the_ledger_cannot_shadow_real_code to prevent. That guard
 * caught this one before it was committed. What is translated is the
 * CALLER's half of SNDSND, which the ledger's own note already
 * claims; the name says so.
 */
bool wm_music_board_call_allowed(uint16_t soundsup, int32_t code);

/* ---- the three tune tables -------------------------------------- */

/*
 * Three tables, two shapes, one set of numbers -- and the difference
 * between the shapes is the whole reason there are three.
 *
 *   LIFEBAR.ASM:3016   .word 5,2,1,7,6,4,8,0,3   (nine)
 *   PROGRESS.ASM:2862  .word 5,2,1,7,6,4,8,0,3   (nine)
 *   ATTRACT.ASM:2817   .word 5,2,1,7,6,4,8,3     (EIGHT)
 *
 * The nine-entry pair is indexed by WRESTLERNUM, which still has Adam
 * Bomb's slot 7 in it; he was cut, and his row is 0. The eight-entry
 * one is indexed by the attract's bio counter, which walks the eight
 * wrestlers that shipped, so slot 7's 0 is simply not there and every
 * row after it has moved down one. Reading the nine-entry table with a
 * bio index, or the eight-entry table with a WRESTLERNUM, gives the
 * wrong man's music for every wrestler past Adam Bomb.
 *
 * The values are raw DCS commands, so Adam Bomb's 0 is not a gap in
 * the table: 0 is the board's silence command.
 */
#define WM_MUSIC_TUNES 9u
#define WM_MUSIC_ATTRACT_TUNES 8u

/* LIFEBAR.ASM:3008-3011, by the winner's WRESTLERNUM. */
int wm_music_wrestler_tune(int32_t wrestler_num);
/*
 * The three tables themselves, so that "the first two are identical
 * and the third is the same list with slot 7 removed" is a claim a
 * test can check rather than one this header merely asserts.
 * `which` is 0 for LIFEBAR's, 1 for PROGRESS's, 2 for ATTRACT's.
 */
const uint8_t *wm_music_table(unsigned which, size_t *count);
/* PROGRESS.ASM:2712-2714, by the human's select index. */
int wm_music_which_music(int32_t select_index);
/* ATTRACT.ASM:2301-2303, by the bio counter. */
int wm_music_attract_tune(int32_t bio_index);

/*
 * ATTRACT.ASM:2303 reads its row as `move *a10,a3,L` -- a LONG, out of
 * a `.word` table whose stride X16 (MACROS.H:192, `sll 4`) makes one
 * word. So A3 comes back holding this row in its low half and the NEXT
 * row in its high half, and the last row reads a word of whatever
 * follows the table. It does not matter: SNDSND masks A3 to its low
 * sixteen bits before splitting it into the two bytes it sends
 * (DCSSOUND.ASM:2530-2531 `sll 32-16,a3 / srl 32-8,a3`), so the upper
 * word is discarded unread. The port therefore reads one row, and this
 * note is here so that the `,L` is not later "fixed" into a bug.
 */

/* ---- MUSIC_HAP (LIFEBAR.ASM:110 `BSSX MUSIC_HAP,16`) ------------ */

/*
 * One word of BSS, .ref'd by ATTRACT.ASM, PROGRESS.ASM and
 * WRESTLE.ASM, and the only state the whole music path has. It is a
 * latch, not a setting: "the winner's theme has already been started".
 *
 *   cleared  ATTRACT.ASM:157   attract mode begins (GAMSTATE=INAMODE)
 *   cleared  WRESTLE.ASM:1679  a match is being set up
 *   cleared  LIFEBAR.ASM:2853-2854  DO_WAIT: somebody won the match
 *   set      LIFEBAR.ASM:3007  DO_RIGHT_MUSIC2, beside its SNDSND
 *
 *   read     PROGRESS.ASM:2709 the pregame, before WHICH_MUSIC
 *   read     LIFEBAR.ASM:2979  the match-over tail, before
 *                              DO_RIGHT_MUSIC2
 *
 * Note that the pregame's SNDSND does NOT set it. Only DO_RIGHT_MUSIC2
 * does, which is what makes the latch mean what it means.
 *
 * This port had the field -- `uint16_t music_hap` on WmAttractState --
 * and no writer and no reader. It also could not have worked there: a
 * per-attract field cannot be the flag the pregame and the match-end
 * both read, and the attract is the one of the three that only ever
 * clears it.
 */

/*
 * And the race the latch exists for. At `#end` (LIFEBAR.ASM:2927) the
 * match-over tail does
 *
 *     CREATE  SOUND_PID,DO_RIGHT_MUSIC
 *
 * which is DO_RIGHT_MUSIC2 behind a `SLEEP 55` (:3003-3005 -- the two
 * entry points are one routine entered a line apart, and that is the
 * only difference between them). It then waits for the award bar and
 * for up to TSEC*3 ticks that a button press cuts short, and only then
 * reads MUSIC_HAP (:2979) and creates DO_RIGHT_MUSIC2 (:2981) if it is
 * still clear.
 *
 * So the second create is a catch-up for a player who skipped the wait
 * faster than 55 ticks, and the latch is how the two processes avoid
 * both playing. Nothing kills the first one, so a skip that beats the
 * sleep leaves BOTH armed: DO_RIGHT_MUSIC2 plays now and DO_RIGHT_MUSIC
 * still fires at tick 55 and sends the same command again. That is the
 * source's behaviour and it is modelled rather than smoothed over.
 */
typedef struct {
    /* @MUSIC_HAP. The source stores the literal 1. */
    uint16_t music_hap;
    /* DO_RIGHT_MUSIC's `SLEEP 55`: ticks left, or 0 for "no process". */
    int16_t pending_ticks;
    /* The tune that deferred process will send when it wakes. */
    int16_t pending_tune;
    /*
     * The command waiting for the board, or -1. This is the SOUND_PID
     * process's `calla SNDSND` held for whoever owns the queue, the
     * same shape as the bell and pin-him processes: the latch decides,
     * the app sends.
     */
    int16_t outbox;
    /* How many commands this latch has emitted, for a test to read. */
    uint32_t sends;
} wm_music_state_t;

void wm_music_init(wm_music_state_t *st);

/* The three `CLR A0 / MOVE A0,@MUSIC_HAP` sites. Clearing the latch
   does NOT disarm a deferred process; nothing in the source does. */
void wm_music_clear(wm_music_state_t *st);

/* `MOVE @MUSIC_HAP,A0 / JRNZ` -- true when the theme is already
   going, which is when both readers skip their SNDSND. */
bool wm_music_started(const wm_music_state_t *st);

/* `CREATE SOUND_PID,DO_RIGHT_MUSIC` (LIFEBAR.ASM:2933). Arms the
   55-tick sleep; nothing is sent and MUSIC_HAP is not touched yet. */
#define WM_MUSIC_DO_RIGHT_SLEEP 55
void wm_music_do_right_music(wm_music_state_t *st, int tune);

/* `CREATE SOUND_PID,DO_RIGHT_MUSIC2` (LIFEBAR.ASM:2981) -- the body
   straight away: MUSIC_HAP set, then the SNDSND. */
void wm_music_do_right_music2(wm_music_state_t *st, int tune);

/*
 * One tick of the deferred process. Runs the body on the tick the
 * sleep runs out, and does nothing at all when none is armed.
 */
void wm_music_tick(wm_music_state_t *st);

/*
 * Take the command the latch wants sent, or -1. Separate from the
 * state so that the queue and the suppression check stay with the
 * caller that owns them -- SNDSND's SOUNDSUP test is the sender's, not
 * this routine's.
 */
int wm_music_take(wm_music_state_t *st);

#ifdef __cplusplus
}
#endif
#endif
