/*
 * DCSSOUND.ASM's four-channel mixer: triple_sound, snd_update and
 * announcer_sound, and the table they arbitrate on.
 */
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "wm_arcade_sound.h"
#include "wm/app.h"
#include "wm/match.h"
#include "wm_arcade_roster.h"
#include "wm/wrestler_sound_tables.h"

/* The table, read out of DCSSOUND.ASM:235 by tools/wlsound.py. */
static void test_table(void) {
    size_t i;
    int blank = 0;

    assert(wm_sound_table_count == 771);

    for (i = 0; i < wm_sound_table_count; ++i) {
        const wm_sound_entry_t *e = &wm_sound_table[i];
        /* sp_anncer is the highest group at 100<<8, so no priority
           can exceed 100 once the high byte is taken. */
        assert(e->priority <= 100);
        if (e->priority == 0 && e->duration == 0 && e->call == 0) ++blank;
    }
    /* The table really is sparse -- 104 of the 771 rows are the
       `.word 0,0` triple_sound skips. A reader that silently dropped
       them would renumber every index after the first hole, and the
       indices are what the whole game passes around. */
    assert(blank == 104);

    /* Three rows checked against the source by hand.
       DCSSOUND.ASM:237 `.word sp_smack|17,>80  ; 1 = face hit #0`:
       sp_smack is 16<<8, so priority 16, duration 17, call 0x80. */
    assert(wm_sound_table[1].priority == 16);
    assert(wm_sound_table[1].duration == 17);
    assert(wm_sound_table[1].call == 0x80);
    /* :239 `.word sp_system2|90,1480  ; 3 = combo earned sound`. */
    assert(wm_sound_table[3].priority == 40);
    assert(wm_sound_table[3].duration == 90);
    assert(wm_sound_table[3].call == 1480);
    /* :374 `.word sp_losmack|75-25,>500`, one of the seven rows with
       arithmetic in the operand: 15, 50, 0x500. */
    assert(wm_sound_table[0x76].priority == 15);
    assert(wm_sound_table[0x76].duration == 50);
    assert(wm_sound_table[0x76].call == 0x500);
}

/* The labels, and the shape they give the announcer ranges. */
static void test_announcer_ranges(void) {
    assert(wm_sound_triple_sndtab == 0);
    assert(wm_sound_announcer_start < wm_sound_vince_end);
    assert(wm_sound_vince_end < wm_sound_randy_end);
    assert(wm_sound_randy_end < wm_sound_howards_end);
    assert(wm_sound_howards_end < wm_sound_more_jerry);
    assert(wm_sound_triple_end == wm_sound_table_count);

    /* Below the table's announcer section: nobody. */
    assert(wm_sound_who_is_it(0) == WM_SOUND_ANNOUNCER_NONE);
    assert(wm_sound_who_is_it((int32_t)wm_sound_announcer_start - 1)
           == WM_SOUND_ANNOUNCER_NONE);
    assert(wm_sound_who_is_it((int32_t)wm_sound_announcer_start)
           == WM_SOUND_ANNOUNCER_VINCE);
    assert(wm_sound_who_is_it((int32_t)wm_sound_vince_end - 1)
           == WM_SOUND_ANNOUNCER_VINCE);
    assert(wm_sound_who_is_it((int32_t)wm_sound_vince_end)
           == WM_SOUND_ANNOUNCER_RANDY);
    assert(wm_sound_who_is_it((int32_t)wm_sound_randy_end)
           == WM_SOUND_ANNOUNCER_HOWARD);
    /*
     * The gap between Howard and Jerry's second block, which the
     * source itself calls bogus: "call # in that range between
     * howard and 2nd jerry".
     */
    assert(wm_sound_who_is_it((int32_t)wm_sound_howards_end)
           == WM_SOUND_ANNOUNCER_NONE);
    assert(wm_sound_who_is_it((int32_t)wm_sound_more_jerry - 1)
           == WM_SOUND_ANNOUNCER_NONE);
    /* And Jerry again on the far side of it. */
    assert(wm_sound_who_is_it((int32_t)wm_sound_more_jerry)
           == WM_SOUND_ANNOUNCER_RANDY);
    assert(wm_sound_who_is_it((int32_t)wm_sound_triple_end)
           == WM_SOUND_ANNOUNCER_NONE);
    assert(wm_sound_who_is_it(-1) == WM_SOUND_ANNOUNCER_NONE);
}

static uint32_t hcv;
static uint32_t hc(void *u) { (void)u; return (hcv += 0x139u) & 0x1ff; }
static uint32_t spf(void *u) { (void)u; return 0x01000000u + ((hcv * 7u) & 0x3fff); }

