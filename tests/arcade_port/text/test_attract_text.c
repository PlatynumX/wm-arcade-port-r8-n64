/*
 * ATTRACT.ASM's own words, and the labels that used to stand for nothing.
 *
 * wm/arcade/wmania_attract_data.h carried each screen's geometry, its timing
 * and its per-line labels in the source's order. The labels resolved to
 * nothing: "copyright_ln1" appeared in exactly one place in the whole tree,
 * the array that declares it. So the copyright and AAMA screens had an exact
 * x, an exact y, a line step and a fade, and no text to print.
 */
#include "wm/arcade/wmania_attract_text.h"
#include "wm/arcade/wmania_attract_data.h"
#include "wm/arcade/wmania_attract_visuals.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Checked against ATTRACT.ASM by hand, and against it mechanically by
   tests/test_source_tools.py. Both pages' first and last lines plus the
   AAMA set, because those are the ones a wrong routine scope would move. */
static void test_the_text_is_the_sources(void) {
    assert(strcmp(wm_attract_text("copyright_ln1"),
                  "VIDEO GAME SOFTWARE DESIGNED AND DEVELOPED") == 0);
    assert(strcmp(wm_attract_text("copyright_ln9"),
                  "ALL RIGHTS RESERVED.") == 0);
    assert(strcmp(wm_attract_text("copyright_ln10"),
                  "'WRESTLEMANIA'") == 0);
    assert(strcmp(wm_attract_text("copyright_ln19"),
                  "BMG MUSIC PUBLISHING LTD./ALL BOYS MUSIC LTD.") == 0);

    assert(strcmp(wm_attract_text("aama_ln1"), "AAMA PARENTAL ADVISORY") == 0);
    assert(strcmp(wm_attract_text("aama_ln2"), "LIFE-LIKE VIOLENCE") == 0);
    assert(strcmp(wm_attract_text("aama_ln2b"), "- MILD") == 0);
    assert(strcmp(wm_attract_text("aama_ln5"), "COMBATIVE ACTIVITY.") == 0);
    puts("attract text: source strings PASS");
}

/*
 * The scoping check with teeth. Both routines number their lines #ln1.., and
 * `#` makes the label local, so a by-name lookup would resolve
 * "copyright_ln1" to aama_message's #ln1 -- "AAMA PARENTAL ADVISORY" would
 * head the copyright page. These two must not be equal, and neither may hold
 * the other's words.
 */
static void test_the_two_routines_did_not_collide(void) {
    const char *c1 = wm_attract_text("copyright_ln1");
    const char *a1 = wm_attract_text("aama_ln1");
    size_t i;

    assert(c1 && a1);
    assert(strcmp(c1, a1) != 0);
    assert(strstr(c1, "AAMA") == NULL);
    assert(strstr(a1, "MIDWAY") == NULL);

    /* And no copyright line anywhere is one of aama's six. */
    for (i = 0; i < wm_attract_text_count; ++i) {
        const wm_attract_text_entry *e = &wm_attract_text_entries()[i];
        if (strncmp(e->label, "copyright_", 10) == 0) {
            assert(strcmp(e->text, "AAMA PARENTAL ADVISORY") != 0);
            assert(strcmp(e->text, "- MILD") != 0);
        }
    }
    puts("attract text: routine scoping PASS");
}

/* Every label the screens declare must resolve. This is the assertion that
   would have failed before the extraction existed. */
static void test_every_declared_label_resolves(void) {
    size_t i;

    for (i = 0; i < WM_ATTRACT_COPYRIGHT_PAGE1_LINES; ++i) {
        const char *t = wm_attract_text(wm_attract_copyright_page1_labels[i]);
        assert(t && *t);
    }
    for (i = 0; i < WM_ATTRACT_COPYRIGHT_PAGE2_LINES; ++i) {
        const char *t = wm_attract_text(wm_attract_copyright_page2_labels[i]);
        assert(t && *t);
    }
    for (i = 0; i < 6u; ++i) {
        const char *t = wm_attract_text(wm_attract_aama_labels[i]);
        assert(t && *t);
    }
    assert(wm_attract_text_count == 25u);
    /* An unknown label answers NULL rather than inventing something. */
    assert(wm_attract_text("copyright_ln20") == NULL);
    assert(wm_attract_text(NULL) == NULL);
    puts("attract text: every declared label resolves PASS");
}

/* The placements must now carry the words, not just the coordinates. */
static void test_placements_carry_their_text(void) {
    WmAttractTextPlacement p[16];
    size_t n, i;

    n = wm_attract_copyright_page1_placements(p, 16u);
    assert(n == WM_ATTRACT_COPYRIGHT_PAGE1_LINES);
    for (i = 0; i < n; ++i) {
        assert(p[i].text && *p[i].text);
        assert(strcmp(p[i].text, wm_attract_text(p[i].source_label)) == 0);
        assert(p[i].x == WM_ATTRACT_COPYRIGHT_X);
        assert(p[i].y == (int16_t)(WM_ATTRACT_COPYRIGHT_FIRST_Y +
                                  (int)i * WM_ATTRACT_COPYRIGHT_LINE_STEP));
    }
    assert(strcmp(p[0].text, "VIDEO GAME SOFTWARE DESIGNED AND DEVELOPED") == 0);

    n = wm_attract_copyright_page2_placements(p, 16u);
    assert(n == WM_ATTRACT_COPYRIGHT_PAGE2_LINES);
    for (i = 0; i < n; ++i) assert(p[i].text && *p[i].text);
    assert(strcmp(p[0].text, "'WRESTLEMANIA'") == 0);

    n = wm_attract_aama_placements(p, 16u);
    assert(n == 6u);
    for (i = 0; i < n; ++i) assert(p[i].text && *p[i].text);
    /* ATTRACT.ASM puts the rating and its severity on ONE line, side by
       side: both at y=114, x 0xb1 and 0x104. */
    assert(p[1].y == p[2].y);
    assert(p[1].x < p[2].x);
    assert(strcmp(p[1].text, "LIFE-LIKE VIOLENCE") == 0);
    assert(strcmp(p[2].text, "- MILD") == 0);
    puts("attract text: placements carry text PASS");
}

int main(void) {
    test_the_text_is_the_sources();
    test_the_two_routines_did_not_collide();
    test_every_declared_label_resolves();
    test_placements_carry_their_text();
    puts("attract text: all checks passed");
    return 0;
}
