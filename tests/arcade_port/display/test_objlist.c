/*
 * DISPLAY.ASM's object allocator and display lists, against the source's own
 * comparisons.
 *
 * The attract screens need this and the match does not: show_copyright creates
 * objects, leaves them up through a three-second button wait, then deletes an
 * ID class and builds a second page. Lifetime is the behaviour.
 */
#include "wm/arcade/wm_arcade_objlist.h"

#include <assert.h>
#include <stdio.h>

static int make(wm_obj_pool *p, int32_t z, int32_t y, int32_t id) {
    int h = wm_obj_get(p);
    assert(h != WM_OBJ_NONE);
    p->obj[h].z = z;
    p->obj[h].y = y;
    p->obj[h].id = id;
    p->obj[h].x = 1;          /* so DELOBJ's clear is observable */
    wm_obj_insert(p, h, WM_OBJ_LIST_FOREGROUND);
    return h;
}

static void test_pool_and_free_list(void) {
    wm_obj_pool p;
    int a, b;

    wm_obj_pool_init(&p);
    /* DISPLAY.EQU:96 NOBJ equ 350, and every block starts free. */
    assert(wm_obj_free_count(&p) == WM_OBJ_COUNT);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 0);

    a = wm_obj_get(&p);
    b = wm_obj_get(&p);
    assert(a != WM_OBJ_NONE && b != WM_OBJ_NONE && a != b);
    assert(wm_obj_free_count(&p) == WM_OBJ_COUNT - 2);

    /* GETOBJ hands back a blank block, not the last tenant's fields. */
    assert(p.obj[a].z == 0 && p.obj[a].id == 0 && p.obj[a].image == NULL);

    wm_obj_free(&p, a);
    wm_obj_free(&p, b);
    assert(wm_obj_free_count(&p) == WM_OBJ_COUNT);
    puts("objlist: pool and free list PASS");
}

/* The pool is a real budget: GETOBJ's failure path is CALLERR, not a resize. */
static void test_the_pool_runs_out(void) {
    wm_obj_pool p;
    int i, h;

    wm_obj_pool_init(&p);
    for (i = 0; i < WM_OBJ_COUNT; ++i) {
        assert(wm_obj_get(&p) != WM_OBJ_NONE);
    }
    h = wm_obj_get(&p);
    assert(h == WM_OBJ_NONE);
    assert(wm_obj_free_count(&p) == 0);
    puts("objlist: exhaustion returns WM_OBJ_NONE PASS");
}

/*
 * INSOBJ sorts by increasing z, then increasing y within equal z. Insert out
 * of order and the walk must come back ordered.
 */
static void test_sorted_insert(void) {
    wm_obj_pool p;
    int cur;
    int32_t last_z = -1, last_y = -1;

    wm_obj_pool_init(&p);
    /*
     * THREE at one z, inserted 10, 30, 20. Two would not do: with only a
     * pair, dropping the y tiebreak happens to give the same order, because
     * equal keys insert before and a two-element list comes out ascending
     * either way. The third element is what separates them -- without the
     * tiebreak this list comes out 20, 30, 10.
     */
    make(&p, 300, 10, 0);
    make(&p, 100, 10, 0);
    make(&p, 100, 30, 0);
    make(&p, 100, 20, 0);
    make(&p, 200, 20, 0);
    make(&p, 300, 5, 0);

    for (cur = wm_obj_first(&p, WM_OBJ_LIST_FOREGROUND);
         cur != WM_OBJ_NONE; cur = wm_obj_next(&p, cur)) {
        int32_t z = p.obj[cur].z, y = p.obj[cur].y;
        assert(z >= last_z);
        if (z == last_z) assert(y >= last_y);
        last_z = z;
        last_y = y;
    }
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 6);
    puts("objlist: sorted insert PASS");
}

/*
 * The subtle one. `jrgt #lp` fails on EQUALITY and falls into the insert, so
 * an object whose z and y both equal an existing one's goes in BEFORE it --
 * the opposite of a stable append. Getting this backwards reverses the draw
 * order of every same-depth pair, which on the copyright page is every line.
 */
static void test_equal_keys_insert_before(void) {
    wm_obj_pool p;
    int first, second, head;

    wm_obj_pool_init(&p);
    first = make(&p, 100, 10, 0);
    second = make(&p, 100, 10, 0);

    head = wm_obj_first(&p, WM_OBJ_LIST_FOREGROUND);
    assert(head == second);
    assert(wm_obj_next(&p, head) == first);
    puts("objlist: equal keys insert before PASS");
}

static void test_delete(void) {
    wm_obj_pool p;
    int a, b, c;

    wm_obj_pool_init(&p);
    a = make(&p, 100, 0, 0);
    b = make(&p, 200, 0, 0);
    c = make(&p, 300, 0, 0);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 3);

    assert(wm_obj_delete(&p, b));
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 2);
    assert(wm_obj_free_count(&p) == WM_OBJ_COUNT - 2);
    /* "Indicates not in use for collisions" -- DELOBJ clears OXPOS. */
    assert(p.obj[b].x == 0);
    /* Deleting something not on the list is delerr, not a crash. */
    assert(!wm_obj_delete(&p, b));

    assert(wm_obj_delete(&p, a));   /* head */
    assert(wm_obj_delete(&p, c));   /* tail */
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 0);
    assert(wm_obj_free_count(&p) == WM_OBJ_COUNT);
    puts("objlist: delete PASS");
}

