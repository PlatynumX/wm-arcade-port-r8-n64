/*
 * UTIL.ASM CYCLE_TABLE and the two high-score page cyclers.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_colcyc.h"

static void test_tables(void) {
    int i;
    assert(WM_COLCYC_TABLES == 2);
    assert(strcmp(wm_colcyc_tables[0].routine, "hscore_colcyc") == 0);
    assert(strcmp(wm_colcyc_tables[0].name, "COLTAB2") == 0);
    assert(strcmp(wm_colcyc_tables[0].palette, "BLUE") == 0);
    assert(strcmp(wm_colcyc_tables[1].routine, "hscore_colcyc2") == 0);
    assert(strcmp(wm_colcyc_tables[1].palette, "RUBYPAL") == 0);

    for (i = 0; i < WM_COLCYC_TABLES; ++i) {
        const wm_colcyc_table_t *t = &wm_colcyc_tables[i];
        size_t j;
        assert(t->word_count == 78);
        assert(t->period == 63);
        /* The period really is where word zero comes back. */
        assert(t->words[t->period] == t->words[0]);
        for (j = 1; j < t->period; ++j) assert(t->words[j] != t->words[0]);
        /* Nothing inside the cycle has bit 15 set, or the loop's
           JRN would restart early and the period would be a lie. */
        for (j = 0; j < t->period; ++j) assert((t->words[j] & 0x8000u) == 0);
        /* The tail is long enough for the seven-colour window. */
        assert(t->word_count - t->period >= WM_COLCYC_COLORS - 1);
    }
    /* The two tables are genuinely different palettes. */
    assert(wm_colcyc_tables[0].words[0] != wm_colcyc_tables[1].words[0]);
}

/* pal_find misses: sixty ticks, then look again. The first look is
   free. */
static void test_waits_for_the_palette(void) {
    wm_colcyc_t c;
    int i;
    wm_colcyc_begin(&c, &wm_colcyc_tables[0],
                    WM_COLCYC_START_COLOR, WM_COLCYC_COLORS);
    assert(c.phase == WM_COLCYC_WAIT_PAL);

    for (i = 0; i < WM_COLCYC_RETRY_TICKS; ++i) {
        assert(wm_colcyc_tick(&c, false, true));
        assert(!c.write);
        assert(c.phase == WM_COLCYC_WAIT_PAL);
    }
    /* Sixty ticks gone; the next tick finds it and writes at once. */
    assert(wm_colcyc_tick(&c, true, true));
    assert(c.phase == WM_COLCYC_RUN);
    assert(c.write);
    assert(c.src == &wm_colcyc_tables[0].words[0]);
}

/* Found straight away: the first tick writes. */
static void test_writes_every_three_ticks(void) {
    wm_colcyc_t c;
    const wm_colcyc_table_t *t = &wm_colcyc_tables[0];
    int i;
    unsigned writes = 0;
    size_t expect = 0;

    wm_colcyc_begin(&c, t, WM_COLCYC_START_COLOR, WM_COLCYC_COLORS);
    for (i = 0; i < 3 * 20; ++i) {
        assert(wm_colcyc_tick(&c, true, true));
        if (c.write) {
            /* The window steps one word at a time, in order. */
            assert(c.src == &t->words[expect]);
            expect = (expect + 1) % t->period;
            writes++;
        }
    }
    /* One write every three ticks, starting on the first. */
    assert(writes == 20);
}

/* A full pass wraps at the period, not at the end of the array. */
static void test_wraps_at_the_period(void) {
    wm_colcyc_t c;
    const wm_colcyc_table_t *t = &wm_colcyc_tables[1];
    int i;
    size_t highest = 0;

    wm_colcyc_begin(&c, t, WM_COLCYC_START_COLOR, WM_COLCYC_COLORS);
    /* Three full passes' worth of ticks. */
    for (i = 0; i < (int)(3 * t->period * WM_COLCYC_TICKS + 10); ++i) {
        assert(wm_colcyc_tick(&c, true, true));
        if (c.write) {
            size_t at = (size_t)(c.src - t->words);
            if (at > highest) highest = at;
        }
    }
    /* It never reads past the last word of the cycle, so the tail
       is only ever touched by the window, never by the cursor. */
    assert(highest == t->period - 1);
    assert(c.passes >= 3);

    /* And the window it hands over at the last position stays
       inside the array -- that is what the tail is for. */
    assert(t->period - 1 + WM_COLCYC_COLORS <= t->word_count);
}

/* KILL_US: the guard is checked before each write, and the process
   dies rather than writing into a reallocated palette. */
static void test_dies_when_the_palette_changes(void) {
    wm_colcyc_t c;
    int i;
    unsigned writes = 0;

    wm_colcyc_begin(&c, &wm_colcyc_tables[0],
                    WM_COLCYC_START_COLOR, WM_COLCYC_COLORS);
    for (i = 0; i < 10; ++i) {
        assert(wm_colcyc_tick(&c, true, true));
        if (c.write) writes++;
    }
    assert(writes > 0);

    /* The slot goes to somebody else. It dies on the next tick that
       would have written, not on the next tick. */
    for (i = 0; i < WM_COLCYC_TICKS; ++i) {
        if (!wm_colcyc_tick(&c, true, false)) break;
        assert(!c.write);
    }
    assert(c.phase == WM_COLCYC_DEAD);
    assert(!c.write);
    /* Dead stays dead. */
    assert(!wm_colcyc_tick(&c, true, true));
}

/* The guard is only consulted on a write tick, so a mid-sleep blip
   does not kill it -- the source only re-reads at the top of a pass. */
static void test_guard_is_read_at_the_top_of_a_pass(void) {
    wm_colcyc_t c;
    wm_colcyc_begin(&c, &wm_colcyc_tables[0],
                    WM_COLCYC_START_COLOR, WM_COLCYC_COLORS);
    assert(wm_colcyc_tick(&c, true, true));      /* writes */
    assert(c.write);
    /* The write tick also spends the first of the three sleep
       ticks, so two more pass with the guard down and nothing
       notices. */
    assert(wm_colcyc_tick(&c, true, false));
    assert(!c.write);
    assert(c.phase == WM_COLCYC_RUN);
    assert(wm_colcyc_tick(&c, true, false));
    assert(c.phase == WM_COLCYC_RUN);
    /* The next tick is the one that would write, and is where it
       notices. */
    assert(!wm_colcyc_tick(&c, true, false));
    assert(c.phase == WM_COLCYC_DEAD);
}

int main(void) {
    test_tables();
    test_waits_for_the_palette();
    test_writes_every_three_ticks();
    test_wraps_at_the_period();
    test_dies_when_the_palette_changes();
    test_guard_is_read_at_the_top_of_a_pass();
    printf("colcyc ok\n");
    return 0;
}
