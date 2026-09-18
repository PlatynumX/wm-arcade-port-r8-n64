/*
 * AWARD.ASM:3056 decompress_string -- see wm/arcade/wm_arcade_story.h.
 */
#include "wm/arcade/wm_arcade_story.h"

size_t wm_story_decompress(const uint8_t *packed, size_t packed_bytes,
                           char *out, size_t cap) {
    uint32_t bits = 0;
    unsigned have = 0;
    size_t i, n = 0;

    if (!out || cap == 0) return (size_t)-1;
    if (!packed) { out[0] = '\0'; return 0; }

    for (i = 0; i < packed_bytes; ++i) {
        bits |= (uint32_t)packed[i] << have;
        have += 8;
        /*
         * `setf 6,0,0 / move *a0+,a2` -- six bits at a time out of a
         * little-endian stream, least significant first.
         */
        while (have >= 6) {
            unsigned v = bits & 0x3fu;
            bits >>= 6;
            have -= 6;
            /* `jrz #decompress_done`, then `clr a0 / movb a0,*a1`. */
            if (v == 0) { out[n] = '\0'; return n; }
            if (n + 1 >= cap) return (size_t)-1;
            /* `addi 01fh,a2 / movb a2,*a1`. */
            out[n++] = (char)(v + 0x1f);
        }
    }
    /* The blob ran out with no zero field. The source would keep
       reading whatever followed it in memory; this stops. */
    out[n] = '\0';
    return n;
}

/* =================================================================
 * STORIES.ASM:95 print_story / :168 show_wrestler_end_story.
 * ================================================================= */

/* STORIES.ASM:157 w_logos. */
const char *const wm_story_logos[WM_STORY_SLOTS] = {
    "HRT3", "RZR3", "UND3", "YOK3", "SHN3",
    "BAM3", "DNK3", NULL,   "LEX3",
};

/*
 * `#do_more_lines: movi 90,a3` through `jrlt #print_story_loop`.
 *
 * The terminator is tested before the line is printed and the cursor
 * limit after, so a page holds at most twelve lines and may hold
 * fewer only by running out of table.
 */
static void lay_out_page(wm_story_player_t *p) {
    int32_t y = WM_STORY_FIRST_Y;

    p->page_first = p->next;
    p->page_lines = 0;
    p->page_ended_short = false;

    for (;;) {
        /* `move *a1+,a0,L / jrz #print_story_done`. */
        if (p->next >= p->line_count || p->lines[p->next] == NULL) {
            p->page_ended_short = true;
            break;
        }
        p->page_y[p->page_lines] = (int16_t)y;
        p->page_lines++;
        p->next++;
        /* `addi 12,a3 / cmpi 230,a3 / jrlt`. */
        y += WM_STORY_LINE_STEP;
        if (y >= WM_STORY_Y_LIMIT) break;
    }
    p->pages_shown++;
}

void wm_story_play_begin(wm_story_player_t *p, const wm_story_t *story) {
    if (!p) return;
    p->lines = story ? story->lines : NULL;
    p->line_count = story ? story->line_count : 0;
    p->next = 0;
    p->page_first = 0;
    p->page_lines = 0;
    p->phase = WM_STORY_PHASE_PRINT;
    p->timer = 0;
    p->pages_shown = 0;
    p->erase_lines = false;
    p->page_ended_short = false;
}

bool wm_story_play_tick(wm_story_player_t *p, unsigned buttons) {
    /*
     * One frame. Laying a page out, deleting the last one and testing
     * the terminator cost no tick in the source -- there is no SLEEP
     * between `calla obj_del1c` and `jruc #do_more_lines` -- so this
     * keeps stepping until it reaches something that does, spends at
     * most one tick on it, and then keeps stepping through anything
     * free that follows. `spent` is what stops it going round twice.
     */
    bool spent = false;

    if (!p) return false;
    p->erase_lines = false;

    for (;;) {
        switch (p->phase) {
        case WM_STORY_PHASE_PRINT:
            lay_out_page(p);
            if (p->page_ended_short) {
                /* `jrz #print_story_done` -- past both waits. */
                p->phase = WM_STORY_PHASE_TRAIL;
                p->timer = WM_STORY_TRAIL_TICKS;
            } else {
                p->phase = WM_STORY_PHASE_MIN_SLEEP;
                p->timer = WM_STORY_MIN_TICKS;
            }
            continue;

        case WM_STORY_PHASE_MIN_SLEEP:
            /* `SLEEP TSEC*5` -- no button is read during it. */
            if (p->timer == 0) {
                p->phase = WM_STORY_PHASE_WAIT;
                p->timer = WM_STORY_WAIT_TICKS;
                continue;
            }
            if (spent) return true;
            p->timer--;
            spent = true;
            continue;

        case WM_STORY_PHASE_WAIT:
            /* `#w_loop: SLEEPK 1` then the two get_but_val_cur calls,
               then `dsjs a9,#w_loop`: the tick is spent first. */
            if (spent) return true;
            p->timer--;
            spent = true;
            if (buttons != 0 || p->timer == 0) {
                /* `move *a1,a0,L / jrz #print_story_done` -- a PEEK,
                   so the terminator is not consumed and the next page
                   starts on the line this test looked at. */
                if (p->next >= p->line_count || p->lines[p->next] == NULL) {
                    p->phase = WM_STORY_PHASE_TRAIL;
                    p->timer = WM_STORY_TRAIL_TICKS;
                } else {
                    /* `calla obj_del1c / jruc #do_more_lines` -- the
                       erase and the next page land in this same
                       frame, which is why the flag and the fresh
                       page_y come back together. */
                    p->erase_lines = true;
                    p->phase = WM_STORY_PHASE_PRINT;
                }
            }
            continue;

        case WM_STORY_PHASE_TRAIL:
            /* `SLEEPK TSEC/2` before RETP. */
            if (p->timer == 0) {
                p->phase = WM_STORY_PHASE_DONE;
                continue;
            }
            if (spent) return true;
            p->timer--;
            spent = true;
            continue;

        case WM_STORY_PHASE_DONE:
        default:
            /* True only if this call was still part of the story. */
            return spent;
        }
    }
}

