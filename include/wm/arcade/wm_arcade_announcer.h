#ifndef WM_ARCADE_ANNOUNCER_H
#define WM_ARCADE_ANNOUNCER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DCSSOUND.ASM's announcer voice queue -- the arbitration this port has
 * been missing, and the reason the whole announcer group of ANI_CODE
 * routines could not be translated. CALL_MISSES alone is 177 call sites,
 * and every one of them ends in ADD_IF_SILENT: "say this, but only if the
 * announcer has nothing else to say."
 *
 * The queue is a ring (DCSSOUND.ASM:185 `bssx ANNOUNCE_QUEUE,32*20` with
 * EOF_ANNOUNCE_QUEUE marking its end) that ANNOUNCE_VOICE drains one line
 * at a time, sleeping until each finishes. Its exact slot count is not
 * quite pinned by the source -- the allocation reads as twenty longs while
 * the stores are plain word-sized `MOVE A0,*A1+` -- so this uses the
 * larger reading. Nothing observable depends on it: the source has no
 * overflow check either, and wraps over unread entries exactly as this
 * does.
 */
#define WM_ANNOUNCE_QUEUE_SLOTS 40

/*
 * DCSSOUND.ASM:2880's split. ANNOUNCE_VOICE routes each queued index by
 * value: below 0E0h it is an ordinary sound and goes to triple_sound;
 * from 0E0h up it is a spoken line and goes to announcer_sound. Both are
 * the same seam in this port (wm_anim_env::sound), so the split is kept
 * as a reported flag rather than two callbacks.
 */
#define WM_ANNOUNCE_VOICE_FIRST 0xE0u

/* SOUND.EQU:136 -- the line ANI_INC_COMBO asks for at eight hits. */
#define WM_VOICE_HES_JUST_GONE_BERSERK 0x1AAu

typedef struct {
    uint16_t slot[WM_ANNOUNCE_QUEUE_SLOTS];
    /* NEXT_ANN_QUEUE / CURRENT_ANN_QUEUE: write and read cursors. Equal
       means empty, which is exactly what IF_SILENT_ADD_VOICE tests. */
    uint8_t next;
    uint8_t current;
    /*
     * ANNOUNCE_VOICE is a process that plays one line and then PRCSLPs
     * until it finishes. This port has no sound-completion signal, so the
     * pump holds for `busy_ticks` instead -- set by the caller to whatever
     * it knows about line length, and 0 for "play them back to back",
     * which is the behaviour a port with no completion callback can
     * honestly offer.
     */
    uint16_t busy;
    uint16_t busy_ticks;
} wm_announcer_state;

/* DCSSOUND.ASM RESET_VOICE_QUEUE. */
void wm_announcer_init(wm_announcer_state *a);

/* Is the announcer silent? The queue empty AND nothing being played --
   IF_SILENT_ADD_VOICE's own `CMP A1,A2 / JRNE NO_ADD` plus WHO_IS_IT. */
bool wm_announcer_is_silent(const wm_announcer_state *a);

/*
 * ADD_VOICE: push unconditionally. Returns false for the source's own
 * OUT_OF_RANGE_SOUND -- a negative index, or one past the end of
 * triple_sndtab -- which it treats as an error rather than a sound.
 */
bool wm_announcer_add(wm_announcer_state *a, uint16_t call);

/* IF_SILENT_ADD_VOICE: the same, but only when nothing is being said.
   Returns false when it declined, which is the -1 the source returns. */
bool wm_announcer_add_if_silent(wm_announcer_state *a, uint16_t call);

/*
 * ANNOUNCE_VOICE, one tick of it. Plays at most one queued line through
 * `sound`, then holds for busy_ticks before the next. `is_voice`, when non-NULL,
 * receives which side of the 0E0h split the played line fell on (true for
 * announcer_sound, false for triple_sound) and is only written when a line
 * actually played. Returns true if a line was played this tick.
 */
bool wm_announcer_tick(wm_announcer_state *a, void *user,
                       void (*sound)(void *user, uint16_t call),
                       bool *is_voice);

#ifdef __cplusplus
}
#endif
#endif
