/*
 * DNKSEQ2.ASM:5202 win_announce and LIFEBAR.ASM:2642 announce_rnd_winner.
 * See wm/arcade/wm_arcade_round_announce.h for what is and is not here.
 */
#include "wm/arcade/wm_arcade_round_announce.h"

#include "wm/arcade/wm_arcade_combat_defs.h"

#include <string.h>

void wm_arcade_round_announce_init(wm_arcade_round_announce_t *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    st->phase = (uint8_t)WM_ARW_IDLE;
}

wm_arcade_actor_t *wm_arcade_anyone_bucking(wm_arcade_actor_t *const *actors,
                                            size_t actor_count,
                                            bool royal_rumble) {
    size_t i;
    /* `move @royal_rumble,a14 / jrnz #no` */
    if (royal_rumble || !actors) return NULL;
    for (i = 0; i < actor_count; ++i) {
        wm_arcade_actor_t *a = actors[i];
        uint32_t f;
        if (!a || !a->active) continue;            /* `jrz #nxt` */
        f = a->status_flags;
        if (!(f & (uint32_t)WM_STATUS_DO_BUCKOFF)) continue;
        if (f & (uint32_t)WM_STATUS_NEW_BUCKOFF) return a;
        if (f & (uint32_t)WM_STATUS_DID_BUCKOFF) continue;
        return a;
    }
    return NULL;
}

void wm_arcade_set_all_buckoffs(wm_arcade_actor_t *const *actors,
                                size_t actor_count) {
    size_t i;
    if (!actors) return;
    for (i = 0; i < actor_count; ++i) {
        if (!actors[i] || !actors[i]->active) continue;
        actors[i]->status_flags |= (uint32_t)WM_STATUS_NO_BUCKOFF;
    }
}

wm_arcade_actor_t *wm_arcade_set_winner(wm_arcade_actor_t *const *actors,
                                        size_t actor_count) {
    wm_arcade_actor_t *alive = NULL;
    size_t i;
    if (!actors) return NULL;
    for (i = 0; i < actor_count; ++i) {
        wm_arcade_actor_t *a = actors[i];
        if (!a || !a->active) continue;                /* `jrz #nxt0` */
        if (a->player_mode == WM_PMODE_DEAD) continue; /* `jreq #nxt0` */
        /* "if he pinned, use him. if not, save his pointer in case we
           find no pinner, but keep looking" */
        if (a->status_flags & (uint32_t)WM_STATUS_DID_PIN) return a;
        if (!alive) alive = a;
    }
    if (alive) return alive;
    /* `#bogus`: "give the win to the first valid wrestler we can find". */
    for (i = 0; i < actor_count; ++i)
        if (actors[i] && actors[i]->active) return actors[i];
    return NULL;                                       /* `#no_hits` */
}

bool wm_arcade_win_announce(wm_arcade_round_announce_t *st) {
    if (!st) return false;
    /* `CALLA KILL_PIN_HIM` -- unconditional, and before the guards below
       have any say, because in the source it is a separate call after the
       CREATE rather than part of the process. */
    ++st->pin_him_kills;
    /* The process's own first two tests, made here rather than by
       starting one that would DIE on its first instruction: `EXISTP
       ANNC_PID / jrz #tmpok` and `move @annc_rnd_winner_done,a14 / jrz
       #arwd_ok`. */
    if (st->phase != (uint8_t)WM_ARW_IDLE) return false;
    if (st->done) return false;
    st->phase = (uint8_t)WM_ARW_FINISH_WAIT;
    st->sleep_left = 0;
    return true;
}

/* `#nobuck` onward, up to the SLEEPK 30. */
static void end_the_round(wm_arcade_round_announce_t *st,
                          wm_arcade_actor_t *const *actors,
                          size_t actor_count,
                          const wm_arcade_round_announce_ctx_t *ctx) {
    st->round_end_time = ctx->pcnt;          /* `move @PCNT,a0,L` */
    st->done = true;                         /* annc_rnd_winner_done */
    wm_arcade_set_all_buckoffs(actors, actor_count);
    if (ctx->sound)
        ctx->sound(ctx->user, WM_ARW_VICTORY_SOUND, WM_ARW_VICTORY_TICKS);
    st->winner = wm_arcade_set_winner(actors, actor_count);
    if (st->winner && ctx->score)
        wm_arcade_match_score_award_round(ctx->score, st->winner->player_side);
    st->phase = (uint8_t)WM_ARW_PRE_TOKEN;
    st->sleep_left = WM_ARW_PRE_TOKEN_SLEEP;
}

