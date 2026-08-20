#include "wmania_hiscore_adapter.h"
#include "wmania_hiscore_counter.h"
#include "wmania_hiscore_entry.h"
#include "wmania_hiscore_factory.h"
#include "wmania_hiscore_persist.h"
#include "wmania_hiscore_present.h"
#include "wmania_hiscore_special.h"
#include "wmania_hiscore_system.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t fixed_random(void *user, uint32_t max_inclusive)
{
    (void)user;
    assert(max_inclusive == 6u);
    return 2u; /* SAL */
}

static void test_core_and_special(void)
{
    WmHsSystem sys;
    uint8_t abcde[5] = {'A','B','C','D','E'};
    uint16_t pos;

    wm_hs_system_init(&sys, 1234u);

    assert(sizeof(WmHsEntry) == 10u);
    assert(wm_hs_entry_score_bcd(&sys.streak[1]) == 0x00000011u);
    assert(wm_hs_entry_score_bcd(&sys.tag[1]) == 0x00009000u);
    assert(memcmp(sys.tag[1].initials, "BOON ", 5u) == 0);

    assert(wm_hs_defeated_icon_count(0x10101011u) == 5u);

    pos = wm_hs_add_special_mask_arcade(
        &sys.tables[WM_HS_TABLE_BEATEN], 0x00000001u, abcde);
    assert(pos != 0u);
    assert(wm_hs_entry_score_bcd(
        &sys.tables[WM_HS_TABLE_BEATEN].entries[pos]) == 0x00000001u);

    /* Same initials merge defeated masks instead of duplicating identity. */
    pos = wm_hs_add_special_mask_arcade(
        &sys.tables[WM_HS_TABLE_BEATEN], 0x00000010u, abcde);
    assert(pos != 0u);
    assert((wm_hs_entry_score_bcd(
        &sys.tables[WM_HS_TABLE_BEATEN].entries[pos]) &
        0x00000011u) == 0x00000011u);

    /* Deliberately awful tag time still follows source slot-17 fallback. */
    pos = wm_hs_add_tag_arcade(
        &sys.tables[WM_HS_TABLE_TAG], 0x99999999u, abcde);
    assert(pos == 17u);
    assert(wm_hs_entry_score_bcd(&sys.tag[17]) == 0x99999999u);
}

static void test_counter(void)
{
    WmHsResetCounter c;
    bool repaired = false;

    wm_hs_counter_init(&c, 1000u);
    assert(c.verifier == ~1000u);
    assert(wm_hs_counter_decrement(&c, 1000u) == 999u);

    wm_hs_counter_put(&c, 100u);
    assert(wm_hs_counter_delay(&c, 1000u) == 750u);

    c.verifier ^= 1u;
    assert(wm_hs_counter_get(&c, 1000u, &repaired) == 1000u);
    assert(repaired);
}

static void test_entry_ui(void)
{
    WmHsEntryState e;
    WmHsEntryInput in = {0};
    uint8_t out[5];
    uint32_t ev;

    wm_hs_entry_begin(&e, WM_HS_ENTRY_THREE_PLUS_WRESTLER,
                      0u, 4u, fixed_random, 0);
    assert(e.initials[0] == 'A');
    assert(wm_hs_entry_cursor_col(&e) == 0u);
    assert(wm_hs_entry_cursor_row(&e) == 0u);
    assert(wm_hs_entry_layout[0].grid_x == 37);
    assert(wm_hs_entry_layout[1].grid_x == 278);
    assert(wm_hs_entry_layout[0].initials_y == 204);

    /* Move right: preview becomes B. */
    in.stick_down = WM_HS_STICK_RIGHT;
    ev = wm_hs_entry_tick(&e, &in);
    assert((ev & WM_HS_ENTRY_EVENT_MOVE) != 0u);
    assert(e.cursor_index == 1u);
    assert(e.initials[0] == 'B');

    /* Commit B. The next slot is NUL until another cursor move. */
    in.stick_down = 0u;
    in.accept_down = true;
    ev = wm_hs_entry_tick(&e, &in);
    assert((ev & WM_HS_ENTRY_EVENT_ADD) != 0u);
    assert(e.committed == 1u);
    assert(e.initials[1] == 0u);

    /* Force END to finalize this short input. */
    in.accept_down = true;
    e.cursor_index = WM_HS_ENTRY_END_CELL;
    ev = wm_hs_entry_tick(&e, &in);
    assert((ev & WM_HS_ENTRY_EVENT_FINISHED) != 0u);
    wm_hs_entry_get_initials(&e, out);
    assert(out[3] == (uint8_t)('A' + 4u));

    /* Timeout preserves the source live-preview letter. */
    wm_hs_entry_begin(&e, WM_HS_ENTRY_FIVE, 0u, 0u, fixed_random, 0);
    e.timer_ticks = 1u;
    memset(&in, 0, sizeof(in));
    ev = wm_hs_entry_tick(&e, &in);
    assert((ev & WM_HS_ENTRY_EVENT_FINISHED) != 0u);
    assert((ev & WM_HS_ENTRY_EVENT_REPLACED) == 0u);
    assert(e.initials[0] == 'A');

    /* Dirty source word gets replaced from random_initials. */
    {
        uint8_t dirty[5] = {'S','H','I','T',0};
        assert(wm_hs_entry_is_dirty(dirty));
    }

    /* Empty entry replacement uses deterministic SAL in this test. */
    wm_hs_entry_begin(&e, WM_HS_ENTRY_FIVE, 0u, 0u, fixed_random, 0);
    memset(e.initials, 0, sizeof(e.initials));
    e.cursor_index = WM_HS_ENTRY_END_CELL;
    in.accept_down = true;
    ev = wm_hs_entry_tick(&e, &in);
    assert((ev & WM_HS_ENTRY_EVENT_REPLACED) != 0u);
    assert(memcmp(e.initials, "SAL", 3u) == 0);
}

