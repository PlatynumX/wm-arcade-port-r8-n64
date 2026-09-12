#ifndef WM_ARCADE_SOUND_H
#define WM_ARCADE_SOUND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DCSSOUND.ASM's four-channel sound mixer: triple_sound (:2061),
 * announcer_sound (:1937) and snd_update (:2266).
 *
 * This is the middle of a chain whose two ends were already here and
 * had nothing between them. WRSND picks an index out of a wrestler's
 * own sound table (src/generated/wrestler_sound_tables.c, read out of
 * the source by tools/wlwrsnd.py); the animation VM's sound opcodes
 * and a hundred-odd `calla triple_sound` call sites carry one; and at
 * the far end wm/audio.h has a command queue the platform drains.
 * What was missing is what an index MEANS and whether it gets played
 * at all -- so every sound this port decided on went into the queue
 * unarbitrated, in the order it happened, four-deep polyphony and
 * priority alike ignored.
 *
 * What the arcade actually does with one:
 *
 *   triple_sndtab[index] is a priority, a duration and a DCS call.
 *   There are four channels. A free one takes the sound. If all four
 *   are busy, the LOWEST-priority channel is found and the new sound
 *   takes it only if it outranks or ties it -- otherwise the sound is
 *   simply dropped, which is the behaviour that keeps a pile-up of
 *   grunts from burying the announcer.
 *
 *   A channel is busy until its duration runs out. snd_update counts
 *   every channel down once a tick and frees it at zero.
 *
 * ------------------------------------------------------------------
 * NOT here, and deliberately: the DCS board itself. SNDSND's
 * send_code_a3 writes the call to a latch one byte at a time with
 * poll_sirq waits between, and poll_sirq, inc_sdram, clear_sound_ram,
 * snd_reset and qsndrst_proc are all that protocol or its debug ring
 * buffer. An N64 has no ADSP-2105 and no sample ROM to drive with it.
 * What survives the boundary is the CALL -- the 16-bit value the
 * board would have been handed -- which goes into wm/audio.h's queue
 * for a platform mixer to interpret.
 */

/* One row of DCSSOUND.ASM:235 triple_sndtab. */
typedef struct {
    /* The high byte of word 0: sp_* / 256, 0-100. */
    uint8_t priority;
    /* Its low byte, in ticks. */
    uint8_t duration;
    /* Word 1 -- the DCS call for CHANNEL ONE. Channels two, three and
       four use call+1, call+2 and call+3, which is why triple_sound
       does `inc a3` / `addk 2,a3` / `addk 3,a3` rather than looking
       anything else up. */
    uint16_t call;
} wm_sound_entry_t;

extern const wm_sound_entry_t wm_sound_table[];
extern const size_t wm_sound_table_count;

/*
 * The labels inside the table. ANNOUNCE_VOICE decides WHICH of the
 * three announcers is speaking purely by where an index falls
 * between these, so they are data the runtime needs rather than
 * documentation. Note the shape: Jerry ("randy") has TWO blocks, one
 * before Howard and one after a gap the source itself calls bogus.
 */
extern const size_t wm_sound_triple_sndtab;
extern const size_t wm_sound_announcer_start;
extern const size_t wm_sound_vince_end;
extern const size_t wm_sound_randy_end;
extern const size_t wm_sound_howards_end;
extern const size_t wm_sound_more_jerry;
extern const size_t wm_sound_wrestle_end;
extern const size_t wm_sound_triple_end;

#define WM_SOUND_CHANNELS 4

typedef enum {
    WM_SOUND_ANNOUNCER_NONE = 0,
    WM_SOUND_ANNOUNCER_VINCE,
    WM_SOUND_ANNOUNCER_RANDY,     /* Jerry, as the source's labels have it */
    WM_SOUND_ANNOUNCER_HOWARD
} wm_sound_announcer_t;

typedef struct {
    /* chanNpri / chanNdur / chanNsnd. A priority of zero is the
       source's own "this channel is free". */
    uint8_t priority[WM_SOUND_CHANNELS];
    uint8_t duration[WM_SOUND_CHANNELS];
    uint16_t call[WM_SOUND_CHANNELS];
    /* vincedur / randydur / howarddur, and the channel each is on.
       Counted down by wm_sound_update beside the four above. */
    uint8_t announcer_duration[4];   /* indexed by wm_sound_announcer_t */
    uint8_t announcer_channel[4];    /* 1-4, or 0 for "not talking" */
    /*
     * @SOUNDSUP, "SOUND ENABLED FLAG" (DCSSOUND.ASM:180) -- and note
     * the sense, which the name inverts: triple_sound and SNDSND both
     * refuse when it is NON-zero. It suppresses.
     */
    bool suppressed;
} wm_sound_state_t;

void wm_sound_init(wm_sound_state_t *s);

/*
 * What one call did. The source returns this in a14 as
 * [channel, duration] with zero for "nothing played", plus a carry
 * flag that separates "there was nothing to play" (set) from
 * "refused" (clear). Both are here because two callers read them:
 * announcer_sound wants the duration, and ring_bell keeps the
 * channel so it can stop the bell early.
 */
typedef struct {
    bool played;
    /* 1-4 when played. */
    uint8_t channel;
    uint8_t duration;
    uint16_t call;
} wm_sound_result_t;

/*
 * DCSSOUND.ASM:2061 triple_sound. `index` is a row of the table.
 *
 * Returns what happened. A negative or out-of-range index, and a row
 * whose priority word or channel-one call is zero, all play nothing
 * and are not errors -- the source treats them as ordinary no-ops
 * (`#a0hi`, `#a0lo` and `#zcall` all reach the success exit).
 */
wm_sound_result_t wm_sound_triple(wm_sound_state_t *s, int32_t index);

/*
 * DCSSOUND.ASM:2266 snd_update, once per source tick: every
 * announcer's duration and every channel's, counted down, and a
 * channel whose duration reaches zero is freed.
 *
 * Not translated: do_tune_commands. A channel running a TUNE SCRIPT
 * does not free itself at zero -- it runs the next command in the
 * script instead -- and no tune script is reachable in this port,
 * because the music routines that install one (DO_RIGHT_MUSIC,
 * wmania_tune) are themselves still open.
 */
void wm_sound_update(wm_sound_state_t *s);

/*
 * DCSSOUND.ASM:1869 WHO_IS_IT: which announcer, if any, owns this
 * index. The answer is decided entirely by the table labels above.
 */
wm_sound_announcer_t wm_sound_who_is_it(int32_t index);

/*
 * DCSSOUND.ASM:1937 announcer_sound. "Lots like triple_sound, except
 * that it identifies the announcer who's talking, and if he's already
 * saying something, the new call cuts off the old one."
 *
 * That cut-off is the whole point and it does NOT go through the
 * priority arbitration: the new line is written straight onto the
 * channel the announcer is already using, whatever is scheduled
 * there, so an announcer can never talk over himself. Only when he
 * is silent does this fall through to triple_sound and take its
 * chances with the other four voices.
 */
wm_sound_result_t wm_sound_announcer(wm_sound_state_t *s, int32_t index);

#ifdef __cplusplus
}
#endif

#endif /* WM_ARCADE_SOUND_H */