/* Find a row with a given priority, so the tests below use real
   indices rather than invented ones. */
static int32_t row_with(uint8_t pri) {
    size_t i;
    for (i = 1; i < wm_sound_table_count; ++i)
        if (wm_sound_table[i].priority == pri && wm_sound_table[i].call &&
            wm_sound_table[i].duration)
            return (int32_t)i;
    return -1;
}

static void test_four_channels(void) {
    wm_sound_state_t s;
    wm_sound_result_t r;
    int32_t idx = row_with(16);
    int i;

    assert(idx > 0);
    wm_sound_init(&s);

    /* Four free channels, taken in order. */
    for (i = 0; i < WM_SOUND_CHANNELS; ++i) {
        r = wm_sound_triple(&s, idx);
        assert(r.played);
        assert(r.channel == (uint8_t)(i + 1));
        /* Channels two to four use the next three consecutive calls;
           the table only stores channel one's. */
        assert(r.call == (uint16_t)(wm_sound_table[idx].call + i));
    }

    /* A fifth of the same priority TIES the lowest, and a tie
       preempts -- `cmp a5,a1 / jrlt #no_preempt` gives up only on a
       negative difference. Reading that as a strict > would drop
       most of the game's sounds, since the grunts and smacks share
       priorities. */
    r = wm_sound_triple(&s, idx);
    assert(r.played);
    assert(r.channel == 1);
}

static void test_priority_refuses(void) {
    wm_sound_state_t s;
    wm_sound_result_t r;
    int32_t loud = row_with(40), quiet = row_with(4);
    int i;

    assert(loud > 0 && quiet > 0);
    wm_sound_init(&s);

    /* Fill all four with the loud one. */
    for (i = 0; i < WM_SOUND_CHANNELS; ++i)
        assert(wm_sound_triple(&s, loud).played);

    /* The quiet one is outranked and is simply dropped. This is the
       behaviour that keeps a pile-up of grunts from burying the
       announcer. */
    r = wm_sound_triple(&s, quiet);
    assert(!r.played);

    /* Free one channel and it fits. */
    s.priority[2] = 0;
    s.duration[2] = 0;
    r = wm_sound_triple(&s, quiet);
    assert(r.played && r.channel == 3);
}

/* "find lowest-priority call and bump it" -- and the FIRST channel
   holding the minimum is the one bumped, because each step of the
   search is `jrge`, which skips a later channel that merely ties. */
static void test_lowest_and_first(void) {
    wm_sound_state_t s;
    wm_sound_result_t r;
    int32_t mid = row_with(20);

    assert(mid > 0);
    wm_sound_init(&s);
    s.priority[0] = 30; s.duration[0] = 50;
    s.priority[1] = 10; s.duration[1] = 50;
    s.priority[2] = 10; s.duration[2] = 50;
    s.priority[3] = 30; s.duration[3] = 50;

    r = wm_sound_triple(&s, mid);
    assert(r.played);
    assert(r.channel == 2);
}

static void test_refusals(void) {
    wm_sound_state_t s;

    wm_sound_init(&s);
    /* @SOUNDSUP -- and note the sense the name inverts: NON-zero
       means refuse. */
    s.suppressed = true;
    assert(!wm_sound_triple(&s, row_with(16)).played);
    s.suppressed = false;

    /* A negative index, one past the end, and a blank row are all
       ordinary no-ops rather than errors -- #a0lo, #a0hi and the
       zero-entry skip all reach triple_sound's success exit. */
    assert(!wm_sound_triple(&s, -1).played);
    assert(!wm_sound_triple(&s, (int32_t)wm_sound_table_count).played);
    assert(!wm_sound_triple(&s, 0).played);          /* `.word 0,0` */
    /* And none of them took a channel. */
    assert(s.priority[0] == 0);
}

/* snd_update: a channel is busy until its duration runs out. */
static void test_update_frees_channels(void) {
    wm_sound_state_t s;
    int32_t idx = row_with(16);
    uint8_t dur;
    int i;

    assert(idx > 0);
    dur = wm_sound_table[idx].duration;
    assert(dur > 1);

    wm_sound_init(&s);
    assert(wm_sound_triple(&s, idx).played);
    assert(s.priority[0] != 0);

    for (i = 0; i < (int)dur - 1; ++i) {
        wm_sound_update(&s);
        assert(s.priority[0] != 0);
    }
    wm_sound_update(&s);
    assert(s.priority[0] == 0);
    assert(s.call[0] == 0);

    /* An idle mixer does not underflow. */
    for (i = 0; i < 10; ++i) wm_sound_update(&s);
    assert(s.duration[0] == 0);
}

