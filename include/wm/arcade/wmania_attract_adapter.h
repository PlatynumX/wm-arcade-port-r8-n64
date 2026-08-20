#ifndef WMANIA_ATTRACT_ADAPTER_H
#define WMANIA_ATTRACT_ADAPTER_H

#include "wm/arcade/wmania_attract_core.h"
#include "wm/arcade/wmania_attract_operator.h"
#include "wm/arcade/wmania_attract_time.h"
#include "wm/arcade/wmania_hiscore_system.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *user;

    /*
     * Existing already-ported frontend screens. They remain callbacks here
     * so this handoff does not duplicate the DCS/Midway/title port.
     */
    void (*show_dcs_logo)(void *user);
    void (*show_sports_logo)(void *user);
    void (*show_title)(void *user);

    /*
     * Explicitly unimplemented by this bundle. The default is a no-op.
     * Both source gameplay-demo call sites invoke this callback if provided.
     */
    void (*show_gameplay_demo_unimplemented)(
        void *user,
        unsigned source_demo_slot);

    /*
     * creditscreen in ATTRACT.ASM is a thin wrapper around the external
     * shared system CRD_SCRN2. This callback is that exact dependency,
     * not an invented replacement credit roll.
     */
    void (*show_credits_crd_scrn2)(void *user);

    /* Source-specific translated screens/data models. */
    void (*show_hiscores)(void *user, WmHsSystem *hiscore);
    void (*show_designer_hint)(
        void *user,
        const WmAttractHint *hint);
    void (*show_general_tips)(void *user);
    void (*show_bio)(
        void *user,
        const WmAttractBio *bio,
        bool tips_variant,
        bool play_music);
    void (*show_operator_message)(
        void *user,
        const WmAttractOperatorMessage *message);
    void (*show_time_date)(
        void *user,
        const WmAttractClockText *clock);
    void (*show_copyright)(void *user);
    void (*show_aama)(void *user);

    /* Source external system hooks. */
    void (*remap_io)(void *user);
    void (*play_sound)(void *user, uint16_t sound_id);
    void (*play_wrestler_tune)(void *user, uint16_t tune_id);

    bool (*music_adjustment_nonzero)(void *user);
    bool (*time_date_dip_enabled)(void *user);
    bool (*read_clock)(void *user, WmAttractClock *out);

    /* Resolve source text labels if renderer wants original strings. */
    const char *(*resolve_source_text)(
        void *user,
        const char *source_label);
} WmAttractAdapter;

/*
 * Execute one built cycle synchronously via callbacks.
 * This function never fabricates gameplay; only the two demo callbacks can
 * be invoked for gameplay and may safely be NULL/no-op.
 */
size_t wm_attract_run_cycle(
    WmAttractState *state,
    WmHsSystem *hiscore,
    const WmAttractOperatorMessage *operator_message,
    const WmAttractAdapter *adapter);

#ifdef __cplusplus
}
#endif

#endif
