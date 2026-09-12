/*
 * DCSSOUND.ASM's four-channel mixer -- see wm/arcade/wm_arcade_sound.h.
 */
#include "wm/arcade/wm_arcade_sound.h"

#include <string.h>

void wm_sound_init(wm_sound_state_t *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
}

wm_sound_announcer_t wm_sound_who_is_it(int32_t index) {
    size_t i;
    /*
     * `X32 a1 / addi triple_sndtab,a1` then a ladder of compares
     * against the table's own labels. Every one of them is `jrlt`,
     * so each range is [previous label, this label).
     */
    if (index < 0) return WM_SOUND_ANNOUNCER_NONE;
    i = (size_t)index;
    if (i < wm_sound_announcer_start) return WM_SOUND_ANNOUNCER_NONE;
    if (i < wm_sound_vince_end) return WM_SOUND_ANNOUNCER_VINCE;
    if (i < wm_sound_randy_end) return WM_SOUND_ANNOUNCER_RANDY;
    if (i < wm_sound_howards_end) return WM_SOUND_ANNOUNCER_HOWARD;
    /* `cmpi more_jerry,a1 / jrlt #error2` -- the source's own comment
       on this gap is "between howard and jerry - bogus". */
    if (i < wm_sound_more_jerry) return WM_SOUND_ANNOUNCER_NONE;
    if (i < wm_sound_triple_end) return WM_SOUND_ANNOUNCER_RANDY;
    return WM_SOUND_ANNOUNCER_NONE;
}

/* tsnd1..tsnd4: take a channel, and send call+(channel-1). */
static void claim(wm_sound_state_t *s, unsigned ch, uint8_t pri,
                  uint8_t dur, uint16_t call, wm_sound_result_t *out) {
    s->priority[ch] = pri;
    s->duration[ch] = dur;
    s->call[ch] = (uint16_t)(call + ch);
    out->played = true;
    out->channel = (uint8_t)(ch + 1u);
    out->duration = dur;
    out->call = s->call[ch];
}

wm_sound_result_t wm_sound_triple(wm_sound_state_t *s, int32_t index) {
    wm_sound_result_t out;
    const wm_sound_entry_t *e;
    unsigned ch, lowest;

    memset(&out, 0, sizeof(out));
    if (!s) return out;

    /* `move @SOUNDSUP,b3,W / jrne send_rets` -- are we allowed. */
    if (s->suppressed) return out;

    /*
     * `TEST a0 / jrn #a0lo`, and `cmpi triple_end,a0 / jrhs #a0hi`.
     * Both land on the ordinary exit rather than an error: a bad
     * index plays nothing and says so.
     */
    if (index < 0 || (size_t)index >= wm_sound_table_count) return out;
    e = &wm_sound_table[(size_t)index];

    /* `move *a0+,a1,W / jreq tsnd9` -- a zero priority word is the
       table's own blank row. 104 of the 771 are blank. */
    if (e->priority == 0 && e->duration == 0) return out;
    /* `move *a0+,a3,W / jrz #zcall` -- and a zero call is nothing to
       send even when the row has a priority. */
    if (e->call == 0) return out;

    /* A free channel first, in order: `move @chan1pri,a0,W / jreq
       tsnd1` and so on down. */
    for (ch = 0; ch < WM_SOUND_CHANNELS; ++ch) {
        if (s->priority[ch] == 0) {
            claim(s, ch, e->priority, e->duration, e->call, &out);
            return out;
        }
    }

    /*
     * "all channels used up ---> see if I am more important". The
     * source's own note beside it is "New Method = find lowest-
     * priority call and bump it if I outrank it", and the search
     * keeps the FIRST channel holding the minimum: each step is
     * `cmp a5,a14 / jrge #check_N`, which skips a later channel
     * whose priority merely ties.
     */
    lowest = 0;
    for (ch = 1; ch < WM_SOUND_CHANNELS; ++ch)
        if (s->priority[ch] < s->priority[lowest]) lowest = ch;

    /*
     * `cmp a5,a1 / jrlt #no_preempt` -- the comparison is
     * (new - lowest) and it gives up only when that is NEGATIVE, so
     * an exact TIE preempts. Reading it as a strict > would silently
     * drop every sound that matched the one already playing, which
     * for the grunts and smacks that share a priority is most of
     * them.
     */
    if (e->priority < s->priority[lowest]) return out;

    claim(s, lowest, e->priority, e->duration, e->call, &out);
    return out;
}

void wm_sound_update(wm_sound_state_t *s) {
    unsigned i;
    if (!s) return;

    /* `move @vincedur,a0,W / jrz #no_vince / dec a0 / ...` for each
       of the three. They are counted down whether or not the channel
       under them is still busy. */
    for (i = WM_SOUND_ANNOUNCER_VINCE; i <= WM_SOUND_ANNOUNCER_HOWARD; ++i) {
        if (s->announcer_duration[i] != 0) {
            s->announcer_duration[i] -= 1;
            if (s->announcer_duration[i] == 0) s->announcer_channel[i] = 0;
        }
    }

    /*
     * `move @chanNdur,a0,W / jrz supN / dec a0 / move a0,@chanNdur,W
     * / jrnz supN` then, with no tune script, `move a1,@chanNpri,W`
     * -- a1 is zero there, so the channel is freed.
     */
    for (i = 0; i < WM_SOUND_CHANNELS; ++i) {
        if (s->duration[i] == 0) continue;
        s->duration[i] -= 1;
        if (s->duration[i] != 0) continue;
        s->priority[i] = 0;
        s->call[i] = 0;
    }
}

wm_sound_result_t wm_sound_announcer(wm_sound_state_t *s, int32_t index) {
    wm_sound_result_t out;
    wm_sound_announcer_t who;
    const wm_sound_entry_t *e;
    unsigned ch;

    memset(&out, 0, sizeof(out));
    if (!s) return out;

    /* `CALLR WHO_IS_IT / JRC #error` -- not an announcer call at all. */
    who = wm_sound_who_is_it(index);
    if (who == WM_SOUND_ANNOUNCER_NONE) return out;
    if (index < 0 || (size_t)index >= wm_sound_table_count) return out;

    /* `jrz #no_cutoff` -- his duration is zero, so he is not talking
       and this is an ordinary triple_sound. */
    if (s->announcer_duration[who] == 0) {
        out = wm_sound_triple(s, index);
        if (out.played) {
            s->announcer_duration[who] = out.duration;
            s->announcer_channel[who] = out.channel;
        }
        return out;
    }

    /*
     * "Guy is already talking. Start the new call on the same track."
     * Straight onto his channel, with no priority test of any kind --
     * he cannot lose an argument with himself.
     */
    ch = s->announcer_channel[who];
    if (ch < 1u || ch > WM_SOUND_CHANNELS) return out;   /* `bad channel?!` */
    ch -= 1u;

    e = &wm_sound_table[(size_t)index];
    s->priority[ch] = e->priority;
    s->duration[ch] = e->duration;
    s->call[ch] = (uint16_t)(e->call + ch);
    s->announcer_duration[who] = e->duration;

    out.played = true;
    out.channel = (uint8_t)(ch + 1u);
    out.duration = e->duration;
    out.call = s->call[ch];
    return out;
}
