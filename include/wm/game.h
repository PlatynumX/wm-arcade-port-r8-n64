#ifndef WM_GAME_H
#define WM_GAME_H
#include <stdint.h>
#include "wm/process.h"

typedef struct {
    wm_scheduler scheduler;
    uint64_t frame;
    int running;
} wm_game;

void wm_game_init(wm_game *g);
void wm_game_tick(wm_game *g);

#endif
