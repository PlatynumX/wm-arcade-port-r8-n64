/*
 * DCSSOUND.ASM's four-channel mixer -- see wm/arcade/wm_arcade_sound.h.
 */
#include "wm/arcade/wm_arcade_sound.h"

#include <string.h>

#include "wm/wrestler_sound_tables.h"

void wm_sound_init(wm_sound_state_t *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->master_volume = WM_SOUND_ADJVOLUME_DEFAULT;
}

void wm_sound_kill_all(wm_sound_state_t *s) {
    unsigned i;
    if (!s) return;
    /* `CLR A0 / MOVE A0,@chanNdur / MOVE A0,@chanNpri` four times,
       each followed by its own stop call to the board. */
    for (i = 0; i < WM_SOUND_CHANNELS; ++i) {
        s->duration[i] = 0;
        s->priority[i] = 0;
        s->call[i] = 0;
    }
    /* An announcer cut off mid-line is no longer talking. */
    memset(s->announcer_duration, 0, sizeof(s->announcer_duration));
    memset(s->announcer_channel, 0, sizeof(s->announcer_channel));
}

void wm_sound_fade_start(wm_sound_fade_t *f, uint8_t from, int32_t ticks) {
    if (!f) return;
    memset(f, 0, sizeof(*f));
    if (ticks <= 0) return;
    /* `MOVE A0,A9 / SLL 16,A9 / MOVE A9,A10 / DIVU A8,A9`. */
    f->level = (int32_t)from << 16;
    f->step = f->level / ticks;
    f->remaining = ticks;
    f->active = true;
}

