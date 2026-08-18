#include "wm/game.h"
#include <string.h>

void wm_game_init(wm_game *g) {
    memset(g, 0, sizeof(*g));
    wm_scheduler_init(&g->scheduler);
    wm_ropes_init(&g->ropes);
    g->running = 1;
}

void wm_game_tick(wm_game *g) {
    wm_scheduler_step(&g->scheduler);
    ++g->frame;
}
