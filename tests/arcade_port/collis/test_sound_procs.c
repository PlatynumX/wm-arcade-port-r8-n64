/*
 * LIFEBAR.ASM's ring_bell and AWARD.ASM's END_MATCH_SPEECH -- the two
 * SOUND_PID processes a match starts, and which nothing in this port
 * started.
 *
 * They are not one-shot calls, which is why they could not go through
 * the seam every other sound uses: each owns a channel for several
 * seconds and has its own state machine. And each had a second problem
 * on top of having no caller -- it arbitrated a channel through
 * triple_sound, kept the DURATION, and threw the CALL away, so even
 * wired it could not have made a sound.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/arcade/wm_arcade_sound.h"
#include "wm/match.h"

static wm_app APP;

static unsigned drain(wm_app *app, uint16_t *seen, unsigned max)
{
    wm_audio_event e;
    unsigned n = 0;
    while (wm_audio_pop_event(&app->audio, &e))
        if (n < max) seen[n++] = e.command;
    return n;
}

/*
 * The bell: three rings, a third of a second apart, the second and
 * third forced onto the first one's channel "to conserve tracks".
 * Every one of them now produces a call for the caller to send.
 */
static void test_the_bell_rings_three_times(void)
{
    wm_sound_state_t s;
    wm_sound_bell_t b;
    uint16_t call = 0;
    int i, calls = 0;

    wm_sound_init(&s);
    wm_sound_bell_start(&b, &s, &call);
    assert(b.active);
    assert(call != 0);
    ++calls;

    for (i = 0; i < 500 && b.active; ++i) {
        call = 0;
        wm_sound_update(&s);
        (void)wm_sound_bell_tick(&b, &s, &call);
        if (call) ++calls;
    }
    assert(!b.active);
    assert(calls == 3);
}

/*
 * And in a real app: starting a match rings it, and the calls reach
 * the audio queue. Before this the match started in silence.
 */
static void test_a_started_match_rings_the_bell(void)
{
    uint16_t seen[32];
    unsigned n;
    int i;
    uint16_t bell_call;

    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);
    bell_call = wm_sound_table[WM_SOUND_BELL_CALL].call;

    /* Run the app far enough into attract for a match to start. */
    for (i = 0; i < 4000 && !APP.bell.active; ++i) {
        wm_input_state in;
        memset(&in, 0, sizeof in);
        wm_app_tick(&APP, &in);
    }
    assert(APP.bell.active);

    n = drain(&APP, seen, 32);
    {
        unsigned k;
        bool found = false;
        for (k = 0; k < n; ++k)
            /* Channels 1-4 use call, call+1, call+2, call+3. */
            if (seen[k] >= bell_call && seen[k] <= (uint16_t)(bell_call + 3))
                found = true;
        assert(found);
    }
}

/*
 * The "PIN HIM!" chant. It only starts 150 times in 1000 -- `movi
 * 150,a0 / calla RNDPER / jals SUCIDE` -- and once running it never
 * says the same line twice in a row, which is what the two spare
 * entries at the end of its five-entry table are for.
 */
static void test_the_chant_never_repeats_itself(void)
{
    wm_sound_state_t s;
    wm_sound_pin_him_t p;
    WmRng rng;
    uint32_t t = 1;
    uint16_t seen[WM_SOUND_PIN_HIM_CALLS];
    int n = 0, i, tries;
    bool started = false;

    for (tries = 0; tries < 64 && !started; ++tries) {
        t = 1;
        wm_rng_init(&rng, 0x2222u + (uint32_t)tries, NULL, NULL, &t);
        started = wm_sound_pin_him_start(&p, &rng);
    }
    assert(started);

    wm_sound_init(&s);
    for (i = 0; i < 5000 && p.active; ++i) {
        uint16_t before = p.last, out = 0;
        wm_sound_update(&s);
        (void)wm_sound_pin_him_tick(&p, &s, &rng, &out);
        if (p.last != before && n < WM_SOUND_PIN_HIM_CALLS) {
            assert(out != 0);          /* the line is handed back */
            seen[n++] = p.last;
        }
    }
    assert(!p.active);
    assert(n >= 2);
    for (i = 1; i < n; ++i) assert(seen[i] != seen[i - 1]);
}

/* KILL_PIN_HIM (DCSSOUND.ASM:3989), which DNKSEQ2.ASM:5212's
   win_announce calls the moment a pin sticks. */
static void test_a_stuck_pin_stops_the_chant(void)
{
    wm_sound_pin_him_t p;
    WmRng rng;
    uint32_t t = 1;
    int tries;
    bool started = false;

    for (tries = 0; tries < 64 && !started; ++tries) {
        t = 1;
        wm_rng_init(&rng, 0x3333u + (uint32_t)tries, NULL, NULL, &t);
        started = wm_sound_pin_him_start(&p, &rng);
    }
    assert(started);
    wm_sound_pin_him_kill(&p);
    assert(!p.active);
}

/* The match carries the three seams, and an app binds all of them. */
static void test_the_match_is_wired_for_them(void)
{
    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);
    {
        wm_input_state in;
        int i;
        memset(&in, 0, sizeof in);
        for (i = 0; i < 4000 && APP.match.start_bell == NULL; ++i)
            wm_app_tick(&APP, &in);
    }
    assert(APP.match.start_bell != NULL);
    assert(APP.match.start_pin_him != NULL);
    assert(APP.match.kill_pin_him != NULL);
    assert(APP.match.sound_proc_user == &APP);
}

int main(void)
{
    test_the_bell_rings_three_times();
    test_the_chant_never_repeats_itself();
    test_a_stuck_pin_stops_the_chant();
    test_the_match_is_wired_for_them();
    test_a_started_match_rings_the_bell();
    printf("sound process tests passed\n");
    return 0;
}
