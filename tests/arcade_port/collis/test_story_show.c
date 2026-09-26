/*
 * STORIES.ASM:95 print_story / :168 show_wrestler_end_story -- the
 * pagination and the two countdowns that decide how long a page of
 * an ending story is on screen.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_story.h"

/* Run a story to completion with no button ever pressed and report
   the tick count and the pages it laid out. */
static unsigned run_to_end(const wm_story_t *story, unsigned *pages_out) {
    wm_story_player_t p;
    unsigned ticks = 0;
    wm_story_play_begin(&p, story);
    while (wm_story_play_tick(&p, 0u)) {
        ticks++;
        assert(ticks < 100000u);   /* it must terminate */
    }
    if (pages_out) *pages_out = p.pages_shown;
    return ticks;
}

/*
 * Twelve lines to a page: the cursor starts at 90, steps 12, and the
 * loop continues while it is below 230.
 */
static void test_pagination(void) {
    static const char *const lines[] = {
        "L00","L01","L02","L03","L04","L05","L06","L07","L08","L09",
        "L10","L11","L12","L13","L14","L15","L16","L17","L18","L19",
        "L20",
    };
    const wm_story_t story = { lines, 21 };
    wm_story_player_t p;
    int i;

    wm_story_play_begin(&p, &story);
    /* The first tick lays the page out and starts the minimum sleep;
       laying out costs no tick of its own. */
    assert(wm_story_play_tick(&p, 0u));
    assert(p.pages_shown == 1);
    assert(p.page_first == 0);
    assert(p.page_lines == WM_STORY_LINES_PER_PAGE);
    assert(!p.page_ended_short);
    for (i = 0; i < WM_STORY_LINES_PER_PAGE; ++i) {
        assert(p.page_y[i] == (int16_t)(WM_STORY_FIRST_Y +
                                        i * WM_STORY_LINE_STEP));
    }
    /* The twelfth line sits at 222; a thirteenth would be at 234. */
    assert(p.page_y[11] == 222);
    assert(WM_STORY_FIRST_Y + 12 * WM_STORY_LINE_STEP >= WM_STORY_Y_LIMIT);
}

/*
 * The exact tick budget of a 21-line story, straight out of the
 * source's SLEEPs:
 *
 *   page 0 is full   -> SLEEP TSEC*5 then the TSEC*15 button wait
 *   page 1 is short  -> the terminator jumps past both waits
 *   then SLEEPK TSEC/2
 */
static void test_tick_budget_short_last_page(void) {
    static const char *const lines[21] = {
        "a","b","c","d","e","f","g","h","i","j","k","l",
        "m","n","o","p","q","r","s","t","u"
    };
    const wm_story_t story = { lines, 21 };
    unsigned pages = 0;
    unsigned ticks = run_to_end(&story, &pages);

    assert(pages == 2);
    assert(ticks == (unsigned)(WM_STORY_MIN_TICKS + WM_STORY_WAIT_TICKS +
                               WM_STORY_TRAIL_TICKS));
}

/*
 * A story whose length is an exact multiple of twelve never hits the
 * terminator mid-page, so its LAST page is held by both waits too --
 * the asymmetry the header calls out.
 */
static void test_tick_budget_full_last_page(void) {
    static const char *const lines[24] = {
        "a","b","c","d","e","f","g","h","i","j","k","l",
        "m","n","o","p","q","r","s","t","u","v","w","x"
    };
    const wm_story_t story = { lines, 24 };
    unsigned pages = 0;
    unsigned ticks = run_to_end(&story, &pages);

    assert(pages == 2);
    assert(ticks == (unsigned)(2 * (WM_STORY_MIN_TICKS + WM_STORY_WAIT_TICKS) +
                               WM_STORY_TRAIL_TICKS));
    /* And it really is longer than the 21-line story above, despite
       being only three lines bigger. */
    {
        static const char *const shortr[21] = {
            "a","b","c","d","e","f","g","h","i","j","k","l",
            "m","n","o","p","q","r","s","t","u"
        };
        const wm_story_t s2 = { shortr, 21 };
        assert(ticks > run_to_end(&s2, NULL));
    }
}

/* A button skips the wait but not the minimum sleep. */
static void test_button_skips_only_the_wait(void) {
    static const char *const lines[13] = {
        "a","b","c","d","e","f","g","h","i","j","k","l","m"
    };
    const wm_story_t story = { lines, 13 };
    wm_story_player_t p;
    unsigned ticks = 0;

    wm_story_play_begin(&p, &story);
    /* Hold every button down from the first frame. */
    while (wm_story_play_tick(&p, 0xffu)) {
        ticks++;
        assert(ticks < 10000u);
    }
    /* The minimum sleep is still paid in full; the wait costs one
       tick, because SLEEPK 1 happens before the button is read. */
    assert(ticks == (unsigned)(WM_STORY_MIN_TICKS + 1 + WM_STORY_TRAIL_TICKS));
}

