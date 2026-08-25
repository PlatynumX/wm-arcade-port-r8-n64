#include "wm/cabinet_bridge.h"
#include <string.h>
void wm_cabinet_bridge_init(wm_cabinet_bridge_state *s) {
    if (s) memset(s, 0, sizeof(*s));
}
bool wm_cabinet_bridge_accept_player_start(wm_cabinet_bridge_state *s, unsigned player) {
    uint8_t bit;
    if (!s || player > 1u) return false;
    bit = (uint8_t)(1u << player);
    s->old_pstatus = s->pstatus;
    s->pstatus = (uint8_t)(s->pstatus | bit);
    ++s->accepted_player_starts[player];
    return true;
}
uint8_t wm_cabinet_bridge_pstatus(const wm_cabinet_bridge_state *s) {
    return s ? s->pstatus : 0u;
}
void wm_cabinet_bridge_note_physical_coin(wm_cabinet_bridge_state *s) {
    if (s) ++s->physical_coin_events;
}
