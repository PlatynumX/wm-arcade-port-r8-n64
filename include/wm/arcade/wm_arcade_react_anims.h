#ifndef WM_ARCADE_REACT_ANIMS_H
#define WM_ARCADE_REACT_ANIMS_H

#include "wm/arcade/wm_arcade_react1_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Which per-wrestler table each REACT reaction animation comes out of.
 *
 * The REACT files pick a victim's reaction animation with FACETBL or
 * FACE24TBL over a nine-entry table indexed by WRESTLERNUM, and this
 * port's typed wm_arcade_react1_anim_group_t is the name of that
 * choice. Joining the two is what makes a reaction visible.
 *
 * Every row here is taken from an actual macro use in the source, not
 * from the table names lining up:
 *
 *   HITBLOCK              REACT1.ASM:218, :341, :924   FACETBL
 *   HITBLOCK_FLAIL        REACT1.ASM:954               FACETBL
 *   HEAD_HIT              REACT1.ASM:1078              FACE24TBL
 *   HEAD_HIT2             REACT1.ASM:1151 and 4 more   FACE24TBL
 *   BODY_HIT              REACT1.ASM:1412              FACE24TBL
 *   FALL_BACK             REACT1.ASM:1086 and 9 more   FACETBL
 *   HIT_ON_GROUND         REACT1.ASM:1347, REACT4:134  FACETBL
 *   FALL_BACK_TBUKL       REACT1.ASM:1593              FACETBL
 *   KNEE_HIT              REACT3.ASM:211               FACE24TBL
 *   KNOCKDOWN             REACT4.ASM:243, :352         FACETBL
 *   BOUNCE_OFF            REACT5.ASM:286               FACE24TBL
 *   SPECIAL_HEAD_HIT2_SAND REACT1.ASM:243              FACETBL
 *   SPECIAL_BODY_HIT2     REACT1.ASM:362               FACETBL
 *   WRES_SLAVE            ANIM.ASM:4590 slaveanim_tbl
 *   LOSE_BALANCE          REACT1.ASM:1060, REACT2.ASM:420, :440,
 *                         REACT5.ASM:202 -- `FACETBL #losebal`
 *
 * The macro matters, not just the table: FACETBL is one long per
 * wrestler and FACE24TBL is two, the second chosen by FACING_DIR. The
 * table carries its own column kind, so wm_roster_anim_facing does the
 * right thing for both and the caller does not have to know which is
 * which.
 *
 * #losebal is a `#local`, written three times -- REACT1.ASM:1102,
 * REACT2.ASM:467, REACT5.ASM:309 -- one beside each of its users. The
 * three name the same animation for every wrestler; REACT1's copy only
 * stops at nine rows where the other two add the Referee. So whichever
 * file a push comes from, the wrestler's row is the same, and
 * tools/wlrostertbl.py emits the table once (the nine-row copy, so no
 * Referee row is claimed). Until that was read, LOSE_BALANCE had no
 * row, and a pushed wrestler got his push velocity (REACT2.ASM:423 and :443,
 * [8,0]) with no animation to end it: in buddy mode Undertaker slid
 * 25000 pixels.
 *
 * NINE GROUPS HAVE NO ROW, and they are left NULL rather than pointed at
 * something plausible: QUICK_KNEE_HIT, SPINKICK_HEAD_HIT,
 * FALL_BACK2, JUMPKICK_HEAD_HIT, BOUNCE_OFF_DIZZY, BACKHAND_HEAD_HIT,
 * EARSLAP_HEAD_HIT, GET_BUZZ and BURN. No FACETBL or FACE24TBL in any
 * REACT file names a global table for them; they select their animation
 * some other way (a `#local` table, which is tools/wlpuppet.py's
 * domain, or a direct label), and that has not been read out yet.
 * wm_react_anim_table_for returns NULL for those, and the caller can
 * tell "no animation for this group" from "no animation for this
 * wrestler".
 *
 * Worth recording while looking at these: head_hit_dizzy_tbl and
 * body_hit_dizzy_tbl are extracted and real, and every FACE24TBL that
 * names them in the shipped source is COMMENTED OUT. They are data with
 * no live reader in the arcade itself.
 */
const char *wm_react_anim_table_for(wm_arcade_react1_anim_group_t group);

/*
 * The label a given wrestler plays for a reaction, or NULL.
 *
 * `facing_dir` is FACING_DIR, used only by the FACE24TBL tables.
 */
const char *wm_react_anim_label(wm_arcade_react1_anim_group_t group,
                                int wrestler, int32_t facing_dir);

#ifdef __cplusplus
}
#endif

#endif