/* The erase flag is raised on exactly the frame a new page starts. */
static void test_erase_is_raised_once_per_page_turn(void) {
    static const char *const lines[25] = {
        "a","b","c","d","e","f","g","h","i","j","k","l",
        "m","n","o","p","q","r","s","t","u","v","w","x","y"
    };
    const wm_story_t story = { lines, 25 };
    wm_story_player_t p;
    unsigned erases = 0, ticks = 0;
    unsigned last_page = 0;

    wm_story_play_begin(&p, &story);
    while (wm_story_play_tick(&p, 0u)) {
        if (p.erase_lines) {
            erases++;
            /* An erase always coincides with a fresh page. */
            assert(p.pages_shown == last_page + 1);
        }
        last_page = p.pages_shown;
        assert(++ticks < 100000u);
    }
    /* 25 lines -> 12 + 12 + 1: three pages, two page turns. */
    assert(p.pages_shown == 3);
    assert(erases == 2);
}

/* which_player is one cell holding two words; PSTATUS >> 1 picks. */
static void test_winning_wrestler(void) {
    const int16_t which[2] = { 3, 5 };
    assert(wm_story_winning_wrestler(1, which) == 3);   /* p1 only */
    assert(wm_story_winning_wrestler(2, which) == 5);   /* p2 only */
    /* PSTATUS 3 shifts to 1 as well -- not clamped, just arithmetic. */
    assert(wm_story_winning_wrestler(3, which) == 5);
}

/* Slot 7 bails before anything is created. */
static void test_invalid_wrestler_does_nothing(void) {
    const int16_t which[2] = { WM_STORY_INVALID_WRESTLER, 0 };
    wm_story_end_t e;
    assert(!wm_story_end_begin(&e, 1, which));
    assert(e.phase == WM_STORY_END_DONE);
    assert(!e.build_scene);
    assert(!wm_story_end_tick(&e, 0u));
    /* And the source's own table agrees there was nothing to draw. */
    assert(wm_story_logos[WM_STORY_INVALID_WRESTLER] == NULL);
}

/*
 * The whole ending for Bret: 30 ticks of fade, his 21-line story, and
 * the caller's own fifteen-second wait -- which is what holds his
 * short second page up.
 */
static void test_end_sequence(void) {
    const int16_t which[2] = { 0, 0 };   /* Bret */
    wm_story_end_t e;
    unsigned ticks = 0, builds = 0, teardowns = 0;

    assert(wm_story_end_begin(&e, 1, which));
    assert(e.wrestler == 0);
    assert(e.phase == WM_STORY_END_FADE);

    while (wm_story_end_tick(&e, 0u)) {
        if (e.build_scene) builds++;
        if (e.tear_down) teardowns++;
        assert(++ticks < 100000u);
    }
    assert(builds == 1);
    assert(teardowns == 1);
    assert(e.phase == WM_STORY_END_DONE);

    /* Bret really is 21 lines, so the story part costs the short
       budget and the outer wait is paid on top. */
    assert(wm_wrestler_stories[0].stories[0].line_count == 21);
    assert(ticks == (unsigned)(WM_STORY_FADE_TICKS +
                               WM_STORY_MIN_TICKS + WM_STORY_WAIT_TICKS +
                               WM_STORY_TRAIL_TICKS +
                               WM_STORY_WAIT_TICKS));
}

/* Every real wrestler's ending terminates, and slot 7 is the only
   one that shows nothing. */
static void test_every_wrestler_ends(void) {
    int w;
    for (w = 0; w < WM_STORY_SLOTS; ++w) {
        const int16_t which[2] = { 0, 0 };
        wm_story_end_t e;
        unsigned ticks = 0;
        int16_t pick[2];
        pick[0] = (int16_t)w;
        pick[1] = 0;
        (void)which;
        if (w == WM_STORY_INVALID_WRESTLER) {
            assert(!wm_story_end_begin(&e, 1, pick));
            continue;
        }
        assert(wm_story_end_begin(&e, 1, pick));
        while (wm_story_end_tick(&e, 0u)) assert(++ticks < 200000u);
        /* At minimum: the fade, one page held, the tail, the wait. */
        assert(ticks >= (unsigned)(WM_STORY_FADE_TICKS +
                                   WM_STORY_TRAIL_TICKS +
                                   WM_STORY_WAIT_TICKS));
        assert(e.player.pages_shown >= 2);
    }
}

/* The logo is centred on (120,50) with the halving done in pixels. */
static void test_logo_centring(void) {
    int32_t x = 0, y = 0;
    wm_story_logo_xy(60, 20, &x, &y);
    assert(x == (int32_t)((120 - 30) << 16));
    assert(y == (int32_t)((50 - 10) << 16));

    /* An odd width loses its bottom bit before the shift, so the
       centre lands half a pixel right of true. */
    wm_story_logo_xy(61, 21, &x, &y);
    assert(x == (int32_t)((120 - 30) << 16));
    assert(y == (int32_t)((50 - 10) << 16));

    /* A logo wider than 240 goes negative rather than clamping. */
    wm_story_logo_xy(400, 0, &x, &y);
    assert(x == (int32_t)((uint32_t)(120 - 200) << 16));
}

int main(void) {
    test_pagination();
    test_tick_budget_short_last_page();
    test_tick_budget_full_last_page();
    test_button_skips_only_the_wait();
    test_erase_is_raised_once_per_page_turn();
    test_winning_wrestler();
    test_invalid_wrestler_does_nothing();
    test_end_sequence();
    test_every_wrestler_ends();
    test_logo_centring();
    printf("story show sequence ok\n");
    return 0;
}
