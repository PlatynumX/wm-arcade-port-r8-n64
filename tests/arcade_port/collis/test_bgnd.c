/*
 * BAKGND.ASM's background scroller: which blocks of the arena are on
 * the display list as the camera moves.
 */
#include <assert.h>
#include <string.h>

#include "wm_arcade_bgnd.h"

/* The ring, read out of BGNDTBL.ASM by tools/wlbgnd.py. */
static void test_the_ring_module(void) {
    const wm_bgnd_module_t *m;
    size_t i;

    assert(wm_bgnd_module_count >= 1);
    /* WRESTLE.ASM:1615 hands a match exactly one module. */
    assert(wm_bgnd_ring_mod_count == 1);
    m = wm_bgnd_ring_mod[0].module;
    assert(m && strcmp(m->name, "ringBMOD") == 0);
    assert(m->width == 1972 && m->height == 868);
    assert(m->block_count == 211);
    assert(m->header_count == 95);
    /* `.word 105,-450 ;x,y` */
    assert(wm_bgnd_ring_mod[0].x == 105);
    assert(wm_bgnd_ring_mod[0].y == -450);

    /*
     * SORTED BY X, and this is not decoration: the binary search and
     * the scan's early exit are both wrong without it, and they
     * would still return plausible-looking answers.
     */
    for (i = 1; i < m->block_count; ++i)
        assert(m->blocks[i].x >= m->blocks[i - 1].x);

    /* Every block's header index resolves, or it has no size. */
    for (i = 0; i < m->block_count; ++i) {
        uint16_t idx = (uint16_t)(m->blocks[i].hdr & 0x0fffu);
        if (m->blocks[i].hdr == 0xffffu) continue;
        assert(idx < m->header_count);
        assert(m->headers[idx].w > 0 && m->headers[idx].h > 0);
    }
}

/* BGND_UD1's framing: the screen plus DISP_PAD on every side. */
static void test_display_rect(void) {
    wm_bgnd_rect_t r = wm_bgnd_display_rect(1000, 500, 0, 0, 320, 240);
    assert(r.left == 1000 - WM_BGND_PAD_X);
    assert(r.top == 500 - WM_BGND_PAD_Y);
    assert(r.right == 1320 + WM_BGND_PAD_X);
    assert(r.bottom == 740 + WM_BGND_PAD_Y);
    /* [20h,40h] is Y:X, so the horizontal pad is the bigger one. */
    assert(WM_BGND_PAD_X == 0x40 && WM_BGND_PAD_Y == 0x20);
}

/* bgnd_get1stx, against a linear answer over the real table. */
static void test_get1stx(void) {
    const wm_bgnd_module_t *m = wm_bgnd_ring_mod[0].module;
    int32_t x;

    for (x = -400; x < 2200; x += 7) {
        size_t got = wm_bgnd_get1stx(m, x);
        size_t want = m->block_count;
        size_t i;
        for (i = 0; i < m->block_count; ++i)
            if ((int32_t)m->blocks[i].x >= x) { want = i; break; }
        assert(got == want);
    }
    /* Past the end is reported as the count, never as block zero --
       the source returns a bare 0 there and a caller that read it as
       an index would start from the left of the world every frame. */
    assert(wm_bgnd_get1stx(m, 100000) == m->block_count);
    assert(wm_bgnd_get1stx(m, -100000) == 0);
}

/* The scan must agree with a brute-force sweep over every block. */
static size_t brute(const wm_bgnd_placed_t *p, wm_bgnd_rect_t v,
                    uint8_t *want, size_t cap) {
    const wm_bgnd_module_t *m = p->module;
    size_t i, n = 0;
    memset(want, 0, cap);
    for (i = 0; i < m->block_count; ++i) {
        const wm_bgnd_block_t *b = &m->blocks[i];
        uint16_t idx = (uint16_t)(b->hdr & 0x0fffu);
        int32_t bx, by, w, h;
        if (b->hdr == 0xffffu || idx >= m->header_count) continue;
        w = m->headers[idx].w;
        h = m->headers[idx].h;
        bx = p->x + b->x;
        by = p->y + b->y;
        /* The ADD test: X+W >= left, X < right, Y+H >= top,
           Y < bottom. */
        if (bx + w >= v.left && bx < v.right &&
            by + h >= v.top && by < v.bottom) {
            want[i] = 1;
            ++n;
        }
    }
    return n;
}

static void test_scan_matches_brute_force(void) {
    static wm_bgnd_state_t st;
    static wm_bgnd_event_t ev[512];
    static uint8_t want[256];
    int32_t cam;

    for (cam = -600; cam < 2200; cam += 53) {
        wm_bgnd_rect_t v = wm_bgnd_display_rect(cam, 0, 0, 0, 320, 240);
        size_t n, i, expect;
        bool of = false;

        /* A fresh list every time, so this is the scan on its own
           rather than the scan plus whatever survived. */
        wm_bgnd_state_init(&st);
        n = wm_bgnd_scanmod(&st, wm_bgnd_ring_mod, wm_bgnd_ring_mod_count,
                            v, ev, 512, &of);
        assert(!of);
        expect = brute(&wm_bgnd_ring_mod[0], v, want, sizeof want);
        assert(n == expect);
        for (i = 0; i < n; ++i)
            assert(want[ev[i].index]);
        for (i = 0; i < sizeof want; ++i)
            if (want[i]) assert(wm_bgnd_is_resident(&st, i));
    }
}