/*
 * announcer_sound: "if he's already saying something, the new call
 * cuts off the old one", and the cut-off does NOT go through the
 * priority arbitration -- an announcer cannot lose an argument with
 * himself.
 */
static void test_announcer_cuts_himself_off(void) {
    wm_sound_state_t s;
    wm_sound_result_t first, second;
    int32_t a = -1, b = -1;
    size_t i;
    int j;

    /* Two of Vince's own lines. */
    for (i = wm_sound_announcer_start; i < wm_sound_vince_end; ++i) {
        if (!wm_sound_table[i].call || !wm_sound_table[i].duration) continue;
        if (a < 0) a = (int32_t)i;
        else { b = (int32_t)i; break; }
    }
    assert(a > 0 && b > 0);

    wm_sound_init(&s);
    first = wm_sound_announcer(&s, a);
    assert(first.played);
    assert(s.announcer_channel[WM_SOUND_ANNOUNCER_VINCE] == first.channel);
    assert(s.announcer_duration[WM_SOUND_ANNOUNCER_VINCE] == first.duration);

    /* Fill the other three channels with the loudest thing there is,
       so ordinary arbitration would refuse him outright. */
    for (j = 0; j < WM_SOUND_CHANNELS; ++j)
        if (j != first.channel - 1) {
            s.priority[j] = 100;
            s.duration[j] = 200;
        }

    second = wm_sound_announcer(&s, b);
    assert(second.played);
    /* Same track, as the source says. */
    assert(second.channel == first.channel);
    assert(s.announcer_duration[WM_SOUND_ANNOUNCER_VINCE] == second.duration);

    /* A non-announcer index is refused outright rather than played. */
    assert(!wm_sound_announcer(&s, 1).played);
}

/*
 * Every one of the 326 announcer lines is priority 100 -- sp_anncer
 * is the top group in the table (DCSSOUND.ASM:221, `equ 100 << 8`)
 * and not one row in the four announcer spans uses anything else.
 * With ties preempting, that means a silent announcer always gets a
 * channel: he can be talked over by nobody.
 *
 * Only two rows outside those spans reach 100, and both are things
 * that have to be heard over a match: :315 the buy-in sound and
 * :440 the round-start bell.
 */
static void test_the_announcer_outranks_everything(void) {
    wm_sound_state_t s;
    size_t i;
    int32_t vince = -1;
    int lines = 0, top = 0, elsewhere = 0;
    int j;

    for (i = wm_sound_announcer_start; i < wm_sound_triple_end; ++i) {
        bool is_announcer = wm_sound_who_is_it((int32_t)i)
                            != WM_SOUND_ANNOUNCER_NONE;
        /* Skip only a wholly blank row; 18 announcer rows carry a
           priority with no call or no duration and are still his. */
        if (wm_sound_table[i].call == 0 && wm_sound_table[i].duration == 0 &&
            wm_sound_table[i].priority == 0)
            continue;
        if (is_announcer) {
            ++lines;
            if (wm_sound_table[i].priority == 100) ++top;
            if (vince < 0 &&
                wm_sound_who_is_it((int32_t)i) == WM_SOUND_ANNOUNCER_VINCE)
                vince = (int32_t)i;
        }
    }
    assert(lines == 326);
    assert(top == lines);

    for (i = 0; i < wm_sound_table_count; ++i)
        if (wm_sound_table[i].priority == 100 &&
            wm_sound_who_is_it((int32_t)i) == WM_SOUND_ANNOUNCER_NONE)
            ++elsewhere;
    assert(elsewhere == 2);

    /* Four channels of the loudest thing in the game, and he still
       gets in -- because a tie preempts. */
    assert(vince > 0);
    wm_sound_init(&s);
    for (j = 0; j < WM_SOUND_CHANNELS; ++j) {
        s.priority[j] = 100;
        s.duration[j] = 200;
    }
    assert(wm_sound_announcer(&s, vince).played);
    assert(s.announcer_duration[WM_SOUND_ANNOUNCER_VINCE] != 0);
}

/* ---- channel_sound: the channel is chosen, not arbitrated -------- */

