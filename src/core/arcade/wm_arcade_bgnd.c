/*
 * BAKGND.ASM's background scroller -- see wm/arcade/wm_arcade_bgnd.h.
 */
#include "wm/arcade/wm_arcade_bgnd.h"

#include <string.h>

void wm_bgnd_state_init(wm_bgnd_state_t *s) {
    if (s) memset(s, 0, sizeof(*s));
}

bool wm_bgnd_is_resident(const wm_bgnd_state_t *s, size_t block) {
    if (!s || block >= WM_BGND_MAX_BLOCKS) return false;
    return (s->resident[block >> 3] & (uint8_t)(1u << (block & 7u))) != 0;
}

static void set_resident(wm_bgnd_state_t *s, size_t block, bool on) {
    uint8_t bit;
    if (!s || block >= WM_BGND_MAX_BLOCKS) return;
    bit = (uint8_t)(1u << (block & 7u));
    if (on) {
        if (s->resident[block >> 3] & bit) return;
        s->resident[block >> 3] |= bit;
        s->resident_count += 1;
    } else {
        if (!(s->resident[block >> 3] & bit)) return;
        s->resident[block >> 3] &= (uint8_t)~bit;
        if (s->resident_count) s->resident_count -= 1;
    }
}

wm_bgnd_rect_t wm_bgnd_display_rect(int32_t worldtl_x, int32_t worldtl_y,
                                    int32_t scrntl_x, int32_t scrntl_y,
                                    int32_t scrnlr_x, int32_t scrnlr_y) {
    wm_bgnd_rect_t r;
    /*
     * `move @WORLDTL,a3,L / move a3,a4 / move @SCRNTL,a0,L /
     * addxy a0,a3 / move @SCRNLR,a0,L / addxy a0,a4`, then DISP_PAD
     * off the top-left and onto the bottom-right.
     */
    r.left = worldtl_x + scrntl_x - WM_BGND_PAD_X;
    r.top = worldtl_y + scrntl_y - WM_BGND_PAD_Y;
    r.right = worldtl_x + scrnlr_x + WM_BGND_PAD_X;
    r.bottom = worldtl_y + scrnlr_y + WM_BGND_PAD_Y;
    return r;
}

/* A block's size, out of its module's header table. */
static bool block_size(const wm_bgnd_module_t *m, const wm_bgnd_block_t *b,
                       uint16_t *w, uint16_t *h) {
    size_t idx;
    /* `MAP_HDR SET TO >FFFF IF BLOCK IS NOT ALLOCATED`. */
    if (b->hdr == 0xffffu) return false;
    /* `sll 32-12,a14 / srl 32-12-4,a14` -- bits 0-11 are the offset,
       and the top nibble is four more bits of palette, not index. */
    idx = (size_t)(b->hdr & 0x0fffu);
    if (idx >= m->header_count) return false;
    *w = m->headers[idx].w;
    *h = m->headers[idx].h;
    return true;
}

size_t wm_bgnd_get1stx(const wm_bgnd_module_t *module, int32_t x) {
    size_t low, high;

    if (!module || module->block_count == 0) return 0;
    low = 0;
    high = module->block_count;

    /*
     * `movk 5,a14 ;threshhold for switching from binary to linear
     * search`, and the halving is `high = mid` / `low = mid` rather
     * than mid±1 -- which does not converge on its own and does not
     * have to, because the threshold stops it.
     */
    while (high - low > 5) {
        size_t mid = low + (high - low) / 2;
        if ((int32_t)module->blocks[mid].x < x) low = mid;
        else high = mid;
    }

    /* "FINISH WITH A LINEAR SEARCH ... ENDING WITH FIRST BLOCK X COOR
       THAT IS >= A0". */
    for (; low <= high && low < module->block_count; ++low)
        if ((int32_t)module->blocks[low].x >= x) return low;
    /* `clr a0 ;block not found, return 0` -- reported here as "past
       the end" so a caller cannot mistake it for block zero. */
    return module->block_count;
}

/*
 * Record one event. The COUNT always advances, even when there is
 * nowhere to put the details -- a caller whose array was too small
 * still learns how many blocks moved, and `overflow` says the
 * details of some of them are missing. The residency bits are
 * updated either way, so the set stays right; it is only the
 * per-block notification that is lost.
 */
static void push(wm_bgnd_event_t *out, size_t cap, size_t *n, bool *overflow,
                 size_t index, const wm_bgnd_placed_t *p,
                 const wm_bgnd_block_t *b, uint16_t w, uint16_t h) {
    if (out && *n < cap) {
        out[*n].index = index;
        out[*n].module = p->module;
        out[*n].block = b;
        out[*n].world_x = p->x + b->x;
        out[*n].world_y = p->y + b->y;
        out[*n].w = w;
        out[*n].h = h;
    } else if (out && overflow) {
        *overflow = true;
    }
    *n += 1;
}

