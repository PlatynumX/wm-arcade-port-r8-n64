#ifndef WM_ARCADE_SMOVE_RUNTIME_H
#define WM_ARCADE_SMOVE_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include "wm_arcade_roster.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WM_ARCADE_SMOVE_MAX_PROCS 64u
#define WM_ARCADE_SMOVE_TIMEOUT_KEEP 0xffffu

typedef enum wm_arcade_smove_wait_result {
    WM_SMOVE_WAIT_STILL_WAITING = 0,
    WM_SMOVE_WAIT_ADVANCED = 1,
    WM_SMOVE_WAIT_RESET = 2
} wm_arcade_smove_wait_result_t;

typedef struct wm_arcade_smove_wait_step {
    uint16_t expected;
    uint16_t ignore_mask;
    uint16_t load_timeout; /* WM_ARCADE_SMOVE_TIMEOUT_KEEP means keep A11. */
} wm_arcade_smove_wait_step_t;

typedef struct wm_arcade_smove_entry {
    wm_arcade_roster_id_t wrestler;
    const char *process_label;
    const char *result_label;
    const wm_arcade_smove_wait_step_t *steps;
    uint8_t step_count;
    uint8_t gate_kind;
    uint8_t post_fire_sleep;
    uint8_t source_exact_body;
} wm_arcade_smove_entry_t;

typedef struct wm_arcade_smove_proc {
    uint8_t active;
    uint8_t unresolved;
    uint8_t owner_slot;
    uint8_t step_index;
    uint8_t timeout_loaded_for;
    uint8_t sleep_ticks;
    uint16_t timeout;
    uint32_t fires;
    const wm_arcade_wrestler_profile_t *profile;
    const char *process_label;
    const wm_arcade_smove_entry_t *entry;
} wm_arcade_smove_proc_t;

typedef struct wm_arcade_smove_runtime {
    wm_arcade_smove_proc_t proc[WM_ARCADE_SMOVE_MAX_PROCS];
    size_t proc_count;
    uint32_t created;
    uint32_t unresolved_created;
    uint32_t reset_count;
    uint32_t kill_count;
    uint32_t fire_count;
} wm_arcade_smove_runtime_t;

typedef struct wm_arcade_smove_callbacks {
    uintptr_t (*resolve_label_token)(const char *source_label, void *user);
    void (*sound_label)(wm_arcade_actor_t *actor, const char *source_label, void *user);
    int (*check_combo_go)(wm_arcade_actor_t *actor, void *user);
    void (*find_and_kill_endless)(wm_arcade_actor_t *actor, void *user);
    void (*do_reversal)(wm_arcade_actor_t *actor, void *user);
    void (*do_reversal_message)(wm_arcade_actor_t *actor, void *user);
    void (*bonus_message)(wm_arcade_actor_t *actor, int bonus, void *user);
    int (*ck_ignore)(wm_arcade_actor_t *actor, void *user);
    void *user;
} wm_arcade_smove_callbacks_t;

void wm_arcade_smove_runtime_init(wm_arcade_smove_runtime_t *rt);
size_t wm_arcade_smove_init_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    wm_arcade_actor_t *owner,
    uint8_t owner_slot,
    const wm_arcade_wrestler_profile_t *profile);
void wm_arcade_smove_reset_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    const wm_arcade_actor_t *owner);
void wm_arcade_smove_kill_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    const wm_arcade_actor_t *owner);
/* WRESTLE2.ASM::init_smoves creates each watchdog with GETPRC_INSERT,
 * immediately before its owning WMAIN.  This owner-scoped dispatcher preserves
 * that process-list interleave. */
void wm_arcade_smove_runtime_tick_owner(
    wm_arcade_smove_runtime_t *rt,
    uint8_t owner_slot,
    wm_arcade_actor_t **actors,
    size_t actor_count,
    const wm_arcade_smove_callbacks_t *callbacks);

/* Whole-bank helper retained for non-WRESTLE callers/tests. */
void wm_arcade_smove_runtime_tick(
    wm_arcade_smove_runtime_t *rt,
    wm_arcade_actor_t **actors,
    size_t actor_count,
    const wm_arcade_smove_callbacks_t *callbacks);

wm_arcade_smove_wait_result_t wm_arcade_smove_waitswitch_down(
    const wm_arcade_actor_t *actor,
    uint16_t expected,
    uint16_t ignore_mask,
    uint16_t *timeout_io);

const wm_arcade_smove_entry_t *wm_arcade_smove_lookup_entry(
    wm_arcade_roster_id_t wrestler,
    const char *process_label);
int wm_arcade_smove_label_source_enabled(
    wm_arcade_roster_id_t wrestler,
    const char *process_label);
size_t wm_arcade_smove_manifest_count(void);
const wm_arcade_smove_entry_t *wm_arcade_smove_manifest_entry(size_t index);

#ifdef __cplusplus
}
#endif
#endif