static void test_channel_sound(void) {
    wm_sound_state_t s;
    wm_sound_result_t r;
    int32_t loud = row_with(100), quiet = row_with(4);
    int i;

    assert(loud > 0 && quiet > 0);
    wm_sound_init(&s);
    for (i = 0; i < WM_SOUND_CHANNELS; ++i)
        assert(wm_sound_triple(&s, loud).played);

    /* triple_sound would refuse this outright. */
    assert(!wm_sound_triple(&s, quiet).played);

    /* channel_sound takes the channel anyway -- "priorities
       notwithstanding". */
    r = wm_sound_channel(&s, quiet, 3);
    assert(r.played && r.channel == 3);
    assert(s.priority[2] == wm_sound_table[quiet].priority);
    assert(s.call[2] == (uint16_t)(wm_sound_table[quiet].call + 2));

    /* A channel outside 1-4 does nothing at all. */
    assert(!wm_sound_channel(&s, quiet, 0).played);
    assert(!wm_sound_channel(&s, quiet, 5).played);
}

/* ring_bell: three rings, and the second and third go onto the
   channel the FIRST one took. */
static void test_ring_bell(void) {
    wm_sound_state_t s;
    wm_sound_bell_t b;
    uint8_t ch;
    int i, rings = 1;

    wm_sound_init(&s);
    wm_sound_bell_start(&b, &s);
    assert(b.active);
    ch = b.channel;
    assert(ch >= 1 && ch <= WM_SOUND_CHANNELS);
    /* The bell is 0xB1, one of the two non-announcer rows at
       priority 100 -- it has to be heard over the match. */
    assert(wm_sound_table[WM_SOUND_BELL_CALL].priority == 100);

    for (i = 0; i < 500 && b.active; ++i) {
        wm_sound_update(&s);
        if (wm_sound_bell_tick(&b, &s)) {
            /* nothing */
        }
        if (s.call[ch - 1] ==
            (uint16_t)(wm_sound_table[WM_SOUND_BELL_CALL].call + ch - 1) &&
            s.duration[ch - 1] == wm_sound_table[WM_SOUND_BELL_CALL].duration)
            ++rings;
    }
    assert(!b.active);
    /* Three in total, a third of a second apart. */
    assert(rings >= 3);
    assert(i >= 2 * WM_SOUND_BELL_GAP);
}

/* wmania_tune: three raw calls on an eight-second loop, and not a
   tune script despite the name. */
static void test_wmania_tune(void) {
    wm_sound_tune_t t;
    uint16_t call;
    int i, a = 0, b = 0;

    wm_sound_tune_start(&t, NULL);
    for (i = 0; i < WM_SOUND_TUNE_GAP * 4 + 4; ++i) {
        assert(wm_sound_tune_tick(&t, &call));
        if (call == WM_SOUND_TUNE_A) ++a;
        if (call == WM_SOUND_TUNE_B) ++b;
    }
    /* They alternate, and it never stops. */
    assert(a >= 2 && b >= 2);
    assert(a == b || a == b + 1);
    assert(t.active);
}

/*
 * PIN_HIM_PROC: eight calls, and the rule that gives the chant its
 * character -- a draw repeating the LAST call is replaced by the
 * next table entry, so it never says the same line twice running.
 */
static void test_pin_him_never_repeats(void) {
    wm_sound_state_t s;
    wm_sound_pin_him_t p;
    WmRng rng;
    uint32_t t = 1;
    uint16_t seen[WM_SOUND_PIN_HIM_CALLS];
    int n = 0, i, started = 0, tries;

    /* The table's two trailing duplicates exist only to be the "one
       after" for the last two draws. */
    assert(wm_sound_which_pin_him[3] == wm_sound_which_pin_him[0]);
    assert(wm_sound_which_pin_him[4] == wm_sound_which_pin_him[1]);

    /* It only runs 150 times in 1000, so try until one starts. */
    for (tries = 0; tries < 200 && !started; ++tries) {
        wm_rng_init(&rng, 0x1234u + (uint32_t)tries, hc, spf, &t);
        started = wm_sound_pin_him_start(&p, &rng);
    }
    assert(started);

    wm_sound_init(&s);
    for (i = 0; i < 5000 && p.active; ++i) {
        uint16_t before = p.last;
        wm_sound_update(&s);
        wm_sound_pin_him_tick(&p, &s, &rng);
        if (p.last != before && n < WM_SOUND_PIN_HIM_CALLS)
            seen[n++] = p.last;
    }
    assert(!p.active);
    assert(n >= 2);
    for (i = 1; i < n; ++i)
        assert(seen[i] != seen[i - 1]);

    /* KILL_PIN_HIM stops it dead. */
    wm_rng_init(&rng, 0x99u, hc, spf, &t);
    for (tries = 0; tries < 200; ++tries)
        if (wm_sound_pin_him_start(&p, &rng)) break;
    if (p.active) {
        wm_sound_pin_him_kill(&p);
        assert(!p.active);
    }
}

