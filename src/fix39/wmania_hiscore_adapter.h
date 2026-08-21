#ifndef WMANIA_HISCORE_ADAPTER_H
#define WMANIA_HISCORE_ADAPTER_H

#include "wmania_hiscore_entry.h"
#include "wmania_hiscore_persist.h"
#include "wmania_hiscore_present.h"
#include "wmania_hiscore_system.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is the only layer the N64 port should need to implement.
 * Nothing below assumes libdragon SRAM/FlashRAM/Controller Pak policy,
 * controller button mapping, a particular font renderer, or an audio engine.
 */
typedef struct {
    void *user;

    /*
     * RNDRNG0-compatible callback: result must be 0..max inclusive.
     * Use wm_rng_rndrng0_callback from wmania_rng.h.
     */
    WmHsRandomRangeFn random_range;
    void (*play_sound)(void *user, uint16_t arcade_sound_id);

    /*
     * Renderer consumes already-decoded table rows. Asset symbol names
     * retained in descriptors help map to ported arcade art.
     */
    void (*begin_screen)(
        void *user, const WmHsPresentDescriptor *descriptor);
    void (*draw_rows)(
        void *user,
        const WmHsPresentDescriptor *descriptor,
        const WmHsDisplayRow *rows,
        size_t row_count);
    void (*end_screen)(void *user, const WmHsPresentDescriptor *descriptor);

    WmHsSaveBackend save;
} WmHsPortAdapter;

/* Source sound IDs used by the initials/high-score path. */
enum {
    WM_HS_SOUND_ADD_INITIAL = 0xB1,
    WM_HS_SOUND_MOVE_P1 = 0xC7,
    WM_HS_SOUND_MOVE_P2 = 0xC8,
    WM_HS_SOUND_SELECT_P1 = 0xCB,
    WM_HS_SOUND_SELECT_P2 = 0xCC,
    WM_HS_SOUND_COUNTDOWN = 0x0A,
    WM_HS_SOUND_POST_ENTRY = 0xB8,
    WM_HS_VOICE_TJM_A = 0x129,
    WM_HS_VOICE_TJM_B = 0x12A,
    WM_HS_VOICE_SMJ = 0x214
};

/* Translate WmHsEntryEvent bits to source sound IDs. */
void wm_hs_adapter_dispatch_entry_events(
    const WmHsPortAdapter *adapter,
    const WmHsEntryState *state,
    uint32_t events,
    bool tjm_voice_variant);

/* Caller uses this after the source's 30-tick post-selection wait. */
void wm_hs_adapter_play_post_entry(const WmHsPortAdapter *adapter);

#ifdef __cplusplus
}
#endif

#endif
