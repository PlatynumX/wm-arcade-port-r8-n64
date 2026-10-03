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

    /* 38 until the extractor learned to read `#local` heads as well as
       global ones -- most of these tables are written that way -- and 64
       until it learned that `REFLONG a,b` is two longs and that several
       identical definitions of a name are not ambiguous. */
    assert(wm_roster_anim_table_count == 72);
    for (i = 0; i < wm_roster_anim_table_count; ++i) {
        const wm_roster_anim_table *t = &wm_roster_anim_tables[i];
        assert(t->name && t->file && t->row);
        assert(t->slots == 9 || t->slots == WM_ROSTER_ANIM_SLOTS);
        assert(t->columns == 1 || t->columns == 2);
        assert(t->kind == WM_ROSTER_COL_SLOT ? t->columns == 1
                                             : t->columns == 2);
        assert(t->line > 0);
        /* Bret is slot 0 and is never the missing one. */
        assert(t->row[0] != NULL);
    }
    assert(wm_roster_anim_find("no_such_tbl") == NULL);
    assert(wm_roster_anim_for(NULL, 0) == NULL);
    assert(wm_roster_anim_col(NULL, 0, 0) == NULL);
    assert(wm_roster_anim_facing(NULL, 0, 1) == NULL);
}

/*
 * mode_puppet's own table (DOINK.ASM:3594 `#stand_tbl`), which was not
 * here at all until two bugs in tools/wlrostertbl.py were fixed: it
 * OPENS with a two-label `REFLONG` row, which the extractor refused and
 * which broke the whole block; and its use site spells it
 * `FACE24TBL #stand_tbl`, which could not match a name kept with its
 * `#`, so even once read it was typed as a bare column pair instead of
 * a facing pair.
 *
 * Its contents are worth pinning beside wm_ani_init_rows, which carries
 * the same nine wrestlers' stand2/stand4 from each `*_ani_init`: the
 * two agree label for label, which is the check that says this is read
 * out of the source rather than derived from the wrestler prefix.
 */
static void test_the_puppet_watchdog_stand_table(void)
{
    const wm_roster_anim_table *t = wm_roster_anim_find("#stand_tbl");

    assert(t);
    assert(t->columns == 2);
    assert(t->kind == WM_ROSTER_COL_FACING);
    assert(t->slots == 9);              /* no Referee row */
    assert(strcmp(wm_roster_anim_facing(t, 0, 1), "hrt_stand2_anim") == 0);
    assert(strcmp(wm_roster_anim_facing(t, 0, 0), "hrt_stand4_anim") == 0);
    assert(strcmp(wm_roster_anim_facing(t, 3, 0), "yok_stand4_anim") == 0);
    assert(strcmp(wm_roster_anim_facing(t, 8, 1), "lex_stand2_anim") == 0);
    /* Adam Bomb's row is a real `.long 0,0`. */
    assert(wm_roster_anim_facing(t, WM_ROSTER_ANIM_ADAM_BOMB, 0) == NULL);
    assert(wm_roster_anim_facing(t, WM_ROSTER_ANIM_ADAM_BOMB, 1) == NULL);
}

/*
 * #faced_tbl is defined three times -- BAMSEQ3, DNKSEQ3, UNDSEQ3 -- and
 * the three agree, so the refusal rule has nothing to protect and it is
 * read. UNDSEQ3 writes it entirely with `.long` and packs the last
 * three entries onto one line, the other two use `REFLONG` with two
 * labels on some rows; the rows are compared after parsing, so the
 * spelling does not enter into it.
 */
