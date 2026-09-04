#include "wm/arcade/wm_arcade_round.h"

int wm_arcade_get_live_bits(wm_arcade_actor_t *const *actors, size_t actor_count) {
    int bits = 0;
    size_t i;

    if (!actors) return 0;

    for (i = 0; i < actor_count; ++i) {
        const wm_arcade_actor_t *a = actors[i];
        bool live;

        if (!a || !a->active) continue;

        live = (a->player_mode != WM_PMODE_DEAD) ||
               (a->status_flags & WM_STATUS_ZOMBIE) != 0;
        if (!live) continue;

        bits |= (a->player_side != 0) ? 0x2 : 0x1;
    }
    return bits;
}

void wm_arcade_round_state_init(wm_arcade_round_state_t *rs) {
    if (!rs) return;
    rs->pin_timeout = 0;
    rs->decided = false;
    rs->decided_winner_side = -1;
}

void wm_arcade_round_tick(wm_arcade_round_state_t *rs,
                          wm_arcade_actor_t *const *actors, size_t actor_count) {
    int live;

    if (!rs || rs->decided) return;

    live = wm_arcade_get_live_bits(actors, actor_count);
    if (live == 3) {
        /* Both sides have a live member: no KO in progress. */
        rs->pin_timeout = 0;
        return;
    }

    if (rs->pin_timeout == 0) {
        /* WRESTLE2.ASM:4196, a newly all-dead condition. */
        rs->pin_timeout = WM_ARCADE_PIN_TIMEOUT_TICKS;
    }

    if (--rs->pin_timeout == 0) {
        rs->decided = true;
        if (live == 1) rs->decided_winner_side = 0;
        else if (live == 2) rs->decided_winner_side = 1;
        else rs->decided_winner_side = -1; /* simultaneous double-KO draw */
    }
}

void wm_arcade_match_score_init(wm_arcade_match_score_t *score) {
    if (!score) return;
    score->p1rounds = 0;
    score->p2rounds = 0;
    score->match_winner = 0;
}

void wm_arcade_match_score_award_round(wm_arcade_match_score_t *score, int winner_side) {
    int32_t *rounds;

    if (!score || score->match_winner != 0) return;
    if (winner_side != 0 && winner_side != 1) return; /* draw: no award */

    rounds = (winner_side == 0) ? &score->p1rounds : &score->p2rounds;
    ++*rounds;

    if (*rounds >= 2)
        score->match_winner = winner_side + 1;
}
