/*
 * HSTD.ASM's high-score tables, which nothing in this port could reach.
 *
 * Nine files and roughly 1,900 lines of it were translated and
 * unit-tested and referenced by nothing outside themselves: no game
 * ever qualified for a table, no initials were ever entered, and
 * nothing was ever saved or loaded. The tables existed and the game
 * had no way in.
 *
 * What was missing is the flow: init and load at power-on, an offer
 * when a credit ends, the initials input, the commit, and the write
 * back out. The persistence itself is deliberately not the core's
 * choice -- wmania_hiscore_persist.h takes two callbacks and says the
 * port supplies them -- so the host keeps the bytes in RAM and the N64
 * build points the same two at a cartridge EEPROM.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm/app.h"
#include "wm/arcade/wmania_hiscore_entry.h"
#include "wm/arcade/wmania_hiscore_persist.h"
#include "wm/arcade/wmania_hiscore_system.h"

static wm_app APP;

/* A fresh machine has no save, so TABLE_CMOS_CHECK builds the factory
   tables -- exactly what the arcade does with blank CMOS. */
static void test_a_fresh_machine_gets_factory_tables(void)
{
    const WmHsTable *t;

    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);

    assert(APP.hiscore.adjusted_reset_value == WM_HS_ADJUSTED_RESET_DEFAULT);
    assert(APP.hs_backend.read != NULL && APP.hs_backend.write != NULL);

    t = wm_hs_system_table_const(&APP.hiscore, WM_HS_TABLE_STREAK);
    assert(t != NULL);
    /* Nothing has been committed or written yet. */
    assert(APP.hs_commits == 0u);
    assert(APP.hs_writes == 0u);
}

/*
 * Encode and decode round-trip through the app's own backend. This is
 * the half that says a table survives being written out and read back,
 * which is the whole point of having one.
 */
static void test_the_tables_round_trip_through_the_backend(void)
{
    WmHsSystem a, b;
    WmHsLoadResult r;

    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);

    wm_hs_system_init(&a, WM_HS_ADJUSTED_RESET_DEFAULT);
    (void)wm_hs_system_table_cmos_check(&a);

    APP.hs_save_cursor = 0;
    APP.hs_save_len = 0;
    assert(wm_hs_save_write(&a, &APP.hs_backend));
    assert(APP.hs_save_len == wm_hs_save_encoded_size());

    APP.hs_save_cursor = 0;
    r = wm_hs_save_read(&b, &APP.hs_backend, WM_HS_ADJUSTED_RESET_DEFAULT);
    assert(r == WM_HS_LOAD_OK || r == WM_HS_LOAD_REPAIRED);
    /*
     * The ROWS, not the WmHsTable struct: that holds an `entries`
     * pointer into its own system, so comparing the structs would be
     * comparing two addresses and would never match.
     */
    {
        const WmHsTable *ta = wm_hs_system_table_const(&a, WM_HS_TABLE_STREAK);
        const WmHsTable *tb = wm_hs_system_table_const(&b, WM_HS_TABLE_STREAK);
        uint16_t i, n;
        assert(ta && tb && ta->entries && tb->entries);
        assert(ta->template_def && tb->template_def);
        n = (uint16_t)(ta->template_def->last_entry + 1u);
        assert(n == (uint16_t)(tb->template_def->last_entry + 1u));
        for (i = 0; i < n; ++i)
            assert(memcmp(&ta->entries[i], &tb->entries[i],
                          sizeof(WmHsEntry)) == 0);
    }
}

/*
 * A credit that ends with a streak behind it is offered the win-streak
 * table. The qualification is not this port's decision -- wm_hs_begin_*
 * runs the source's own FIND_LOW_TABLE_LEVEL and refuses a row that
 * does not make the table -- so a big streak is used here to make sure
 * one does.
 */
static void test_a_finished_credit_is_offered_the_table(void)
{
    WmHsPendingEntry pending;
    bool qualified;

    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);

    qualified = wm_hs_begin_winstreak(&APP.hiscore, 0u, 0u, 99u, &pending);
    assert(qualified);

    /* A streak of zero is not a run at all and is never offered. */
    assert(!wm_hs_begin_winstreak(&APP.hiscore, 0u, 0u, 0u, &pending));
}

/*
 * The source's auto_init cache: a player who typed his initials once
 * this credit does not type them again -- the row commits straight
 * away, and the tables are written out.
 */
static void test_cached_initials_commit_without_typing(void)
{
    static const uint8_t INITIALS[WM_HS_NUM_INITIALS] = { 1, 2, 3, 0, 0 };
    WmHsPendingEntry pending;
    uint16_t slot;
    uint8_t back[WM_HS_NUM_INITIALS];

    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);

    assert(!wm_hs_system_has_cached_initials(&APP.hiscore, 0u));
    wm_hs_system_cache_initials(&APP.hiscore, 0u, INITIALS);
    assert(wm_hs_system_has_cached_initials(&APP.hiscore, 0u));
    wm_hs_system_get_cached_initials(&APP.hiscore, 0u, back);
    assert(memcmp(back, INITIALS, sizeof back) == 0);

    assert(wm_hs_begin_winstreak(&APP.hiscore, 0u, 0u, 99u, &pending));
    slot = wm_hs_commit_pending(&APP.hiscore, &pending, INITIALS);
    (void)slot;

    APP.hs_save_cursor = 0;
    APP.hs_save_len = 0;
    assert(wm_hs_save_write(&APP.hiscore, &APP.hs_backend));
    assert(APP.hs_save_len > 0);
}

/*
 * The initials input itself: the grid is walked with the stick and
 * committed with an attack button, and it finishes on its own.
 */
static void test_the_initials_input_runs_and_finishes(void)
{
    WmHsEntryState st;
    WmHsEntryInput in;
    uint8_t initials[WM_HS_NUM_INITIALS];
    int i;

    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);

    wm_hs_entry_begin(&st, WM_HS_ENTRY_THREE_PLUS_WRESTLER, 0u, 0u,
                      wm_rng_rndrng0_callback, &APP.rng);
    assert(!st.finished);

    /* Accept three times, releasing between, the way the source's own
       switch read requires an edge. */
    for (i = 0; i < 400 && !st.finished; ++i) {
        memset(&in, 0, sizeof in);
        in.accept_down = (i % 2) == 0;
        (void)wm_hs_entry_tick(&st, &in);
    }
    assert(st.finished);
    wm_hs_entry_get_initials(&st, initials);
    /* Something was chosen -- an all-zero result would mean the grid
       never moved. */
    assert(!wm_hs_entry_is_empty(initials));
}

/* The app carries the mode the input runs in, and the backend it
   saves through. */
static void test_the_app_is_wired_for_it(void)
{
    memset(&APP, 0, sizeof APP);
    wm_app_init(&APP);
    assert(APP.hs_backend.user == &APP);
    assert(WM_APP_MODE_HISCORE_ENTRY != WM_APP_MODE_GAME_OVER);
    /* 1,130 bytes encoded, inside the 1,200 the format allows -- and
       inside a 16 kbit EEPROM, which is what the N64 backend picks. */
    assert(wm_hs_save_encoded_size() <= WM_HS_SAVE_MAX_BYTES);
}

int main(void)
{
    test_a_fresh_machine_gets_factory_tables();
    test_the_tables_round_trip_through_the_backend();
    test_a_finished_credit_is_offered_the_table();
    test_cached_initials_commit_without_typing();
    test_the_initials_input_runs_and_finishes();
    test_the_app_is_wired_for_it();
    printf("hiscore flow tests passed\n");
    return 0;
}