static void test_hooks_and_presentation(void)
{
    WmHsSystem sys;
    WmHsPendingEntry p;
    WmHsDisplayRow rows[18];
    uint8_t inits[5] = {'T','E','S','T','!'};
    size_t n;
    char timebuf[32];

    wm_hs_system_init(&sys, 1000u);

    assert(wm_hs_begin_winstreak(&sys, 0u, 2u, 12u, &p));
    assert(p.typed_initial_count == 3u);
    assert(p.score_bcd == 0x00000012u);

    assert(!wm_hs_begin_pin_speed(&sys, 2u, 0u, 1u,
                                  0x00005000u, &p));
    assert(!wm_hs_begin_pin_speed(&sys, 0u, 0u, 3u,
                                  0x00005000u, &p));
    assert(wm_hs_begin_pin_speed(&sys, 0u, 0u, 1u,
                                 0x00005000u, &p));

    assert(wm_hs_begin_tag_time(&sys, 0u, 0x99999999u, &p));
    assert(wm_hs_commit_pending(&sys, &p, inits) == 17u);
    assert(sys.recent_index[WM_HS_TABLE_TAG] == 17u);

    n = wm_hs_present_rows(&sys, WM_HS_PRESENT_TAG, 1u,
                           rows, sizeof(rows)/sizeof(rows[0]));
    assert(n == 9u);
    assert(rows[0].physical_index == 1u);
    assert(rows[0].partner_physical_index == 2u);
    assert(rows[8].physical_index == 17u);
    assert(rows[8].partner_physical_index == 18u);
    assert(rows[8].highlighted);

    n = wm_hs_present_rows(&sys, WM_HS_PRESENT_STREAK, 1u,
                           rows, sizeof(rows)/sizeof(rows[0]));
    assert(n == 18u);
    assert(wm_hs_present_sequence[WM_HS_PRESENT_STREAK]
               .layout.second_initials_x == 213);
    assert(wm_hs_present_sequence[WM_HS_PRESENT_TAG]
               .layout.row_y_step == 23);

    assert(wm_hs_format_time(0x00006000u, timebuf, sizeof(timebuf)));
    assert(strcmp(timebuf, "60.0") == 0);
    {
        char tiny[2];
        assert(!wm_hs_format_time(0x00006000u, tiny, sizeof(tiny)));
    }
}

static void test_table_cmos_check(void)
{
    WmHsSystem sys;

    wm_hs_system_init(&sys, 1000u);
    sys.recent_index[WM_HS_TABLE_STREAK] = 2u;
    sys.recent_index[WM_HS_TABLE_PIN_SPEED] = 2u;
    sys.recent_index[WM_HS_TABLE_BEATEN] = 2u;
    sys.recent_index[WM_HS_TABLE_INTER] = 2u;
    sys.recent_index[WM_HS_TABLE_TAG] = 2u;

    /*
     * Corrupt enough streak entries to cross its source reset threshold (3).
     * Recomputed checksums are intentionally NOT supplied.
     */
    sys.streak[1].initials[0] = 'a';
    sys.streak[2].initials[0] = 'a';
    sys.streak[3].initials[0] = 'a';

    assert(wm_hs_system_table_cmos_check(&sys));
    assert(wm_hs_entry_score_bcd(&sys.streak[1]) == 0x00000011u);

    /* Exact INIT_HSTRING audit set: INTER survives this source routine. */
    assert(sys.recent_index[WM_HS_TABLE_STREAK] == 0u);
    assert(sys.recent_index[WM_HS_TABLE_PIN_SPEED] == 0u);
    assert(sys.recent_index[WM_HS_TABLE_BEATEN] == 0u);
    assert(sys.recent_index[WM_HS_TABLE_TAG] == 0u);
    assert(sys.recent_index[WM_HS_TABLE_INTER] == 2u);
}

static void test_persistence(void)
{
    WmHsSystem a, b;
    uint8_t buffer[WM_HS_SAVE_MAX_BYTES];
    size_t size = 0u;
    WmHsLoadResult r;

    wm_hs_system_init(&a, 900u);
    a.recent_index[WM_HS_TABLE_STREAK] = 3u;
    wm_hs_counter_put(&a.reset_counter, 777u);

    assert(wm_hs_save_encode(&a, buffer, sizeof(buffer), &size));
    assert(size == wm_hs_save_encoded_size());

    r = wm_hs_save_decode(&b, buffer, size, 900u);
    assert(r == WM_HS_LOAD_OK);
    assert(b.recent_index[WM_HS_TABLE_STREAK] == 3u);
    assert(b.reset_counter.value == 777u);
    assert(wm_hs_entry_score_bcd(&b.streak[1]) ==
           wm_hs_entry_score_bcd(&a.streak[1]));

    buffer[20] ^= 0x80u;
    r = wm_hs_save_decode(&b, buffer, size, 900u);
    assert(r == WM_HS_LOAD_RESET);
    assert(wm_hs_entry_score_bcd(&b.streak[1]) == 0x00000011u);
}

int main(void)
{
    test_core_and_special();
    test_counter();
    test_entry_ui();
    test_hooks_and_presentation();
    test_table_cmos_check();
    test_persistence();
    puts("wmania_hiscore_complete tests: PASS");
    return 0;
}
