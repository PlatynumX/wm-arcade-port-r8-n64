/*
 * UTIL.ASM:1174 CYCLE_TABLE -- see wm/arcade/wm_arcade_colcyc.h.
 * The tables are src/generated/colcyc_tables.c.
 */
#include "wm/arcade/wm_arcade_colcyc.h"

void wm_colcyc_begin(wm_colcyc_t *c, const wm_colcyc_table_t *table,
                     uint16_t start_color, uint16_t count) {
    if (!c) return;
    c->table = table;
    c->phase = WM_COLCYC_WAIT_PAL;
    c->timer = 0;
    c->pos = 0;
    c->start_color = start_color;
    c->count = count;
    /* `MOVE *A10,A2,W / MOVE A2,*A13(PDATA+16)`. */
    c->stop_word = (table && table->words && table->word_count)
                       ? table->words[0] : 0u;
    c->passes = 0;
    c->write = false;
    c->src = NULL;
}

bool wm_colcyc_tick(wm_colcyc_t *c, bool palette_found,
                    bool palette_is_still_ours) {
    bool spent = false;

    if (!c) return false;
    c->write = false;
    c->src = NULL;
    if (!c->table || !c->table->words || c->table->word_count == 0) {
        c->phase = WM_COLCYC_DEAD;
        return false;
    }

    for (;;) {
        switch (c->phase) {
        case WM_COLCYC_WAIT_PAL:
            if (c->timer > 0) {
                if (spent) return true;
                c->timer--;
                /* A sleep tick is the whole tick: `SLEEP 60` runs to
                   completion before the pal_find below it. */
                return true;
            }
            /* `MOVE A9,A0 / calla pal_find / jrz CYC0`. The first
               look costs nothing; a miss costs sixty ticks. */
            if (palette_found) {
                c->pos = 0;
                c->phase = WM_COLCYC_RUN;
                continue;
            }
            c->timer = WM_COLCYC_RETRY_TICKS;
            continue;

        case WM_COLCYC_RUN:
            if (c->timer > 0) {
                if (spent) return true;
                c->timer--;
                /* `CALLA PRCSLP` with a11 = 3: three whole ticks
                   between writes, not two and a shared one. */
                return true;
            }
            /*
             * The KILL_US guard, re-read before every write: the
             * palette record has to still be the one pal_find
             * returned, or the process dies rather than writing
             * into whoever has the slot now.
             */
            if (!palette_is_still_ours) {
                c->phase = WM_COLCYC_DEAD;
                continue;
            }
            /* `move a8,a1 / MOVE A9,A0 / MOVE *A13(PDATA),A2 /
               calla pal_set`. */
            c->write = true;
            c->src = &c->table->words[c->pos];
            /* `MOVE A11,A0 / CALLA PRCSLP`. */
            c->timer = WM_COLCYC_TICKS;
            /* `ADDK >10,A9 / MOVE *A9,A0 / JRN RESTUFF`, then the
               compare against word zero. */
            c->pos++;
            if (c->pos >= c->table->word_count ||
                (c->table->words[c->pos] & 0x8000u) != 0u ||
                c->table->words[c->pos] == c->stop_word) {
                c->pos = 0;
                c->passes++;
            }
            continue;

        case WM_COLCYC_DEAD:
        default:
            return false;
        }
    }
}
