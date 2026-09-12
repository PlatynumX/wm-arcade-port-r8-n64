/* DCSSOUND.ASM's announcer queue -- see wm/arcade/wm_arcade_announcer.h. */
#include "wm/arcade/wm_arcade_announcer.h"
#include <string.h>

/*
 * The upper bound ADD_VOICE range-checks against is
 * (triple_end-triple_sndtab)/32 -- the number of entries in the sound
 * table. This port passes indexes through to its own audio queue without
 * a table of its own, so the bound is the widest a WORD index can be;
 * what matters, and is kept, is that the check exists and that a refusal
 * is reported rather than silently swallowed.
 */
#define WM_ANNOUNCE_MAX_CALL 0x7FFFu

void wm_announcer_init(wm_announcer_state *a) {
    if (a) memset(a, 0, sizeof(*a));
}

bool wm_announcer_is_silent(const wm_announcer_state *a) {
    if (!a) return false;
    return a->next == a->current && a->busy == 0;
}

bool wm_announcer_add(wm_announcer_state *a, uint16_t call) {
    if (!a) return false;
    if (call > WM_ANNOUNCE_MAX_CALL) return false;   /* OUT_OF_RANGE_SOUND */
    a->slot[a->next] = call;
    a->next = (uint8_t)((a->next + 1u) % WM_ANNOUNCE_QUEUE_SLOTS);
    return true;
}

bool wm_announcer_add_if_silent(wm_announcer_state *a, uint16_t call) {
    if (!wm_announcer_is_silent(a)) return false;    /* NO_ADD, returns -1 */
    return wm_announcer_add(a, call);
}

bool wm_announcer_tick(wm_announcer_state *a, void *user,
                       void (*sound)(void *user, uint16_t call),
                       bool *is_voice) {
    uint16_t call;

    if (!a) return false;
    if (a->busy) { --a->busy; return false; }
    if (a->next == a->current) return false;         /* NOTHING_TO_DO_NOW */

    call = a->slot[a->current];
    a->current = (uint8_t)((a->current + 1u) % WM_ANNOUNCE_QUEUE_SLOTS);

    /* Both sides of DCSSOUND.ASM:2880's 0E0h split reach the same seam
       here; FIND_AND_KILL_ENDLESS runs before either in the source and is
       already translated as its own ANI_CODE routine. */
    if (is_voice) *is_voice = (call >= WM_ANNOUNCE_VOICE_FIRST);
    if (sound) sound(user, call);
    a->busy = a->busy_ticks;
    return true;
}
