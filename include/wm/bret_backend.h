#ifndef WM_BRET_BACKEND_H
#define WM_BRET_BACKEND_H

#include "wm/arcade/wm_arcade_bret.h"
#include "wm/visual.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The "merge adapter" wm_arcade_bret.h's own comment anticipates: resolves
 * BRET.ASM's wm_arcade_bret_anim_id_t tokens to the native wm_visual_sequence
 * data already extracted from the arcade assets (src/generated/bret_visuals.c,
 * bret_attacks.c), and drives two wm_visual_state tracks (body + torso) from
 * wm_arcade_bret_callbacks_t.
 *
 * Coverage is deliberately partial, matching what tools/wlanim.py has
 * actually extracted from HRTSEQ1-4.ASM so far:
 *   - WM_BRET_ANIM_STAND2/STAND4/TORSO2/TORSO4 (idle stance + torso)
 *   - WM_BRET_ANIM_PUNCH2/PUNCH4 (light punch, HRTSEQ2.ASM hrt_2/4_punch_anim)
 *   - WM_BRET_ANIM_SUPER_PUNCH2_4 (power punch, hrt_4_super_punch_anim --
 *     hrt_2_super_punch_anim does not exist in the source tree, so
 *     WM_BRET_ANIM_SUPER_PUNCH2_2/SUPER_PUNCH4 stay unmapped)
 *   - WM_BRET_ANIM_KICK2/KICK4 (light kick, hrt_2/4_kick_anim)
 *   - WM_BRET_ANIM_SUPER_KICK2 (power kick, hrt_2_super_kick_anim --
 *     hrt_4_super_kick_anim exists in HRTSEQ2.ASM:1335 but has not been
 *     extracted yet, so WM_BRET_ANIM_SUPER_KICK4 stays unmapped)
 *
 * Every other wm_arcade_bret_anim_id_t (grapples, pins, finishers, running
 * moves, turnbuckle moves, ...) resolves to NULL and wm_bret_backend_change_*
 * no-ops for it rather than substituting a placeholder animation -- BRET.ASM
 * itself still picks the correct id; only its visual result is missing.
 *
 * wm_arcade_bret_callbacks_t.execute_walk is NOT implemented here.
 * WRESTLE.ASM::execute_walk (a shared, non-Bret-specific routine) drives both
 * movement velocity AND the idle "stance" animation reselection every tick
 * through set_rotate_anim/change_anim1 and a per-wrestler rotate-anim table;
 * porting it is its own separate step. Until it exists, a Bret actor's
 * sprite stays on whatever wm_bret_backend_ani_init set (correct for a
 * standing, non-moving, non-attacking wrestler, which is what the current
 * drone AI -- see wm/match.h -- always produces).
 */

typedef struct {
    wm_visual_state visual;
    wm_visual_state torso_visual;
} wm_bret_backend_actor;

void wm_bret_backend_init(wm_bret_backend_actor *bva);

/* NULL for any wm_arcade_bret_anim_id_t not listed above. */
const wm_visual_sequence *wm_bret_anim_sequence(wm_arcade_bret_anim_id_t id);

/* wm_arcade_bret_callbacks_t.change_anim body. */
void wm_bret_backend_change_anim(wm_arcade_actor_t *actor,
                                 wm_arcade_bret_anim_id_t id, void *user);
/* wm_arcade_bret_callbacks_t.change_torso_anim body. */
void wm_bret_backend_change_torso_anim(wm_arcade_actor_t *actor,
                                       wm_arcade_bret_anim_id_t id, void *user);

/* Builds a callbacks struct with only change_anim/change_torso_anim/user
   populated; every other BRET.ASM callback (execute_walk, sound, secret
   moves, ...) is intentionally left NULL -- see the file comment. */
wm_arcade_bret_callbacks_t wm_bret_backend_callbacks(wm_bret_backend_actor *bva);

/* Advances both visual tracks by one source tick. */
void wm_bret_backend_tick(wm_bret_backend_actor *bva);

#ifdef __cplusplus
}
#endif

#endif
