/*
 * DCSSOUND.ASM:2911 ADD_TO_QUEUE / ADD_IF_SILENT -- the front half of the
 * announcer, over the tables tools/wlvoice.py extracts.
 *
 * The shape of the routine, in the source's own order:
 *
 *   - the table's `.WORD -1` at -050H clears REPEAT_STATE first;
 *   - its `.LONG` at -040H is a crowd reaction table (DO_CROWD_ANYWAY);
 *   - if REPEAT_STATE is live the whole draw is skipped and the line is
 *     forced to REPEAT_MODE;
 *   - otherwise RNDPER at the caller's percentage, then RNDRNG0 over the
 *     row count at -020H picks a row;
 *   - six negative row values mean "a personal line", resolved by
 *     SET_UP_PERSONAL_CALL from the attacking wrestler's number;
 *   - ARE_WE_REPEATING rejects a line said in the last four, and an
 *     ordinary row that is rejected walks FORWARD to the next row;
 *   - what survives goes to ADD_VOICE or IF_SILENT_ADD_VOICE, and a
 *     multi-word row queues its remaining words too, stopping at a zero.
 *
 * Two things the source does that this does not, both stated rather than
 * approximated: DO_CROWD_ANYWAY (the crowd's own sound and cheer
 * animation, which needs a crowd this port does not have) and the process
 * creation around REPEAT_DUMMY, whose 80-tick life is a counter here.
 */
#include "wm/announce_tables.h"

#include <string.h>

const wm_announce_table *wm_announce_table_find(const char *name) {
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < wm_announce_table_count; ++i)
        if (strcmp(wm_announce_tables[i].name, name) == 0)
            return &wm_announce_tables[i];
    return NULL;
}

const wm_announce_call *wm_announce_call_find(const char *name) {
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < wm_announce_call_count; ++i)
        if (strcmp(wm_announce_calls[i].name, name) == 0)
            return &wm_announce_calls[i];
    return NULL;
}

void wm_announce_tick_repeat(wm_announcer_state *a) {
    if (!a || !a->repeat_ticks) return;
    if (--a->repeat_ticks == 0) a->repeat_state = 0;   /* REPEAT_DUMMY */
}

/* UTIL.ASM:1734 RNDPER, exactly as the rest of this port draws it. */
static bool rndper(const wm_announce_ctx *ctx, uint32_t per_mille) {
    if (!ctx || !ctx->rng) return false;
    return wm_rng_rndrng0(ctx->rng, 999u) < per_mille;
}

static uint32_t rnd0(const wm_announce_ctx *ctx, uint32_t max_inclusive) {
    if (!ctx || !ctx->rng) return 0u;
    return wm_rng_rndrng0(ctx->rng, max_inclusive);
}

/* ARE_WE_REPEATING: was this line one of the last four? */
static bool are_we_repeating(const wm_announcer_state *a, int16_t call) {
    int i;
    for (i = 0; i < WM_ANNOUNCE_REPEAT_MEMORY; ++i)
        if (a->last_voice[i] == call) return true;
    return false;
}

/* ADD_SPEECH_TO_LIST: pre-increment, wrap past the fourth slot. */
static void add_speech_to_list(wm_announcer_state *a, int16_t call) {
    a->which_last_voice =
        (uint8_t)((a->which_last_voice + 1u) % WM_ANNOUNCE_REPEAT_MEMORY);
    a->last_voice[a->which_last_voice] = call;
}

/*
 * SET_UP_PERSONAL_CALL. The chain of `INC A0 / JRZ` tests maps each
 * sentinel onto one of the five per-wrestler tables; anything left is
 * REPEAT_MODE, which instead advances the repeat counter and reads
 * ASCENDING_TABLE. Returns the resolved line id.
 */
static int16_t set_up_personal_call(wm_announcer_state *a, int16_t call,
                                    const wm_announce_ctx *ctx) {
    int who = ctx ? ctx->wrestler_num : -1;
    int kind;

    /*
     * A5 is the attacking wrestler, and only the callers whose process
     * copies WRESTLERNUM into it have one. GETPRC (MPROC.ASM:436) carries
     * A7-A12 into a created process and nothing else, so for the callers
     * that do NOT set it -- PROC_MISSES and PROC_MISS_YOKO -- A5 is
     * whatever the scheduler last left there, and MISS_YOKO does contain a
     * GIVE_CREDIT row. Which wrestler the original credits there is simply
     * not determined by the source, so this says nothing rather than
     * crediting one it picked itself.
     */
    if (who < 0 || who >= WM_ANNOUNCE_WRESTLERS) return 0;

    switch (call) {
    case WM_ANN_GIVE_CREDIT:         kind = 0; break;
    case WM_ANN_VERY_IMPRESSIVE:     kind = 1; break;
    case WM_ANN_IT_DOESNT_LOOK_GOOD: kind = 2; break;
    case WM_ANN_R_IMPRESSIVE_MOVE:   kind = 3; break;
    case WM_ANN_GIDDUP_MODE:         kind = 4; break;
    default:                         kind = -1; break;
    }
    if (kind >= 0) return wm_announce_personal[kind][who];

    /* REPEAT_MODE. `MOVK 4,A0` only when the counter is not already
       running, then `DEC` either way -- so the first repeat reads slot 3
       and each one after it climbs. */
    if (a->repeat_state == 0) a->repeat_state = 4;
    --a->repeat_state;
    a->repeat_ticks = WM_ANNOUNCE_REPEAT_TICKS;      /* SET_DUMMY_SLEEP */
    return wm_announce_ascending[who]
                               [a->repeat_state % WM_ANNOUNCE_REPEAT_STEPS];
}