/*
 * arw_bwait: clear NEW_BUCKOFF on everyone who has it (counting them),
 * then either go round again, give up on buckoffs, or -- if both teams
 * are somehow still alive -- kill the process.
 */
static bool buckoff_wake(wm_arcade_round_announce_t *st,
                         wm_arcade_actor_t *const *actors,
                         size_t actor_count,
                         const wm_arcade_round_announce_ctx_t *ctx) {
    size_t i;
    int woke = 0;

    for (i = 0; i < actor_count; ++i) {
        wm_arcade_actor_t *a = actors ? actors[i] : NULL;
        if (!a || !a->active) continue;
        if (!(a->status_flags & (uint32_t)WM_STATUS_NEW_BUCKOFF)) continue;
        a->status_flags &= ~(uint32_t)WM_STATUS_NEW_BUCKOFF;
        ++woke;
    }

    /* "now check for an all-dead condition. If neither team is dead,
       die." -- and this test exists only on the buckoff path. */
    if (wm_arcade_get_live_bits(actors, actor_count) == 3) {
        st->phase = (uint8_t)WM_ARW_IDLE;          /* `jauc SUCIDE` */
        return false;
    }

    if (woke) {                                    /* `jrnz #any_b` */
        wm_arcade_actor_t *b =
            wm_arcade_anyone_bucking(actors, actor_count, ctx->royal_rumble);
        if (b) {
            st->sleep_left = WM_ARW_BUCKOFF_SLEEP;
            return false;
        }
        end_the_round(st, actors, actor_count, ctx);
        return true;
    }

    /* "there was at least one wrestler who could have done a buckoff, but
       was too slow." */
    for (i = 0; i < actor_count; ++i) {
        wm_arcade_actor_t *a = actors ? actors[i] : NULL;
        if (!a || !a->active) continue;
        a->buckoff_count = 0;
        a->status_flags &= ~(uint32_t)WM_STATUS_DO_BUCKOFF;
        a->status_flags |= (uint32_t)WM_STATUS_NO_BUCKOFF;
    }
    end_the_round(st, actors, actor_count, ctx);
    return true;
}

bool wm_arcade_round_announce_tick(wm_arcade_round_announce_t *st,
                                   wm_arcade_actor_t *const *actors,
                                   size_t actor_count,
                                   const wm_arcade_round_announce_ctx_t *ctx) {
    if (!st || !ctx) return false;

    switch ((wm_arcade_arw_phase_t)st->phase) {
    case WM_ARW_IDLE:
    case WM_ARW_FINISHED:
        return false;

    case WM_ARW_FINISH_WAIT: {
        wm_arcade_actor_t *b;
        /* `#fini_wait`: "Are we doing a finishing move?" */
        if (ctx->in_finish_move) {
            if (st->sleep_left > 0) { --st->sleep_left; return false; }
            st->sleep_left = WM_ARW_FINISH_POLL;
            return false;
        }
        st->sleep_left = 0;
        /* `#any_b` */
        b = wm_arcade_anyone_bucking(actors, actor_count, ctx->royal_rumble);
        if (b) {
            st->phase = (uint8_t)WM_ARW_BUCKOFF_WAIT;
            st->sleep_left = WM_ARW_BUCKOFF_SLEEP;
            return false;
        }
        end_the_round(st, actors, actor_count, ctx);
        return true;
    }

    case WM_ARW_BUCKOFF_WAIT:
        if (st->sleep_left > 0) { --st->sleep_left; return false; }
        return buckoff_wake(st, actors, actor_count, ctx);

    case WM_ARW_PRE_TOKEN:
        if (st->sleep_left > 0) { --st->sleep_left; return false; }
        /* `movi 27,a3 / calla SNDSND`, then CALL_MATCH_OVER with the
           winner's own PLYR_SIDE. */
        if (ctx->sound) ctx->sound(ctx->user, WM_ARW_TOKEN_SOUND, 0);
        if (ctx->match_over)
            ctx->match_over(ctx->user, st->winner ? st->winner->player_side
                                                  : -1);
        st->phase = (uint8_t)WM_ARW_FINISHED;
        return false;
    }
    return false;
}
