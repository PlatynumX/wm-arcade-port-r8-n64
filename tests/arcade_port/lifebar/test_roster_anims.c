/*
 * The global per-wrestler animation tables.
 *
 * These are the "everyone does this, each in his own animation" tables:
 * falling back, being hit on the ground, hitting a block, climbing the
 * ropes. What is worth pinning is the two places where the data does
 * not match what the naming would suggest.
 */
#include "wm/arcade/wm_arcade_roster_anims.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_shape(void)
{
    int i;

    assert(wm_roster_anim_table_count == 18);
    for (i = 0; i < wm_roster_anim_table_count; ++i) {
        const wm_roster_anim_table *t = &wm_roster_anim_tables[i];
        assert(t->name && t->file && t->row);
        assert(t->slots == 9 || t->slots == WM_ROSTER_ANIM_SLOTS);
        assert(t->line > 0);
        /* Bret is slot 0 and is never the missing one. */
        assert(t->row[0] != NULL);
    }
    assert(wm_roster_anim_find("no_such_tbl") == NULL);
    assert(wm_roster_anim_for(NULL, 0) == NULL);
}

static void test_the_cut_wrestler_and_the_referee(void)
{
    /* REACT1.ASM's reaction tables are nine rows: Adam Bomb (slot 7) is
     * `.long 0` and there is no Referee slot at all. */
    const wm_roster_anim_table *fb = wm_roster_anim_find("fall_back_tbl");
    assert(fb && fb->slots == 9);
    assert(strcmp(wm_roster_anim_for(fb, 0), "hrt_fall_back_anim") == 0);
    assert(strcmp(wm_roster_anim_for(fb, 8), "lex_fall_back_anim") == 0);
    assert(wm_roster_anim_for(fb, WM_ROSTER_ANIM_ADAM_BOMB) == NULL);
    /* Asking a nine-row table for the Referee is out of range, not a
     * NULL entry -- the row does not exist. */
    assert(wm_roster_anim_for(fb, WM_ROSTER_ANIM_REFEREE) == NULL);

    /* The climb tables declare ten rows and fill both of those slots
     * with Doink's animation rather than leaving them empty -- so Adam
     * Bomb and the referee really can climb through the ropes. */
    {
        const wm_roster_anim_table *c =
            wm_roster_anim_find("climbthru_bot_anims");
        assert(c && c->slots == WM_ROSTER_ANIM_SLOTS);
        assert(strcmp(wm_roster_anim_for(c, 6), "dnk_climbthru_bot_anim") == 0);
        assert(strcmp(wm_roster_anim_for(c, WM_ROSTER_ANIM_ADAM_BOMB),
                      "dnk_climbthru_bot_anim") == 0);
        assert(strcmp(wm_roster_anim_for(c, WM_ROSTER_ANIM_REFEREE),
                      "dnk_climbthru_bot_anim") == 0);
    }

    /* hitonground_tbl declares ten rows and leaves both empty -- so the
     * row count and the contents really are two separate statements. */
    {
        const wm_roster_anim_table *h = wm_roster_anim_find("hitonground_tbl");
        assert(h && h->slots == WM_ROSTER_ANIM_SLOTS);
        assert(wm_roster_anim_for(h, WM_ROSTER_ANIM_ADAM_BOMB) == NULL);
        assert(wm_roster_anim_for(h, WM_ROSTER_ANIM_REFEREE) == NULL);
    }
}

static void test_yokozuna_has_no_turnbuckle_fall(void)
{
    /*
     * fall_back_tbukl_tbl gives every wrestler his own
     * *_fall_back_tbukl_anim -- except Yokozuna, who is pointed at the
     * ordinary yok_fall_back_anim. He has no turnbuckle fall of his
     * own, and no amount of deriving the name from the pattern would
     * have found that.
     */
    const wm_roster_anim_table *t = wm_roster_anim_find("fall_back_tbukl_tbl");
    int i;

    assert(t);
    assert(strcmp(wm_roster_anim_for(t, 3), "yok_fall_back_anim") == 0);
    for (i = 0; i < 9; ++i) {
        const char *a = wm_roster_anim_for(t, i);
        if (!a || i == 3) {
            continue;
        }
        assert(strstr(a, "_fall_back_tbukl_anim") != NULL);
    }

    /* And the plain table gives him one like everyone else, so this is
     * specific to the turnbuckle variant. */
    {
        const wm_roster_anim_table *p = wm_roster_anim_find("fall_back_tbl");
        assert(strcmp(wm_roster_anim_for(p, 3), "yok_fall_back_anim") == 0);
    }
}

static void test_the_six_climb_tables_are_all_there(void)
{
    static const char *const names[6] = {
        "climbthru_bot_anims", "climbthru_top_anims", "climbthru_side_anims",
        "climbin_bot_anims", "climbin_top_anims", "climbin_side_anims"
    };
    int i;
    int j;

    for (i = 0; i < 6; ++i) {
        const wm_roster_anim_table *t = wm_roster_anim_find(names[i]);
        assert(t);
        assert(strcmp(t->file, "WRESTLE2.ASM") == 0);
        assert(t->slots == WM_ROSTER_ANIM_SLOTS);
        /* Every slot filled -- these are the only tables with no gaps. */
        for (j = 0; j < WM_ROSTER_ANIM_SLOTS; ++j) {
            assert(wm_roster_anim_for(t, j) != NULL);
        }
    }
}

int main(void)
{
    test_shape();
    test_the_cut_wrestler_and_the_referee();
    test_yokozuna_has_no_turnbuckle_fall();
    test_the_six_climb_tables_are_all_there();
    printf("global per-wrestler animation tables: all checks passed\n");
    return 0;
}