/* And the scan really is cheap: it does not visit every block. */
static void test_the_scan_is_bounded(void) {
    static wm_bgnd_state_t st;
    static wm_bgnd_event_t ev[512];
    wm_bgnd_rect_t v = wm_bgnd_display_rect(900, 0, 0, 0, 320, 240);
    size_t n;

    wm_bgnd_state_init(&st);
    n = wm_bgnd_scanmod(&st, wm_bgnd_ring_mod, wm_bgnd_ring_mod_count,
                        v, ev, 512, NULL);
    /* A 320-wide window over a 1972-wide module: some of the ring is
       on screen and most of it is not. */
    assert(n > 0);
    assert(n < wm_bgnd_ring_mod[0].module->block_count);
}

/*
 * Adding and deleting do NOT use the same test, and the asymmetry is
 * in the shipped code: a block exactly on the right edge is not
 * added, but once it is resident it is not deleted either.
 */
static void test_add_and_delete_disagree_on_the_edge(void) {
    static const wm_bgnd_header_t hdr[] = { { 10, 10 } };
    static const wm_bgnd_block_t blk[] = { { 0, 100, 0, 0 } };
    static const wm_bgnd_module_t mod = {
        "edge", 200, 200, blk, 1, hdr, 1
    };
    static const wm_bgnd_placed_t list[] = { { &mod, 0, 0 } };
    wm_bgnd_state_t st;
    wm_bgnd_rect_t v;
    size_t n;

    /* right == the block's own X: the add refuses it. */
    v.left = 0; v.top = -50; v.right = 100; v.bottom = 50;
    wm_bgnd_state_init(&st);
    n = wm_bgnd_scanmod(&st, list, 1, v, NULL, 0, NULL);
    assert(n == 0);
    assert(!wm_bgnd_is_resident(&st, 0));

    /* One pixel wider and it goes on. */
    v.right = 101;
    n = wm_bgnd_scanmod(&st, list, 1, v, NULL, 0, NULL);
    assert(n == 1);
    assert(wm_bgnd_is_resident(&st, 0));

    /* Now shrink back to the exact edge: the DELETE test keeps it. */
    v.right = 100;
    n = wm_bgnd_delnonvis(&st, list, 1, v, NULL, 0, NULL);
    assert(n == 0);
    assert(wm_bgnd_is_resident(&st, 0));

    /* Past it, and it goes. */
    v.right = 99;
    n = wm_bgnd_delnonvis(&st, list, 1, v, NULL, 0, NULL);
    assert(n == 1);
    assert(!wm_bgnd_is_resident(&st, 0));
}

/* A whole update, and the residency that makes it a delta. */
static void test_update_is_a_delta(void) {
    static wm_bgnd_state_t st;
    static wm_bgnd_event_t added[512], removed[512];
    size_t na = 0, nr = 0, first;
    bool of = false;
    wm_bgnd_rect_t v = wm_bgnd_display_rect(600, 0, 0, 0, 320, 240);

    wm_bgnd_state_init(&st);
    wm_bgnd_update(&st, wm_bgnd_ring_mod, wm_bgnd_ring_mod_count, v,
                   added, 512, &na, removed, 512, &nr, &of);
    assert(!of);
    assert(na > 0 && nr == 0);
    first = st.resident_count;
    assert(first == na);

    /* The same camera again adds nothing: everything is already on
       the list, which is the whole point of BAKBITS. */
    wm_bgnd_update(&st, wm_bgnd_ring_mod, wm_bgnd_ring_mod_count, v,
                   added, 512, &na, removed, 512, &nr, &of);
    assert(na == 0 && nr == 0);
    assert(st.resident_count == first);

    /* Scroll a long way and the old blocks leave as the new arrive. */
    v = wm_bgnd_display_rect(1500, 0, 0, 0, 320, 240);
    wm_bgnd_update(&st, wm_bgnd_ring_mod, wm_bgnd_ring_mod_count, v,
                   added, 512, &na, removed, 512, &nr, &of);
    assert(nr > 0);
    assert(na > 0);
    assert(st.resident_count == first - nr + na);

    /* Off the module entirely: everything leaves. */
    v = wm_bgnd_display_rect(100000, 0, 0, 0, 320, 240);
    wm_bgnd_update(&st, wm_bgnd_ring_mod, wm_bgnd_ring_mod_count, v,
                   added, 512, &na, removed, 512, &nr, &of);
    assert(na == 0);
    assert(st.resident_count == 0);
}

/* A full array is reported rather than quietly losing a block. */
static void test_overflow_is_reported(void) {
    static wm_bgnd_state_t st;
    wm_bgnd_event_t one[1];
    wm_bgnd_rect_t v = wm_bgnd_display_rect(600, 0, 0, 0, 320, 240);
    size_t n;
    bool of = false;

    wm_bgnd_state_init(&st);
    n = wm_bgnd_scanmod(&st, wm_bgnd_ring_mod, wm_bgnd_ring_mod_count,
                        v, one, 1, &of);
    assert(of);
    /*
     * The COUNT is still the true one -- a caller with a short array
     * learns how many blocks moved even though it only got the first
     * one's details -- and residency is correct, so the set is not
     * damaged. What is lost is the notification, which is worth
     * saying plainly rather than hiding behind a clamped count.
     */
    assert(n > 1);
    assert(st.resident_count == n);
}

int main(void) {
    test_the_ring_module();
    test_display_rect();
    test_get1stx();
    test_scan_matches_brute_force();
    test_the_scan_is_bounded();
    test_add_and_delete_disagree_on_the_edge();
    test_update_is_a_delta();
    test_overflow_is_reported();
    return 0;
}
