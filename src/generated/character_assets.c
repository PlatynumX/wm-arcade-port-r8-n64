/* Combat2ES R14D: generated-asset bridge restored.
 * Keeps wm_character_visual()/wm_character_sprite_find() defined instead of
 * removing real generated source references from CMake.
 *
 * Roster 0 returns the existing generated Bret sequences.  Other roster slots
 * return roster-distinct bridge sequences so the frontend/demo layer does not
 * collapse every wrestler to Bret while the missing per-wrestler generated
 * visual units are restored in a later asset-source pass.  Combat source parity
 * does not depend on these frontend visual-sequence bridge objects.
 */
#include "wm/character_assets.h"
#include "wm/bret_visuals.h"
#include "wm/progress_wrestlers.h"

static const wm_visual_sequence *const bret_visuals[WM_CV_COUNT] = {
    &wm_bret_stand2_anim,
    &wm_bret_stand4_anim,
    &wm_bret_torso2_anim,
    &wm_bret_torso4_anim,
    &wm_bret_walk2_f2_anim,
    &wm_bret_walk8_f2_anim,
    &wm_bret_walk4_f4_anim,
    &wm_bret_walk6_f4_anim,
    &wm_bret_run_anim,
    &wm_bret_light_punch2_anim,
    &wm_bret_light_punch4_anim,
    &wm_bret_power_punch_anim,
    &wm_bret_light_kick2_anim,
    &wm_bret_light_kick4_anim,
    &wm_bret_power_kick_anim,
};

static const wm_visual_frame fallback_frame[] = {
    {"SOURCE_VISUAL_PENDING", 1},
};

static const wm_visual_sequence fallback_visuals[9][WM_CV_COUNT] = {
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster0_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster1_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster2_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster3_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster4_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster5_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster6_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster7_PK", fallback_frame, 1, true},
    },
    {
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_STAND2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_STAND4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_TORSO2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_TORSO4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_WALK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_WALK8", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_WALK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_WALK6", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_RUN", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_LP2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_LP4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_PP", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_LK2", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_LK4", fallback_frame, 1, true},
        {"COMBAT2ES_R14D_GENERATED_BRIDGE", "roster8_PK", fallback_frame, 1, true},
    },
};

const wm_visual_sequence *wm_character_visual(uint8_t roster_id,
                                              wm_character_visual_slot slot)
{
    if ((unsigned)slot >= WM_CV_COUNT)
        return 0;
    if (roster_id == 0u)
        return bret_visuals[(unsigned)slot];
    if (roster_id < 9u)
        return &fallback_visuals[roster_id][(unsigned)slot];
    return 0;
}

const wm_source_sprite *wm_character_sprite_find(uint8_t roster_id,
                                                 const char *source_frame)
{
    (void)roster_id;
    return wm_progress_sprite_find(source_frame);
}

const wm_source_sprite *wm_character_base_sprite(uint8_t roster_id)
{
    const wm_progress_anim *anim = wm_progress_anim_get(roster_id,
        WM_PROGRESS_ACT_STAND, false);
    if (anim != 0 && anim->frames != 0 && anim->frame_count != 0)
        return wm_progress_sprite_find(anim->frames[0].source_frame);
    return 0;
}

size_t wm_character_sprite_count(uint8_t roster_id)
{
    (void)roster_id;
    return wm_progress_sprite_count();
}
