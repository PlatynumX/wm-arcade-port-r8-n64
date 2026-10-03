#include "wm/arcade/wm_arcade_react_anims.h"
#include "wm/arcade/wm_arcade_roster_anims.h"
#include "wm/arcade/wm_arcade_combat_defs.h"

#include <stddef.h>

const char *wm_react_anim_table_for(wm_arcade_react1_anim_group_t group) {
    switch (group) {
    case WM_R1_ANIM_HITBLOCK:              return "hitblock_tbl";
    case WM_R1_ANIM_HITBLOCK_FLAIL:        return "hitblock_flail_tbl";
    case WM_R1_ANIM_HEAD_HIT:              return "head_hit_tbl";
    case WM_R1_ANIM_HEAD_HIT2:             return "head_hit2_tbl";
    case WM_R1_ANIM_BODY_HIT:              return "body_hit_tbl";
    case WM_R1_ANIM_FALL_BACK:             return "fall_back_tbl";
    case WM_R1_ANIM_HIT_ON_GROUND:         return "hitonground_tbl";
    case WM_R1_ANIM_FALL_BACK_TBUKL:       return "fall_back_tbukl_tbl";
    case WM_R1_ANIM_KNEE_HIT:              return "knee_hit_tbl";
    case WM_R1_ANIM_KNOCKDOWN:             return "knockdwn";
    case WM_R1_ANIM_BOUNCE_OFF:            return "bncoff";
    case WM_R1_ANIM_SPECIAL_HEAD_HIT2_SAND: return "head_hit2_sand_tbl";
    case WM_R1_ANIM_SPECIAL_BODY_HIT2:     return "body_hit2_tbl";
    case WM_R1_ANIM_WRES_SLAVE:            return "slaveanim_tbl";
    /* See the header: no global table names these, so none is guessed. */
    case WM_R1_ANIM_LOSE_BALANCE:
    case WM_R1_ANIM_QUICK_KNEE_HIT:
    case WM_R1_ANIM_SPINKICK_HEAD_HIT:
    case WM_R1_ANIM_FALL_BACK2:
    case WM_R1_ANIM_JUMPKICK_HEAD_HIT:
    case WM_R1_ANIM_BOUNCE_OFF_DIZZY:
    case WM_R1_ANIM_BACKHAND_HEAD_HIT:
    case WM_R1_ANIM_EARSLAP_HEAD_HIT:
    case WM_R1_ANIM_GET_BUZZ:
    case WM_R1_ANIM_BURN:
    default:                               return NULL;
    }
}

const char *wm_react_anim_label(wm_arcade_react1_anim_group_t group,
                                int wrestler, int32_t facing_dir) {
    const char *name = wm_react_anim_table_for(group);
    const wm_roster_anim_table *table;
    if (!name) return NULL;
    table = wm_roster_anim_find(name);
    if (!table) return NULL;
    /*
     * FACE24TBL's own rule: column 0 when MOVE_UP_BIT is SET in
     * FACING_DIR, column 1 when it is clear. FACETBL tables ignore the
     * facing entirely, and wm_roster_anim_facing knows which kind it
     * was handed, so the bit is computed here and the table decides
     * whether it matters.
     */
    return wm_roster_anim_facing(table, wrestler,
                                 (facing_dir & (int32_t)WM_MOVE_UP) != 0);
}
