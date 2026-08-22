#include "wm_arcade_roster.h"

/* The 32-entry action selector is source-identical across the wrestler modules.
 * It is exported only as a diagnostic/reference utility. Each dedicated wrestler
 * module retains its own local action table and does not call this function. */
enum roster_action { A_NONE=0, A_PUNCH, A_BLOCK, A_SPUNCH, A_KICK, A_PUNCHKICK, A_SKICK, A_GRABOH };
static const uint8_t action_table[32] = {
 A_NONE,A_PUNCH,A_BLOCK,A_BLOCK,A_SPUNCH,A_SPUNCH,A_BLOCK,A_BLOCK,
 A_KICK,A_PUNCHKICK,A_BLOCK,A_BLOCK,A_SPUNCH,A_PUNCHKICK,A_BLOCK,A_BLOCK,
 A_SKICK,A_SKICK,A_BLOCK,A_BLOCK,A_GRABOH,A_GRABOH,A_BLOCK,A_BLOCK,
 A_SKICK,A_PUNCHKICK,A_BLOCK,A_BLOCK,A_GRABOH,A_GRABOH,A_BLOCK,A_BLOCK
};
uint8_t wm_arcade_roster_action_for_buttons(uint16_t b) { return action_table[b & WM_BTN_ATTACK_MASK]; }

const wm_arcade_wrestler_profile_t *wm_arcade_roster_profile(wm_arcade_roster_id_t id)
{
    switch (id) {
    case WM_ROSTER_BRET: return &wm_arcade_profile_bret;
    case WM_ROSTER_RAZOR: return &wm_arcade_profile_razor;
    case WM_ROSTER_TAKER: return &wm_arcade_profile_taker;
    case WM_ROSTER_YOKO: return &wm_arcade_profile_yoko;
    case WM_ROSTER_SHAWN: return &wm_arcade_profile_shawn;
    case WM_ROSTER_BAM: return &wm_arcade_profile_bam;
    case WM_ROSTER_DOINK: return &wm_arcade_profile_doink;
    case WM_ROSTER_LEX: return &wm_arcade_profile_lex;
    default: return NULL;
    }
}
