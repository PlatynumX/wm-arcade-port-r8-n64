#include "wm/arcade/wm_arcade_music.h"

#include <stddef.h>
#include <string.h>

/*
 * ATTRACT.ASM:3004-3009, :2294-2299 and :670-675 -- the same two tests
 * three times over. Written once here, and the three call sites say
 * which of them they are.
 */
bool wm_music_attract_allowed(unsigned amode_loops, uint16_t adj_music) {
    /* `MOVE @AMODE_LOOPS,A0 / CMPI 2,A0 / JRGE no` */
    if (amode_loops >= 2u) return false;
    /* `ADJUST ADJMUSIC / JRNZ no` -- non-zero is OFF. */
    if (adj_music != 0u) return false;
    return true;
}

bool wm_music_demo_suppresses_sound(unsigned amode_loops,
                                    uint16_t adj_music) {
    /*
     * TURN_SOUNDS_OFF_IF_NEED tests the two in the other order and
     * with the branches inverted -- `ADJUST ADJMUSIC / JRNZ
     * TURN_OFF_SOUNDS` then `CMPI 2,A0 / JRLT SOUNDS_SHOULD_BE_ON` --
     * which comes to the same decision. It is the same gate read as
     * "suppress unless music is allowed", and routing it through the
     * one predicate is what keeps the two from drifting apart.
     */
    return !wm_music_attract_allowed(amode_loops, adj_music);
}

bool wm_music_board_call_allowed(uint16_t soundsup, int32_t code) {
    /* `move @SOUNDSUP,a0 / jrnz sendx` (DCSSOUND.ASM:2523-2524). */
    if (soundsup != 0u) return false;
    /* `move a3,a3 / jrn sendx` (:2526-2527) -- a NEGATIVE code only.
       Zero is the board's silence command, not a null call. */
    if (code < 0) return false;
    return true;
}

/*
 * One set of numbers, two shapes. The nine-entry rows are
 * LIFEBAR.ASM:3016 and PROGRESS.ASM:2862, which are identical; the
 * eight-entry row is ATTRACT.ASM:2817, the same list with Adam Bomb's
 * 0 removed rather than zeroed.
 *
 * Transcribed one array per source table, rather than one shared array
 * behind three accessors: the first two being identical is a fact
 * about the dump that test_music.c asserts, not an assumption this
 * file is entitled to make.
 */
static const uint8_t WRESTLER_TUNES[WM_MUSIC_TUNES] = {      /* LIFEBAR:3016 */
    5, 2, 1, 7, 6, 4, 8, 0, 3
};
static const uint8_t WHICH_MUSIC[WM_MUSIC_TUNES] = {         /* PROGRESS:2862 */
    5, 2, 1, 7, 6, 4, 8, 0, 3
};
static const uint8_t ATTRACT_TUNES[WM_MUSIC_ATTRACT_TUNES] = { /* ATTRACT:2817 */
    5, 2, 1, 7, 6, 4, 8, 3
};

/*
 * So that what the comment above says about these three -- two shapes,
 * one set of numbers, slot 7 removed rather than zeroed -- can be
 * checked rather than trusted. Exported for test_music.c alone; no
 * source routine is named WHICH_MUSIC's accessor, and nothing in src/
 * needs a table it does not already have a named reader for.
 */
const uint8_t *wm_music_table(unsigned which, size_t *count) {
    switch (which) {
    case 0: if (count) *count = WM_MUSIC_TUNES; return WRESTLER_TUNES;
    case 1: if (count) *count = WM_MUSIC_TUNES; return WHICH_MUSIC;
    case 2: if (count) *count = WM_MUSIC_ATTRACT_TUNES; return ATTRACT_TUNES;
    default: if (count) *count = 0u; return NULL;
    }
}

/*
 * The source does no range check on any of the three: it shifts the
 * index and adds it to the table address. An out-of-range index there
 * reads whatever follows the table, which is not something a port can
 * reproduce or should try to. Each of these returns 0 instead, which
 * is the board's silence command -- the quietest wrong answer
 * available, and the same value Adam Bomb's real row holds.
 */
int wm_music_wrestler_tune(int32_t wrestler_num) {
    if (wrestler_num < 0 || wrestler_num >= (int32_t)WM_MUSIC_TUNES)
        return 0;
    return (int)WRESTLER_TUNES[wrestler_num];
}

int wm_music_which_music(int32_t select_index) {
    if (select_index < 0 || select_index >= (int32_t)WM_MUSIC_TUNES)
        return 0;
    return (int)WHICH_MUSIC[select_index];
}

int wm_music_attract_tune(int32_t bio_index) {
    if (bio_index < 0 || bio_index >= (int32_t)WM_MUSIC_ATTRACT_TUNES)
        return 0;
    return (int)ATTRACT_TUNES[bio_index];
}

void wm_music_init(wm_music_state_t *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    st->outbox = -1;
}

void wm_music_clear(wm_music_state_t *st) {
    /* `CLR A0 / MOVE A0,@MUSIC_HAP`, and nothing else: the three
       sites touch the one word. A deferred DO_RIGHT_MUSIC keeps its
       sleep, because no source site kills that process. */
    if (st) st->music_hap = 0u;
}

bool wm_music_started(const wm_music_state_t *st) {
    return st != NULL && st->music_hap != 0u;
}

/* The shared body of DO_RIGHT_MUSIC2 (LIFEBAR.ASM:3005-3013). */
static void do_right_music2_body(wm_music_state_t *st, int tune) {
    /* `MOVK 1,A0 / MOVE A0,@MUSIC_HAP` -- the literal 1, before the
       send, so a reader that runs between the two still sees the
       theme as started. */
    st->music_hap = 1u;
    /* `MOVE *A11,A3 / calla SNDSND`. */
    st->outbox = (int16_t)tune;
    ++st->sends;
}

void wm_music_do_right_music(wm_music_state_t *st, int tune) {
    if (!st) return;
    /*
     * `CREATE SOUND_PID,DO_RIGHT_MUSIC`, whose first instruction is
     * `SLEEP 55`. The tune is captured now because the source reads it
     * from *A10(WRESTLERNUM) -- the winner's own process block, which
     * the created process carries a pointer to -- rather than from
     * anything that could change under it.
     */
    st->pending_ticks = (int16_t)WM_MUSIC_DO_RIGHT_SLEEP;
    st->pending_tune = (int16_t)tune;
    /*
     * One slot, not a queue, and that is enough rather than a
     * simplification: `CREATE SOUND_PID,DO_RIGHT_MUSIC` is reached
     * once per match -- `#end` is the match-over branch, taken when a
     * side has its second round -- and the only other entry into these
     * five instructions is DO_RIGHT_MUSIC2, which has no sleep to
     * hold. Two deferred themes cannot be in flight at once.
     */
}

void wm_music_do_right_music2(wm_music_state_t *st, int tune) {
    if (!st) return;
    do_right_music2_body(st, tune);
}

void wm_music_tick(wm_music_state_t *st) {
    if (!st || st->pending_ticks <= 0) return;
    /*
     * `SLEEP 55` then fall through. The body runs on the 55th tick
     * after the create, which is the same counting every other
     * translated sleep in this tree uses.
     */
    if (--st->pending_ticks == 0)
        do_right_music2_body(st, (int)st->pending_tune);
}

int wm_music_take(wm_music_state_t *st) {
    int out;
    if (!st) return -1;
    out = (int)st->outbox;
    st->outbox = -1;
    return out;
}