/* wrtable_sound: the per-wrestler lookup joined to the mixer. */
static void test_wrtable_sound(void) {
    wm_sound_state_t s;
    int w, m;
    int played = 0;

    wm_sound_init(&s);
    for (w = 0; w < 8 && played < 3; ++w)
        for (m = 0; m < 8 && played < 3; ++m) {
            uint16_t idx = wm_wrsnd_lookup(w, m);
            wm_sound_result_t r;
            wm_sound_init(&s);
            r = wm_sound_wrtable(&s, w, (uint16_t)m);
            if (idx == 0) { assert(!r.played); continue; }
            if (!r.played) continue;
            /* What it played is that wrestler's own table row. */
            assert(r.call == wm_sound_table[idx].call);
            ++played;
        }
    assert(played > 0);

    /* The W_LOOKUP bit is stripped, so 8000h|m is the same as m. */
    {
        wm_sound_state_t a, b;
        wm_sound_result_t ra, rb;
        wm_sound_init(&a);
        wm_sound_init(&b);
        ra = wm_sound_wrtable(&a, 0, 0u);
        rb = wm_sound_wrtable(&b, 0, (uint16_t)(0x8000u | 0u));
        assert(ra.played == rb.played && ra.call == rb.call);
    }
}

/* nosounds / clear_sound_ram. */
static void test_clear_ram(void) {
    wm_sound_state_t s;
    int i;
    wm_sound_init(&s);
    for (i = 0; i < WM_SOUND_CHANNELS; ++i)
        assert(wm_sound_triple(&s, row_with(16)).played);
    wm_sound_clear_ram(&s);
    for (i = 0; i < WM_SOUND_CHANNELS; ++i) {
        assert(s.priority[i] == 0);
        assert(s.duration[i] == 0);
        assert(s.call[i] == 0);
    }
}

/* ---- and the same mixer in a live app ---------------------------- */

static wm_app A;

/*
 * The one seam every in-match sound arrives through carries a
 * triple_sndtab INDEX. Before the mixer existed that index went into
 * the audio queue unchanged -- so the platform was handed a table row
 * number where a DCS sound call belonged, and every sound was queued
 * regardless of the four channels.
 */
static void test_the_app_queues_calls_not_indices(void) {
    wm_audio_event ev;
    int32_t idx = row_with(16);
    const wm_sound_entry_t *e;

    assert(idx > 0);
    e = &wm_sound_table[idx];
    /* The two really are different numbers, or this proves nothing. */
    assert(e->call != (uint16_t)idx);

    memset(&A, 0, sizeof A);
    hcv = 1;
    wm_app_init(&A);
    wm_rng_init(&A.rng, 0x12345678u, hc, spf, NULL);
    wm_match_init(&A.match);
    A.match.anim_sound_user = &A;
    A.match.anim_sound = NULL;   /* set by wm_app_bind_anim_env */

    /* Reach the seam the way the match does. */
    {
        wm_sound_result_t r = wm_sound_triple(&A.sound, idx);
        assert(r.played);
        assert(wm_audio_send_command(&A.audio, r.call));
    }
    assert(wm_audio_pop_event(&A.audio, &ev));
    assert(ev.command == e->call);
}

/*
 * And the app ticks snd_update, without which the four channels fill
 * up once and nothing is ever heard again.
 */
static void test_the_app_frees_channels(void) {
    int32_t idx = row_with(16);
    int i;

    memset(&A, 0, sizeof A);
    hcv = 1;
    wm_app_init(&A);
    assert(wm_sound_triple(&A.sound, idx).played);
    assert(A.sound.priority[0] != 0);

    for (i = 0; i < 300 && A.sound.priority[0] != 0; ++i)
        wm_app_tick(&A, NULL);
    assert(A.sound.priority[0] == 0);
    assert(i <= wm_sound_table[idx].duration);
}

int main(void) {
    test_table();
    test_announcer_ranges();
    test_four_channels();
    test_priority_refuses();
    test_lowest_and_first();
    test_refusals();
    test_update_frees_channels();
    test_announcer_cuts_himself_off();
    test_the_announcer_outranks_everything();
    test_channel_sound();
    test_ring_bell();
    test_wmania_tune();
    test_pin_him_never_repeats();
    test_wrtable_sound();
    test_clear_ram();
    test_the_app_queues_calls_not_indices();
    test_the_app_frees_channels();
    return 0;
}