bool wm_sound_fade_tick(wm_sound_fade_t *f, wm_sound_state_t *s) {
    if (!f || !f->active) return false;

    /* `SUB A9,A10 / MOVE A10,A0 / SRL 16,A0 / CALLR SET_LOWER_VOL`. */
    f->level -= f->step;
    if (f->level < 0) f->level = 0;
    if (s) s->master_volume = (uint8_t)((f->level >> 16) & 0xff);

    /* `SLEEPK 1 / DSJS A8,NEXT_FADE`, then a final `CLR A0` before
       the process dies -- so it really does reach silence rather
       than stopping one step short. */
    f->remaining -= 1;
    if (f->remaining > 0) return true;
    f->active = false;
    if (s) s->master_volume = 0;
    return false;
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

/* ------------------------------------------------------------------ */
/* channel_sound, clear_sound_ram, wrtable_sound                      */

wm_sound_result_t wm_sound_channel(wm_sound_state_t *s, int32_t index,
                                   unsigned channel) {
    wm_sound_result_t out;
    const wm_sound_entry_t *e;
    unsigned ch;

    memset(&out, 0, sizeof(out));
    if (!s) return out;
    /* `dec a1 / jrz #chan1 / ... ;error!` -- a channel outside 1-4
       falls off the end and does nothing. */
    if (channel < 1u || channel > WM_SOUND_CHANNELS) return out;
    if (index < 0 || (size_t)index >= wm_sound_table_count) return out;
    ch = channel - 1u;
    e = &wm_sound_table[(size_t)index];

    /*
     * No priority test of ANY kind -- "priorities notwithstanding",
     * says the source. The channel is taken whatever is on it.
     */
    s->duration[ch] = e->duration;
    s->priority[ch] = e->priority;
    s->call[ch] = (uint16_t)(e->call + ch);

    out.played = true;
    out.channel = (uint8_t)channel;
    out.duration = e->duration;
    out.call = s->call[ch];
    return out;
}

void wm_sound_clear_ram(wm_sound_state_t *s) {
    unsigned i;
    if (!s) return;
    /* `movi chan1ram,a1 / nos2 move a0,*a1+,W / cmpi chan4scp+32,a1`
       -- every channel's priority, duration, call and script pointer
       in one sweep. */
    for (i = 0; i < WM_SOUND_CHANNELS; ++i) {
        s->priority[i] = 0;
        s->duration[i] = 0;
        s->call[i] = 0;
    }
    memset(s->announcer_duration, 0, sizeof(s->announcer_duration));
    memset(s->announcer_channel, 0, sizeof(s->announcer_channel));
}

wm_sound_result_t wm_sound_wrtable(wm_sound_state_t *s, int wrestler_num,
                                   uint16_t index) {
    wm_sound_result_t out;
    uint16_t call;

    memset(&out, 0, sizeof(out));
    /* `sll 32-15,a0 / srl 32-15,a0` -- undo the |W_LOOKUP if present. */
    index = (uint16_t)(index & 0x7fffu);
    call = wm_wrsnd_lookup(wrestler_num, (int)index);
    /* `move a1,a0 / jrz #done` -- a zero on either table is silence. */
    if (call == 0) return out;
    return wm_sound_triple(s, (int32_t)call);
}

/* ------------------------------------------------------------------ */
/* ring_bell                                                          */

void wm_sound_bell_start(wm_sound_bell_t *b, wm_sound_state_t *s) {
    wm_sound_result_t r;
    if (!b) return;
    memset(b, 0, sizeof(*b));
    /* `movi bell_snd,a0 / callr triple_sound / sra 16,a14 / move
       a14,*a13(#BELL_CHANNEL),W` -- the first ring goes through the
       ordinary arbitration, and the channel it lands on is kept. */
    r = wm_sound_triple(s, WM_SOUND_BELL_CALL);
    b->channel = r.channel;
    b->rings_left = 2;
    b->sleep = WM_SOUND_BELL_GAP;
    b->active = true;
}

bool wm_sound_bell_tick(wm_sound_bell_t *b, wm_sound_state_t *s) {
    if (!b || !b->active) return false;
    if (--b->sleep > 0) return true;
    /* The second and third rings go onto the channel the first one
       took, whatever is there now. */
    (void)wm_sound_channel(s, WM_SOUND_BELL_CALL, b->channel);
    if (--b->rings_left <= 0) { b->active = false; return false; }
    b->sleep = WM_SOUND_BELL_GAP;
    return true;
}

/* ------------------------------------------------------------------ */
/* wmania_tune                                                        */

void wm_sound_tune_start(wm_sound_tune_t *t, wm_sound_state_t *s) {
    (void)s;
    if (!t) return;
    memset(t, 0, sizeof(*t));
    t->sleep = WM_SOUND_TUNE_GAP;
    t->active = true;
}

bool wm_sound_tune_tick(wm_sound_tune_t *t, uint16_t *out_call) {
    if (out_call) *out_call = 0;
    if (!t || !t->active) return false;
    if (--t->sleep > 0) return true;
    /* `SLEEP TSEC*8 / movi 14,a3 / SNDSND / SLEEP TSEC*8 / movi 13,a3
       / SNDSND / jruc #loop` -- the two alternate for ever. */
    if (out_call)
        *out_call = t->second ? WM_SOUND_TUNE_B : WM_SOUND_TUNE_A;
    t->second = !t->second;
    t->sleep = WM_SOUND_TUNE_GAP;
    return true;
}

/* ------------------------------------------------------------------ */
/* END_MATCH_SPEECH / PIN_HIM_PROC                                    */

/*
 * `WHICH_PIN_HIM .WORD 0D3H,0D4H,0D5H,0d3h,0D4H`. RNDRNG0 draws 0-3,
 * so the fifth entry is reachable only as the "one after" when the
 * draw at index 3 repeats the last call -- which is exactly what the
 * two trailing duplicates are there for.
 */
const uint16_t wm_sound_which_pin_him[5] = {
    0xD3, 0xD4, 0xD5, 0xD3, 0xD4
};

/* UTIL.ASM:1734 RNDPER, the same reading the rest of the port uses:
   RNDRNG0(999) and the event happens when the probability exceeds
   the draw. */
static bool sound_rndper(WmRng *rng, uint32_t per_mille) {
    if (!rng) return false;
    return wm_rng_rndrng0(rng, 999u) < per_mille;
}

bool wm_sound_pin_him_start(wm_sound_pin_him_t *p, WmRng *rng) {
    if (!p) return false;
    memset(p, 0, sizeof(*p));
    /* `movi 150,a0 / calla RNDPER / jals SUCIDE` -- most of the time
       the crowd says nothing at all. */
    if (!sound_rndper(rng, 150u)) return false;
    p->calls_left = WM_SOUND_PIN_HIM_CALLS;   /* `movk 8,a11` */
    p->active = true;
    return true;
}

bool wm_sound_pin_him_tick(wm_sound_pin_him_t *p, wm_sound_state_t *s,
                           WmRng *rng) {
    uint32_t pick;
    uint16_t call;
    wm_sound_result_t r;
    int32_t nap;

    if (!p || !p->active) return false;
    if (p->sleep > 0) { p->sleep -= 1; return true; }

    /* `movk 3,a0 / CALLA RNDRNG0` -- 0 to 3 inclusive. */
    pick = rng ? wm_rng_rndrng0(rng, 3u) : 0u;
    if (pick > 3u) pick = 3u;
    call = wm_sound_which_pin_him[pick];
    /*
     * `CMP A0,A9 / JRNE NO_NEED / MOVE *A1(010H),A0` -- a draw that
     * repeats the last call is replaced by the NEXT table entry, so
     * the chant never says the same line twice running.
     */
    if (call == p->last) call = wm_sound_which_pin_him[pick + 1u];
    p->last = call;

    r = wm_sound_triple(s, (int32_t)call);

    /*
     * `CLR A0 / MOVX A14,A0 / subk 20,a0 / CALLA PRCSLP` -- the low
     * half of triple_sound's answer is the duration, and the next
     * line starts twenty ticks before this one ends, so they overlap.
     * A refused call leaves a14 zero and the subtraction goes
     * negative; the source does not guard it and PRCSLP would treat
     * it as a very long sleep, so this floors at zero instead of
     * reproducing a hang.
     */
    nap = (int32_t)r.duration - 20;
    p->sleep = nap > 0 ? nap : 0;

    if (--p->calls_left <= 0) { p->active = false; return false; }
    return true;
}

void wm_sound_pin_him_kill(wm_sound_pin_him_t *p) {
    /* `MOVI PIN_HIM_PID,A0 / CLR A1 / NOT A1 / CALLA KILALL`. */
    if (p) p->active = false;
}
