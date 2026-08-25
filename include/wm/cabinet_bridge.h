#ifndef WM_CABINET_BRIDGE_H
#define WM_CABINET_BRIDGE_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* WRESTLE/SELECT source semantics: PSTATUS bit0=P1 active, bit1=P2 active.
   N64 has no physical coin switch; platform Start produces an accepted
   player-start/buy-in event. No synthetic coin credits are fabricated. */
typedef struct {
    uint8_t pstatus;
    uint8_t old_pstatus;
    uint32_t accepted_player_starts[2];
    uint32_t physical_coin_events;
} wm_cabinet_bridge_state;
void wm_cabinet_bridge_init(wm_cabinet_bridge_state *s);
bool wm_cabinet_bridge_accept_player_start(wm_cabinet_bridge_state *s, unsigned player);
uint8_t wm_cabinet_bridge_pstatus(const wm_cabinet_bridge_state *s);
void wm_cabinet_bridge_note_physical_coin(wm_cabinet_bridge_state *s);
#ifdef __cplusplus
}
#endif
#endif