/*
 * obj_del1c is obj_delc with mask 0 -- an exact OID match. The mask names
 * bits to IGNORE (`andn a1,a0`), so a non-zero mask widens the match.
 */
static void test_delete_class(void) {
    wm_obj_pool p;

    wm_obj_pool_init(&p);
    make(&p, 100, 0, 0x11);
    make(&p, 200, 0, 0x22);
    make(&p, 300, 0, 0x11);
    make(&p, 400, 0, 0x33);

    assert(wm_obj_delete_class(&p, 0x11, 0) == 2);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 2);
    assert(wm_obj_free_count(&p) == WM_OBJ_COUNT - 2);

    /*
     * The mask must actually widen the match, so the case has to be one an
     * exact compare would get WRONG. 0x33 and 0x11 both reduce to 0x11 once
     * the 0x22 bits are ignored, so masking deletes two where an exact
     * compare on 0x33 would delete one.
     */
    assert((0x33 & ~0x22) == 0x11);
    assert((0x11 & ~0x22) == 0x11);
    assert((0x22 & ~0x22) == 0x00);
    make(&p, 500, 0, 0x11);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 3);
    assert(wm_obj_delete_class(&p, 0x33, 0x22) == 2);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 1);
    /* What survives is the 0x22 one, which masks to 0x00 and never matched. */
    assert(p.obj[wm_obj_first(&p, WM_OBJ_LIST_FOREGROUND)].id == 0x22);
    puts("objlist: delete class PASS");
}

/* Consecutive matches must ALL go: the source loops back to #cmp, not #lp,
   so it does not skip the block after a deletion. */
static void test_consecutive_matches_all_deleted(void) {
    wm_obj_pool p;
    int i;

    wm_obj_pool_init(&p);
    for (i = 0; i < 5; ++i) make(&p, 100 + i, 0, 0x55);
    make(&p, 500, 0, 0x66);

    assert(wm_obj_delete_class(&p, 0x55, 0) == 5);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 1);
    assert(p.obj[wm_obj_first(&p, WM_OBJ_LIST_FOREGROUND)].id == 0x66);

    /* Including when the run starts at the head. */
    wm_obj_pool_init(&p);
    make(&p, 100, 0, 0x55);
    make(&p, 200, 0, 0x55);
    make(&p, 300, 0, 0x66);
    assert(wm_obj_delete_class(&p, 0x55, 0) == 2);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 1);
    puts("objlist: consecutive matches PASS");
}

/* The two lists are independent: INSBOBJ threads BAKLST, and DELOBJ only ever
   walks OBJLST. */
static void test_the_two_lists_are_separate(void) {
    wm_obj_pool p;
    int fg, bg;

    wm_obj_pool_init(&p);
    fg = wm_obj_get(&p);
    p.obj[fg].z = 100;
    wm_obj_insert(&p, fg, WM_OBJ_LIST_FOREGROUND);

    bg = wm_obj_get(&p);
    p.obj[bg].z = 50;
    wm_obj_insert(&p, bg, WM_OBJ_LIST_BACKGROUND);

    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 1);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_BACKGROUND) == 1);
    /* DELOBJ walks the foreground list only, so a background object is
       "not found" there -- the source's delerr. */
    assert(!wm_obj_delete(&p, bg));
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_BACKGROUND) == 1);
    /* obj_delc is foreground-only too. */
    assert(wm_obj_delete_class(&p, 0, 0) == 1);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_BACKGROUND) == 1);
    puts("objlist: the two lists are separate PASS");
}

/* A full create/hold/delete/rebuild cycle, the shape show_copyright uses. */
static void test_a_copyright_page_cycle(void) {
    wm_obj_pool p;
    int i;
    const int32_t TEXT_ID = 0x0301;   /* CLSNEUT|TYPTEXT|SUBTXT, shape only */
    const int32_t LOGO_ID = 0x0100;

    wm_obj_pool_init(&p);
    for (i = 0; i < 9; ++i) make(&p, 1803, 110 + i * 12, TEXT_ID);
    make(&p, 20000, 65, LOGO_ID);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 10);

    /* The logo sits at z=20000 and sorts last, behind nothing. */
    {
        int cur = wm_obj_first(&p, WM_OBJ_LIST_FOREGROUND), last = cur;
        while (wm_obj_next(&p, cur) != WM_OBJ_NONE) cur = wm_obj_next(&p, cur);
        last = cur;
        assert(p.obj[last].id == LOGO_ID);
    }

    /* obj_del1c CLSNEUT|TYPTEXT|SUBTXT: the nine lines go, the logo stays. */
    assert(wm_obj_delete_class(&p, TEXT_ID, 0) == 9);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 1);

    /* Page two builds from the same pool, with the blocks just returned. */
    for (i = 0; i < 10; ++i) make(&p, 1803, 110 + i * 12, TEXT_ID);
    assert(wm_obj_list_count(&p, WM_OBJ_LIST_FOREGROUND) == 11);
    assert(wm_obj_free_count(&p) == WM_OBJ_COUNT - 11);
    puts("objlist: a copyright page cycle PASS");
}

int main(void) {
    test_pool_and_free_list();
    test_the_pool_runs_out();
    test_sorted_insert();
    test_equal_keys_insert_before();
    test_delete();
    test_delete_class();
    test_consecutive_matches_all_deleted();
    test_the_two_lists_are_separate();
    test_a_copyright_page_cycle();
    puts("objlist: all checks passed");
    return 0;
}