static void test_identically_defined_tables_are_available(void)
{
    const wm_roster_anim_table *t = wm_roster_anim_find("#faced_tbl");

    assert(t);
    assert(t->columns == 1);
    assert(t->slots == 9);
    assert(strcmp(wm_roster_anim_for(t, 0), "hrt_break_face_anim") == 0);
    assert(strcmp(wm_roster_anim_for(t, 1), "rzr_break_face_anim") == 0);
    assert(strcmp(wm_roster_anim_for(t, 8), "lex_break_face_anim") == 0);
    assert(wm_roster_anim_for(t, WM_ROSTER_ANIM_ADAM_BOMB) == NULL);

    /* Sixteen identical definitions, and it comes in the same way. */
    assert(wm_roster_anim_find("#headheld_tbl") != NULL);
    /* A name whose definitions really DO conflict is still refused:
       REACT4.ASM carries #head_hit2 twice, with different rows. */
    assert(wm_roster_anim_find("#head_hit2") == NULL);
}

static void test_face24_tables_are_facing_pairs(void)
{
    /*
     * FACE24TBL (MACROS.H:65) scales WRESTLERNUM by X64 -- two longs
     * per wrestler -- then adds a long when MOVE_UP_BIT is *clear*. So
     * column 0 is the facing-up animation and column 1 the facing-down
     * one, and the source's own `_2_` / `_4_` naming lines up with
     * that.
     */
    static const char *const face24[6] = {
        "head_hit_tbl", "head_hit2_tbl", "head_hit_dizzy_tbl",
        "body_hit_tbl", "body_hit_dizzy_tbl", "knee_hit_tbl"
    };
    int i;

    for (i = 0; i < 6; ++i) {
        const wm_roster_anim_table *t = wm_roster_anim_find(face24[i]);
        assert(t);
        assert(t->columns == 2);
        assert(t->kind == WM_ROSTER_COL_FACING);
        /* Bret always has both. */
        assert(wm_roster_anim_facing(t, 0, 1) != NULL);
        assert(wm_roster_anim_facing(t, 0, 0) != NULL);
        /* Adam Bomb has neither, in every one of them. */
        assert(wm_roster_anim_facing(t, WM_ROSTER_ANIM_ADAM_BOMB, 1) == NULL);
        assert(wm_roster_anim_facing(t, WM_ROSTER_ANIM_ADAM_BOMB, 0) == NULL);
    }

    {
        const wm_roster_anim_table *t = wm_roster_anim_find("head_hit_tbl");
        assert(strcmp(wm_roster_anim_facing(t, 0, 1),
                      "hrt_2_head_hit_anim") == 0);
        assert(strcmp(wm_roster_anim_facing(t, 0, 0),
                      "hrt_4_head_hit_anim") == 0);
        /* wm_roster_anim_for is column 0, which is the facing-up one. */
        assert(wm_roster_anim_for(t, 0) == wm_roster_anim_facing(t, 0, 1));
    }

    /*
     * Three wrestlers have one head_hit2 animation for both facings --
     * Undertaker, Bam Bam and Doink -- where the other five have a
     * distinct pair. Reading only the `_2_` column would have got Bret
     * right and these three wrong in the other direction.
     */
    {
        const wm_roster_anim_table *t = wm_roster_anim_find("head_hit2_tbl");
        static const int shared[3] = { 2, 5, 6 };
        int k;

        for (k = 0; k < 3; ++k) {
            const char *up = wm_roster_anim_facing(t, shared[k], 1);
            const char *dn = wm_roster_anim_facing(t, shared[k], 0);
            assert(up && dn && strcmp(up, dn) == 0);
            assert(strstr(up, "_2_") == NULL && strstr(up, "_4_") == NULL);
        }
        assert(strcmp(wm_roster_anim_facing(t, 0, 1),
                      "hrt_2_head_hit2_anim") == 0);
        assert(strcmp(wm_roster_anim_facing(t, 0, 0),
                      "hrt_4_head_hit2_anim") == 0);
    }

    /*
     * head_hit_dizzy_tbl and body_hit_dizzy_tbl are the two the shipped
     * game never indexes: every FACE24TBL line naming them is commented
     * out. They are still two columns wide, and every row writes the
     * same label twice, so the column choice cannot be observed.
     */
    {
        static const char *const dizzy[2] = {
            "head_hit_dizzy_tbl", "body_hit_dizzy_tbl"
        };
        int k;
        int w;

        for (k = 0; k < 2; ++k) {
            const wm_roster_anim_table *t = wm_roster_anim_find(dizzy[k]);
            assert(t);
            for (w = 0; w < t->slots; ++w) {
                assert(wm_roster_anim_facing(t, w, 1) ==
                       wm_roster_anim_facing(t, w, 0));
            }
        }
    }
}