/* ---- show_wrestler_end_story ------------------------------------ */

int wm_story_winning_wrestler(int32_t pstatus, const int16_t which_player[2]) {
    /* `srl 1,a14 / sll 4,a14 / addi which_player,a14 / move *a14,a8`. */
    int idx = (int)((pstatus >> 1) & 1);
    if (!which_player) return WM_STORY_INVALID_WRESTLER;
    return (int)which_player[idx];
}

void wm_story_logo_xy(int32_t isizex, int32_t isizey,
                      int32_t *x, int32_t *y) {
    /* `srl 1` in pixels, `sll 16` into the integer half, `neg`, then
       `addi [120,0]` / `addi [50,0]`. */
    int32_t hx = (int32_t)(((uint32_t)isizex) >> 1);
    int32_t hy = (int32_t)(((uint32_t)isizey) >> 1);
    if (x) *x = (int32_t)((120u - (uint32_t)hx) << 16);
    if (y) *y = (int32_t)((50u - (uint32_t)hy) << 16);
}

bool wm_story_end_begin(wm_story_end_t *e, int32_t pstatus,
                        const int16_t which_player[2]) {
    if (!e) return false;
    e->timer = 0;
    e->build_scene = false;
    e->tear_down = false;
    e->erase_lines = false;
    e->wrestler = wm_story_winning_wrestler(pstatus, which_player);
    wm_story_play_begin(&e->player, NULL);

    /* `cmpi 7,a8 / jrz inval_wnum` -- straight to RETP. */
    if (e->wrestler == WM_STORY_INVALID_WRESTLER ||
        e->wrestler < 0 || e->wrestler >= WM_STORY_SLOTS) {
        e->phase = WM_STORY_END_DONE;
        return false;
    }
    e->phase = WM_STORY_END_FADE;
    e->timer = WM_STORY_FADE_TICKS;
    return true;
}

bool wm_story_end_tick(wm_story_end_t *e, unsigned buttons) {
    bool consumed = false;

    if (!e) return false;
    e->build_scene = false;
    e->tear_down = false;
    e->erase_lines = false;

    while (!consumed) {
        switch (e->phase) {
        case WM_STORY_END_FADE:
            /* `CREATE0 fade_down / SLEEPK 30`, then WIPEOUT, the
               background, and the three objects -- all in one frame. */
            if (e->timer > 0) {
                e->timer--;
                consumed = true;
                break;
            }
            e->build_scene = true;
            {
                /* `move *a1,a1,L` twice: the wrestler's story TABLE,
                   then its first entry. Only story zero is ever
                   shown, however many a wrestler has. */
                const wm_story_set_t *set = &wm_wrestler_stories[e->wrestler];
                const wm_story_t *story =
                    (set->stories && set->story_count > 0) ? &set->stories[0]
                                                           : NULL;
                wm_story_play_begin(&e->player, story);
            }
            e->phase = WM_STORY_END_STORY;
            break;

        case WM_STORY_END_STORY:
            /* `JSRP print_story`. */
            if (wm_story_play_tick(&e->player, buttons)) {
                e->erase_lines = e->player.erase_lines;
                consumed = true;
                break;
            }
            e->phase = WM_STORY_END_WAIT;
            e->timer = WM_STORY_WAIT_TICKS;
            break;

        case WM_STORY_END_WAIT:
            /* `#but_wait: SLEEPK 1` / buttons / `dsjs a9,#but_wait`.
               This is what actually holds a short final page up. */
            consumed = true;
            if (e->timer > 0) e->timer--;
            if (buttons != 0 || e->timer == 0) {
                /* `obj_del1c CLSMK3` then `obj_del1c TYPWCCOUNT`. */
                e->tear_down = true;
                e->phase = WM_STORY_END_DONE;
            }
            break;

        case WM_STORY_END_DONE:
        default:
            return false;
        }
    }
    return true;
}
