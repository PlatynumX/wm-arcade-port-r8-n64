#include "wmania_hiscore_adapter.h"

void wm_hs_adapter_dispatch_entry_events(
    const WmHsPortAdapter *adapter,
    const WmHsEntryState *state,
    uint32_t events,
    bool tjm_voice_variant)
{
    uint16_t move;
    uint16_t select;

    if (adapter == 0 || adapter->play_sound == 0 || state == 0) {
        return;
    }

    move = state->player_index == 0u ?
        WM_HS_SOUND_MOVE_P1 : WM_HS_SOUND_MOVE_P2;
    select = state->player_index == 0u ?
        WM_HS_SOUND_SELECT_P1 : WM_HS_SOUND_SELECT_P2;

    if ((events & WM_HS_ENTRY_EVENT_COUNTDOWN) != 0u) {
        adapter->play_sound(adapter->user, WM_HS_SOUND_COUNTDOWN);
    }
    if ((events & WM_HS_ENTRY_EVENT_MOVE) != 0u) {
        adapter->play_sound(adapter->user, move);
    }
    if ((events & WM_HS_ENTRY_EVENT_ADD) != 0u) {
        adapter->play_sound(adapter->user, WM_HS_SOUND_ADD_INITIAL);
    }
    if ((events & WM_HS_ENTRY_EVENT_SELECT) != 0u) {
        adapter->play_sound(adapter->user, select);
    }
    if ((events & WM_HS_ENTRY_EVENT_TJM_VOICE) != 0u) {
        adapter->play_sound(
            adapter->user,
            tjm_voice_variant ? WM_HS_VOICE_TJM_B : WM_HS_VOICE_TJM_A);
    }
    if ((events & WM_HS_ENTRY_EVENT_SMJ_VOICE) != 0u) {
        adapter->play_sound(adapter->user, WM_HS_VOICE_SMJ);
    }
}

void wm_hs_adapter_play_post_entry(const WmHsPortAdapter *adapter)
{
    if (adapter != 0 && adapter->play_sound != 0) {
        adapter->play_sound(adapter->user, WM_HS_SOUND_POST_ENTRY);
    }
}
