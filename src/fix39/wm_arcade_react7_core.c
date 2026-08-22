#include "wm_arcade_react7_core.h"

void wm_arcade_react7_att30_stub(void) { }
void wm_arcade_react7_att31_stub(void) { }
void wm_arcade_react7_att32_stub(void) { }
void wm_arcade_react7_att33_stub(void) { }
void wm_arcade_react7_att34_stub(void) { }

void wm_arcade_react1234567_reaction_callback(wm_arcade_actor_t *a,
                                              wm_arcade_actor_t *v,
                                              wm_arcade_reaction_id_t r,
                                              int16_t *pending,
                                              int16_t *newdir,
                                              void *user)
{
    /* REACT6/7 stubs are not referenced by the current hit_table. */
    wm_arcade_react12345_reaction_callback(a, v, r, pending, newdir, user);
}