size_t wm_bgnd_delnonvis(wm_bgnd_state_t *s,
                         const wm_bgnd_placed_t *list, size_t list_count,
                         wm_bgnd_rect_t view,
                         wm_bgnd_event_t *out, size_t cap, bool *overflow) {
    size_t li, n = 0, base = 0;

    if (!s || !list) return 0;
    for (li = 0; li < list_count; ++li) {
        const wm_bgnd_placed_t *p = &list[li];
        size_t bi;
        if (!p->module) continue;
        for (bi = 0; bi < p->module->block_count; ++bi) {
            const wm_bgnd_block_t *b = &p->module->blocks[bi];
            size_t index = base + bi;
            int32_t bx, by;
            uint16_t w, h;
            bool keep;

            if (!wm_bgnd_is_resident(s, index)) continue;
            if (!block_size(p->module, b, &w, &h)) {
                set_resident(s, index, false);
                continue;
            }
            bx = p->x + b->x;
            by = p->y + b->y;
            /*
             * The delete test, and note that it is NOT the add test
             * inverted: `CMPXY A7,A4 / JRXLT DEL_IT` keeps a block
             * whose X is exactly BR.x, where the add test's
             * `JRXGE` would have refused it.
             */
            keep = bx <= view.right && by <= view.bottom &&
                   bx + (int32_t)w >= view.left &&
                   by + (int32_t)h >= view.top;
            if (keep) continue;
            set_resident(s, index, false);
            push(out, cap, &n, overflow, index, p, b, w, h);
        }
        base += p->module->block_count;
    }
    return n;
}

/*
 * bgnd_addmod, for one module, with the rect already made relative
 * to the module's origin the way bgnd_scanmod makes it.
 */
static void add_module(wm_bgnd_state_t *s, const wm_bgnd_placed_t *p,
                       size_t base, wm_bgnd_rect_t rel,
                       wm_bgnd_event_t *out, size_t cap, size_t *n,
                       bool *overflow) {
    const wm_bgnd_module_t *m = p->module;
    size_t bi;
    bool quick = false;

    /*
     * `movx a3,a0 / sext a0 / subi WIDEST_BLOCK,a0 / callr
     * bgnd_get1stx` -- start a whole widest-block to the left of the
     * display, or a wide block straddling the edge is never reached.
     */
    bi = wm_bgnd_get1stx(m, rel.left - WM_BGND_WIDEST_BLOCK);

    for (; bi < m->block_count; ++bi) {
        const wm_bgnd_block_t *b = &m->blocks[bi];
        size_t index = base + bi;
        uint16_t w, h;
        int32_t bx = b->x, by = b->y;

        /* `movb *a8,a0 / jrn #sclp1 ;Already displayed?` */
        if (wm_bgnd_is_resident(s, index)) continue;

        if (!quick) {
            /* `cmpxy a3,a1 / JRXGE #qscanstrt` -- once a block starts
               at or past the display's left edge, every later one
               does too, and the cheap scan takes over. */
            if (bx >= rel.left) quick = true;
        }

        if (!block_size(m, b, &w, &h)) continue;

        if (!quick) {
            if (bx + (int32_t)w < rel.left) continue;
            if (by >= rel.bottom) continue;
            if (by + (int32_t)h < rel.top) continue;
        } else {
            if (by >= rel.bottom) continue;
            if (by + (int32_t)h < rel.top) continue;
            /*
             * `cmpxy a4,a1 / JRXGE #x ;BLOCK X > BR X` -- not a skip
             * but a STOP. The blocks are sorted by X, so nothing
             * after this one can be visible either.
             */
            if (bx >= rel.right) return;
        }

        set_resident(s, index, true);
        push(out, cap, n, overflow, index, p, b, w, h);
    }
}

size_t wm_bgnd_scanmod(wm_bgnd_state_t *s,
                       const wm_bgnd_placed_t *list, size_t list_count,
                       wm_bgnd_rect_t view,
                       wm_bgnd_event_t *out, size_t cap, bool *overflow) {
    size_t li, n = 0, base = 0;

    if (!s || !list) return 0;
    for (li = 0; li < list_count; ++li) {
        const wm_bgnd_placed_t *p = &list[li];
        wm_bgnd_rect_t rel;
        int32_t ex, ey;

        if (!p->module) continue;
        /*
         * The whole module is culled first: `cmpxy a3,a11 / jrxlt
         * #next` and its three siblings, against the module's own
         * start and its start plus its size.
         */
        ex = p->x + p->module->width;
        ey = p->y + p->module->height;
        if (ex < view.left || ey < view.top ||
            p->x > view.right || p->y > view.bottom) {
            base += p->module->block_count;
            continue;
        }

        /* `subxy a9,a3 / subxy a9,a4` -- the scan works in module
           coordinates, and puts them back afterwards. */
        rel.left = view.left - p->x;
        rel.top = view.top - p->y;
        rel.right = view.right - p->x;
        rel.bottom = view.bottom - p->y;

        add_module(s, p, base, rel, out, cap, &n, overflow);
        base += p->module->block_count;
    }
    return n;
}

void wm_bgnd_update(wm_bgnd_state_t *s,
                    const wm_bgnd_placed_t *list, size_t list_count,
                    wm_bgnd_rect_t view,
                    wm_bgnd_event_t *added, size_t added_cap,
                    size_t *added_count,
                    wm_bgnd_event_t *removed, size_t removed_cap,
                    size_t *removed_count,
                    bool *overflow) {
    size_t gone, came;

    if (overflow) *overflow = false;
    /* `callr bgnd_delnonvis` then `callr bgnd_scanmod`, in that
       order: the deletes free the objects the adds then take. */
    gone = wm_bgnd_delnonvis(s, list, list_count, view,
                             removed, removed_cap, overflow);
    came = wm_bgnd_scanmod(s, list, list_count, view,
                           added, added_cap, overflow);
    if (removed_count) *removed_count = gone;
    if (added_count) *added_count = came;
}
