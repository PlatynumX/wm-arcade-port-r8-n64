#ifndef WM_ARCADE_DRONE_DATA_H
#define WM_ARCADE_DRONE_DATA_H

#include "wm/arcade/wm_arcade_drone.h"
#include "wm/arcade/wmania_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DRONE.ASM's real AI decision data for Bret Hart -- bret_s_t/bret_m_t/
 * bret_l_t (the short/medium/long-range mode-dispatch lists), brM_n/
 * brMm_n/brM_hh/brM_hhr (Bret's own normal/aggressive/headhold/headhold-
 * reversal script lists), and every primitive action script they and the
 * shared M_og/Mm_og/M_opptbkl/M_shrtblkr/M_shrtblkr[dl] lists transitively
 * reference -- plus the shared, wrestler-agnostic drn_* routines (drone_
 * chkrun, drone_chrg, drn_seek, drn_seekclose, drn_retreat, drn_run,
 * drn_oprun, drn_roll, drn_climbtb/drn_ontb, drn_inair, drn_opinair,
 * drn_enterring, drn_taunt, drn_oppdead, drone_seekdirdist and its drone_
 * seek/seek2/seekxz/getxz helpers, drn_combo) that DRONE.ASM's own script
 * interpreter runs as inline "DS_CODE" native-code blocks.
 *
 * wm_arcade_drone_main()/wm_arcade_drone_script_step() (wm_arcade_drone.h)
 * already implement the *generic*, wrestler-agnostic decision engine and
 * bytecode interpreter -- this file supplies the real *data* and the real
 * DS_CODE callback bodies it was built to run, but was never given.
 *
 * Only Bret Hart (wrestler_num==WM_ROSTER_BRET) has a real move/animation
 * backend in this port (wm_arcade_move_bret / wm_bret_backend_*), so the
 * callbacks here only ever hand out real *attack content* -- range_script_
 * list()'s lists, drn_combo's and drone_chrg's per-wrestler branch, and
 * drn_taunt's per-wrestler taunt anim -- when the drone-controlled actor
 * itself is Bret; for any other wrestler_num those same seams return NULL/
 * no-op (source data for the other seven wrestlers' own script tables is
 * not needed by this port and is not transcribed here), matching this
 * port's existing Bret-only backend boundary. The rest of the engine --
 * positioning, blocking, getup timing, the shared drn_* movement/seek
 * scripts -- is genuine source behavior and runs for any wrestler pairing.
 */
wm_arcade_drone_callbacks_t wm_arcade_drone_data_callbacks(WmRng *rng);

#ifdef __cplusplus
}
#endif

#endif
