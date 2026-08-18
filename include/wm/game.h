#ifndef WM_GAME_H
#define WM_GAME_H
#include <stdint.h>
#include "wm/process.h"
#include "wm/ropes.h"

typedef struct {
    wm_scheduler scheduler;
    wm_rope_system ropes;
    uint64_t frame;
    int running;
} wm_game;

void wm_game_init(wm_game *g);
void wm_game_tick(wm_game *g);

#endif
