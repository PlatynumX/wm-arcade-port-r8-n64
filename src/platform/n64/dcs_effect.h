#ifndef WM_N64_DCS_EFFECT_H
#define WM_N64_DCS_EFFECT_H

#include "wm/app.h"
#include "wm/bret_sprites.h"

/* Render the source-translated ADD_PIXEL_ROT / ADD_PIXEL_VEL phases.
   The caller owns the black background and the static dcslogo object. */
void wm_n64_render_dcs_effect(const wm_app *app, const wm_source_sprite *spr);

#endif