static void test_a_two_long_row_is_not_always_a_facing_pair(void)
{
    /*
     * PROGRESS.ASM's six `*_addr` tables are the same shape as the
     * FACE24TBL ones and mean something else entirely: the loop at
     * PROGRESS.ASM:3218 reads their two longs in turn and hands the
     * first to change_anim1a and the second to change_anim2a. Leg
     * animation and torso animation, not up and down.
     */
    static const char *const addrs[6] = {
        "taunting_addr", "standing_addr", "dead_addr",
        "running_addr", "clever_addr", "waiting_addr"
    };
    int i;

    for (i = 0; i < 6; ++i) {
        const wm_roster_anim_table *t = wm_roster_anim_find(addrs[i]);
        assert(t);
        assert(strcmp(t->file, "PROGRESS.ASM") == 0);
        assert(t->columns == 2);
        assert(t->kind == WM_ROSTER_COL_PAIR);
        /* Column 1 is always a torso animation, in every row that has
         * one -- which is what makes it not a facing. */
        {
            int w;
            for (w = 0; w < t->slots; ++w) {
                const char *torso = wm_roster_anim_col(t, w, 1);
                if (torso) {
                    assert(strstr(torso, "_torso") != NULL);
                }
            }
        }
    }

    {
        const wm_roster_anim_table *t = wm_roster_anim_find("taunting_addr");
        assert(strcmp(wm_roster_anim_col(t, 0, 0), "hrt_taunt4_anim") == 0);
        assert(strcmp(wm_roster_anim_col(t, 0, 1), "hrt_torso4_anim") == 0);
    }
}

static void test_the_directive_only_tables(void)
{
    /*
     * knee_hit_tbl (REACT3.ASM:223) puts eight `.ref` lines between its
     * label and its rows, and head_hit2_sand_tbl (REACT1.ASM:1746)
     * writes its rows as REFLONG rather than `.long`. Both are ordinary
     * tables; both were invisible until the reader learned that those
     * are declarations rather than the end of the data.
     */
    const wm_roster_anim_table *knee = wm_roster_anim_find("knee_hit_tbl");
    const wm_roster_anim_table *sand = wm_roster_anim_find("head_hit2_sand_tbl");

    assert(knee && knee->columns == 2);
    assert(strcmp(wm_roster_anim_facing(knee, 0, 1), "hrt_2_knee_hit_anim") == 0);
    assert(strcmp(wm_roster_anim_facing(knee, 0, 0), "hrt_4_knee_hit_anim") == 0);
    /* Doink has only the facing-up knee hit -- the `.ref` block names
     * dnk_2_knee_hit_anim and no dnk_4. */
    assert(wm_roster_anim_facing(knee, 6, 1) != NULL);

    assert(sand && sand->columns == 1);
    assert(strcmp(wm_roster_anim_for(sand, 0), "hrt_4_head_hit2s_anim") == 0);
    assert(wm_roster_anim_for(sand, WM_ROSTER_ANIM_ADAM_BOMB) == NULL);
    assert(strcmp(wm_roster_anim_for(sand, 8), "lex_4_head_hit2s_anim") == 0);
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
    test_the_puppet_watchdog_stand_table();
    test_identically_defined_tables_are_available();
    test_the_cut_wrestler_and_the_referee();
    test_yokozuna_has_no_turnbuckle_fall();
    test_the_six_climb_tables_are_all_there();
    test_face24_tables_are_facing_pairs();
    test_a_two_long_row_is_not_always_a_facing_pair();
    test_the_directive_only_tables();
    printf("global per-wrestler animation tables: all checks passed\n");
    return 0;
}