static bool is_special(int16_t call) {
    return call == WM_ANN_GIVE_CREDIT || call == WM_ANN_VERY_IMPRESSIVE ||
           call == WM_ANN_IT_DOESNT_LOOK_GOOD ||
           call == WM_ANN_R_IMPRESSIVE_MOVE || call == WM_ANN_GIDDUP_MODE ||
           call == WM_ANN_REPEAT_MODE;
}

static bool queue(wm_announcer_state *a, int16_t call, bool if_silent) {
    /* `MOVE A0,A0 / JRZ NO_MORE` at ADD_AGAIN3: a zero line id is "no
       line", not line zero. Adam Bomb's cut slot is a literal 0 in all
       five personal tables for the same reason. */
    if (call <= 0) return false;
    return if_silent ? wm_announcer_add_if_silent(a, (uint16_t)call)
                     : wm_announcer_add(a, (uint16_t)call);
}

/* DO_THE_SPEECH plus its ADD_AGAIN3 tail: queue the drawn line, then the
   rest of a multi-word row until a zero word or the row runs out. */
static int do_the_speech(wm_announcer_state *a, const wm_announce_table *t,
                         size_t word, int16_t call, bool if_silent,
                         unsigned words_left) {
    int added = 0;

    if (call <= 0) return 0;
    add_speech_to_list(a, call);
    if (!queue(a, call, if_silent)) return 0;   /* JRN NO_MORE */
    ++added;

    while (words_left > 1u) {
        int16_t more;
        --words_left;
        ++word;
        if (word >= t->word_count) break;
        more = t->rows[word];
        if (more == 0) break;                   /* `JRZ NO_MORE` */
        if (!wm_announcer_add(a, (uint16_t)more)) break;
        ++added;
    }
    return added;
}

/*
 * DO_END_STUFF. A row of END_GAME_STUFF asks whether anybody is under 40
 * health; if so the line comes from SPECIAL_LAST_STUFF instead and the
 * repeat counter is cleared, and if not the call falls back into the
 * ordinary walk-forward. Returns -1 for "not near the end, keep walking".
 */
static int do_end_stuff(wm_announcer_state *a, bool if_silent,
                        const wm_announce_ctx *ctx) {
    const wm_announce_table *last = wm_announce_table_find("SPECIAL_LAST_STUFF");
    int16_t call;
    size_t word;

    if (!ctx || !ctx->anyone_near_death) return -1;
    if (!ctx->anyone_near_death(ctx->user)) return -1;
    if (!last) return -1;

    word = (size_t)rnd0(ctx, last->last_index) * last->stride;
    if (word >= last->word_count) return 0;
    call = last->rows[word];
    if (are_we_repeating(a, call)) return 0;    /* SET_NO_MORE */
    a->repeat_state = 0;
    a->repeat_ticks = 0;
    if (call == WM_ANN_IT_DOESNT_LOOK_GOOD)
        call = set_up_personal_call(a, call, ctx);
    return do_the_speech(a, last, word, call, if_silent, last->stride);
}

int wm_announce_from_table(wm_announcer_state *a, const wm_announce_table *t,
                           uint16_t percent, bool if_silent,
                           const wm_announce_ctx *ctx) {
    size_t word;
    unsigned words_left;

    if (!a || !t || t->stride == 0 || t->word_count == 0) return 0;

    /* `MOVE *A2(-050H),A3 / JRZ NO_RESET_REPEAT` */
    if (t->reset_repeat) {
        a->repeat_state = 0;
        a->repeat_ticks = 0;
    }
    /* `MOVE *A2(-040H),A3,L / JRZ NO_CROWD / CALLA DO_CROWD_ANYWAY` is
       deliberately not translated: it drives the crowd's own sound and
       cheer animation, and this port has no crowd. */

    if (a->repeat_state != 0) {
        /* The queue is already counting repeats: no draw, no percentage. */
        int16_t call = set_up_personal_call(a, WM_ANN_REPEAT_MODE, ctx);
        if (are_we_repeating(a, call)) return 0;
        return do_the_speech(a, t, t->word_count, call, if_silent, 1u);
    }

    if (!rndper(ctx, percent)) return 0;        /* JRLS NO_MORE */

    word = (size_t)rnd0(ctx, t->last_index) * t->stride;
    words_left = t->stride;

    for (;;) {
        int16_t call;

        /*
         * The source has no bound here at all: a rejected row walks
         * forward with `ADD A3,A1` and reads whatever the assembler put
         * next. Every table carries a copy of its own first rows as
         * padding for exactly that reason, and the walk stops when it
         * runs out of them rather than reading past the data.
         */
        if (word >= t->word_count) return 0;
        call = t->rows[word];

        if (call == WM_ANN_END_GAME_STUFF) {
            int done = do_end_stuff(a, if_silent, ctx);
            if (done >= 0) return done;
            word += t->stride;                  /* NO_SPECIAL_END_STUFF */
            continue;
        }
        if (is_special(call)) {
            call = set_up_personal_call(a, call, ctx);
            /* A repeated personal line ends the call outright -- the
               source returns -1 from SET_NO_MORE instead of walking. */
            if (are_we_repeating(a, call)) return 0;
            return do_the_speech(a, t, word, call, if_silent, words_left);
        }
        if (!are_we_repeating(a, call))
            return do_the_speech(a, t, word, call, if_silent, words_left);
        word += t->stride;                      /* NO_SPECIAL_END_STUFF */
    }
}
