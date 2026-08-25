#include "wm_arcade_smove_runtime.h"

#include <string.h>

#define STEP(e,i,l) {(uint16_t)(e),(uint16_t)(i),(uint16_t)(l)}

/* DAMAGE.EQU AT_LEAPING is 57; keep it local to avoid colliding with
   wm_arcade_damage.h's enum member of the same source name. */
#define WM_SMOVE_AT_LEAPING 57

enum {
    G_NONE = 0,
    G_BRET_CHARGE_FACE_RAKE,
    G_BRET_CHARGE_FLYING_KICK,
    G_BRET_ROLL_UPPERCUT,
    G_BRET_HD_PILE,
    G_BRET_HD_DDT,
    G_BRET_HD_FACESLAM,
    G_BRET_GRAB_TOSS_AIR,
    G_BRET_HD_COMBO1,
    G_BRET_HD_COMBO2,
    G_RAZOR_CHARGE_SLASHES,
    G_RAZOR_HD_PILE,
    G_RAZOR_HD_COMBO1,
    G_RAZOR_HD_EDGE,
    G_RAZOR_HD_RUG,
    G_RAZOR_GRAB_TOSS_AIR,
    G_RAZOR_HD_COMBO2,
    G_RAZOR_SLIDING_RUG,
    G_YOKO_HD_COMBO1,
    G_YOKO_HD_SCISSOR,
    G_YOKO_HD_SUPLEX,
    G_YOKO_SALT_THROW,
    G_YOKO_GRAB_TOSS_AIR,
    G_YOKO_HD_COMBO2,
    G_SHAWN_HD_COMBO2,
    G_SHAWN_HD_COMBO1,
    G_SHAWN_CHARGE_SUPLEX,
    G_SHAWN_SWIRL_SPEEDKICK,
    G_SHAWN_SLIDING_KICKTOSS,
    G_SHAWN_HD_SUPLEX,
    G_SHAWN_HD_FRANK,
    G_SHAWN_HD_KICKTOSS,
    G_SHAWN_HD_BUTTS,
    G_SHAWN_FLIPSLAM,
    G_SHAWN_GRAB_TOSS_AIR,
    G_BAM_CHARGE_NECKBREAKER,
    G_BAM_HD_COMBO1,
    G_BAM_HD_PILE,
    G_BAM_HD_POGO,
    G_BAM_HD_COMBO2,
    G_BAM_GRAB_TOSS_AIR,
    G_DOINK_CHARGE_FLYKICK,
    G_DOINK_HD_SLAM,
    G_DOINK_HD_COMBO1,
    G_DOINK_HD_PILE,
    G_DOINK_HD_COMBO2,
    G_DOINK_HD_BUZZ,
    G_DOINK_GRAB_TOSS_AIR,
    G_LEX_HD_PILE,
    G_LEX_HD_ELBOW_FACE,
    G_LEX_HD_GRABOH,
    G_LEX_GRAB_TOSS_AIR,
    G_LEX_HD_COMBO1,
    G_LEX_HD_COMBO2,
    G_TAKER_HD_NECK,
    G_TAKER_HD_FACESLAM,
    G_TAKER_HD_PILE,
    G_TAKER_CHOKE_SLIDE,
    G_TAKER_SPIRIT_PUSH,
    G_TAKER_SPIRIT_PULL,
    G_TAKER_GRAB_TOSS_AIR,
    G_TAKER_COMBO1,
    G_TAKER_COMBO2,
    G_TAKER_FINISH1
};


/* BRET.ASM hrt_smove_table bodies translated in Combat2ES R15. */
static const wm_arcade_smove_wait_step_t hrt_roll_uppercut[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_UP | WM_J_DOWN, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};

static const wm_arcade_smove_wait_step_t hrt_hdhold_pile[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t hrt_hdhold_ddt[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t hrt_hdhold_faceslam[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_DOWN | WM_J_UP, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t hrt_grab_toss_air[] = {
    STEP(WM_J_AWAY, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, 0, 40),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t hrt_hdhold_combo1[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t hrt_hdhold_combo2[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SKICK, WM_J_DOWN_TOWARD | WM_J_UP_TOWARD, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};

static const wm_arcade_smove_wait_step_t rzr_sliding_rug[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t rzr_hdhold_pile[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t rzr_hdhold_edge[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t rzr_hdhold_rug[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t rzr_grab_toss_air[] = {
    STEP(WM_J_AWAY, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, 0, 40),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t rzr_hdhold_combo1[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t rzr_hdhold_combo2[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};


static const wm_arcade_smove_wait_step_t yok_hdhold_combo1[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t yok_hdhold_scissor[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t yok_hdhold_suplex[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t yok_salt_throw[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_DOWN | WM_J_UP, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t yok_grab_toss_air[] = {
    STEP(WM_J_AWAY, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, 0, 40),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t yok_hdhold_combo2[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};


/* Combat2ES/R21 shared WAITSWITCH chains for the remaining active SMOVE bodies.
   The source files use repeated motifs here: toward/toward/button headhold
   monitors, down/down/button headhold monitors, away/away/punch grab-toss-air,
   and down/toward/button motion specials. */
static const wm_arcade_smove_wait_step_t smv_tow_tow_spunch[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t smv_tow_tow_punch[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t smv_tow_tow_kick[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t smv_down_down_skick[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t smv_down_down_kick[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t smv_down_tow_punch[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_DOWN | WM_J_UP, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t smv_down_tow_skick[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_DOWN | WM_J_UP, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t smv_away_away_punch[] = {
    STEP(WM_J_AWAY, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, 0, 40),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};

static const wm_arcade_smove_wait_step_t und_hd_neck[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_hd_faceslam[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_hd_pile[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_choke_slide[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_UP | WM_J_DOWN, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_spirit_push[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_UP | WM_J_DOWN, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_spirit_pull[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, WM_J_UP | WM_J_DOWN, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_grab_toss_air[] = {
    STEP(WM_J_AWAY, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, 0, 40),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_combo1[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_combo2[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_finish1[] = {
    STEP(WM_J_UP, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 53),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};

static const wm_arcade_smove_entry_t manifest[] = {
    { WM_ROSTER_BRET, "hrt_charge_face_rake", "hrt_rake_face_anim", 0, 0, G_BRET_CHARGE_FACE_RAKE, 1, 1 },
    { WM_ROSTER_BRET, "hrt_charge_flying_kick", "hrt_flying_kick_anim", 0, 0, G_BRET_CHARGE_FLYING_KICK, 1, 1 },
    { WM_ROSTER_BRET, "hrt_roll_uppercut", "hrt_roll_uppercut_anim", hrt_roll_uppercut, 3, G_BRET_ROLL_UPPERCUT, 1, 1 },
    { WM_ROSTER_BRET, "hrt_hdhold_pile", "hrt_3_pile_driver_anim", hrt_hdhold_pile, 3, G_BRET_HD_PILE, 20, 1 },
    { WM_ROSTER_BRET, "hrt_hdhold_ddt", "hrt_hh_2_ddt_anim", hrt_hdhold_ddt, 3, G_BRET_HD_DDT, 20, 1 },
    { WM_ROSTER_BRET, "hrt_hdhold_faceslam", "hrt_3_face_driver2_anim", hrt_hdhold_faceslam, 3, G_BRET_HD_FACESLAM, 20, 1 },
    { WM_ROSTER_BRET, "hrt_grab_toss_air", "hrt_hiptoss_anim", hrt_grab_toss_air, 3, G_BRET_GRAB_TOSS_AIR, 20, 1 },
    { WM_ROSTER_BRET, "hrt_hdhold_combo1", "hrt_combo_punch_anim", hrt_hdhold_combo1, 3, G_BRET_HD_COMBO1, 20, 1 },
    { WM_ROSTER_BRET, "hrt_hdhold_combo2", "hrt_combo_kick_anim", hrt_hdhold_combo2, 3, G_BRET_HD_COMBO2, 20, 1 },
    { WM_ROSTER_RAZOR, "rzr_charge_slashes", "rzr_repeat_slash_anim", 0, 0, G_RAZOR_CHARGE_SLASHES, 1, 1 },
    { WM_ROSTER_RAZOR, "rzr_hdhold_pile", "rzr_3_pile_driver_anim", rzr_hdhold_pile, 3, G_RAZOR_HD_PILE, 20, 1 },
    { WM_ROSTER_RAZOR, "rzr_hdhold_combo1", "rzr_combo_punch_anim", rzr_hdhold_combo1, 3, G_RAZOR_HD_COMBO1, 20, 1 },
    { WM_ROSTER_RAZOR, "rzr_hdhold_edge", "rzr_razors_edge_anim", rzr_hdhold_edge, 3, G_RAZOR_HD_EDGE, 20, 1 },
    { WM_ROSTER_RAZOR, "rzr_hdhold_rug", "rzr_rugshake2_anim", rzr_hdhold_rug, 3, G_RAZOR_HD_RUG, 20, 1 },
    { WM_ROSTER_RAZOR, "rzr_grab_toss_air", "rzr_hiptoss_anim", rzr_grab_toss_air, 3, G_RAZOR_GRAB_TOSS_AIR, 20, 1 },
    { WM_ROSTER_RAZOR, "rzr_hdhold_combo2", "rzr_combo_kick_anim", rzr_hdhold_combo2, 3, G_RAZOR_HD_COMBO2, 20, 1 },
    { WM_ROSTER_RAZOR, "rzr_sliding_rug", "rzr_sliding_rug_anim", rzr_sliding_rug, 3, G_RAZOR_SLIDING_RUG, 20, 1 },
    { WM_ROSTER_YOKO, "yok_hdhold_combo1", "yok_combo_jabs_anim", yok_hdhold_combo1, 3, G_YOKO_HD_COMBO1, 20, 1 },
    { WM_ROSTER_YOKO, "yok_hdhold_scissor", "yok_scissor_anim", yok_hdhold_scissor, 3, G_YOKO_HD_SCISSOR, 20, 1 },
    { WM_ROSTER_YOKO, "yok_hdhold_suplex", "yok_vsuplex_anim", yok_hdhold_suplex, 3, G_YOKO_HD_SUPLEX, 20, 1 },
    { WM_ROSTER_YOKO, "yok_salt_throw", "yok_4_salt_anim", yok_salt_throw, 3, G_YOKO_SALT_THROW, 120, 1 },
    { WM_ROSTER_YOKO, "yok_grab_toss_air", "yok_hiptoss_anim", yok_grab_toss_air, 3, G_YOKO_GRAB_TOSS_AIR, 20, 1 },
    { WM_ROSTER_YOKO, "yok_hdhold_combo2", "yok_combo_kick_anim", yok_hdhold_combo2, 3, G_YOKO_HD_COMBO2, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_hdhold_combo2", "shn_combo_kick_anim", smv_tow_tow_kick, 3, G_SHAWN_HD_COMBO2, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_hdhold_combo1", "shn_combo_punch_anim", smv_tow_tow_spunch, 3, G_SHAWN_HD_COMBO1, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_charge_suplex", "shn_vsuplex_anim", 0, 0, G_SHAWN_CHARGE_SUPLEX, 1, 1 },
    { WM_ROSTER_SHAWN, "shn_swirl_speedkick", "shn_swirl_speedkick_anim", smv_down_tow_skick, 3, G_SHAWN_SWIRL_SPEEDKICK, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_sliding_kicktoss", "shn_sliding_kicktoss_anim", smv_tow_tow_kick, 3, G_SHAWN_SLIDING_KICKTOSS, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_hdhold_suplex", "shn_vsuplex_anim", smv_down_down_skick, 3, G_SHAWN_HD_SUPLEX, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_hdhold_frank", "shn_frankensteiner_anim", smv_tow_tow_spunch, 3, G_SHAWN_HD_FRANK, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_hdhold_kicktoss", "shn_kicktoss_anim", smv_down_down_kick, 3, G_SHAWN_HD_KICKTOSS, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_hdhold_butts", "shn_butts_anim", smv_tow_tow_kick, 3, G_SHAWN_HD_BUTTS, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_flipslam", "shn_flipslam_anim", smv_down_tow_punch, 3, G_SHAWN_FLIPSLAM, 20, 1 },
    { WM_ROSTER_SHAWN, "shn_grab_toss_air", "shn_hiptoss_anim", smv_away_away_punch, 3, G_SHAWN_GRAB_TOSS_AIR, 20, 1 },

    { WM_ROSTER_BAM, "bam_charge_neckbreaker", "bam_neckbreaker_anim", 0, 0, G_BAM_CHARGE_NECKBREAKER, 1, 1 },
    { WM_ROSTER_BAM, "bam_hdhold_combo1", "bam_combo_punch_anim", smv_tow_tow_spunch, 3, G_BAM_HD_COMBO1, 20, 1 },
    { WM_ROSTER_BAM, "bam_hdhold_pile", "bam_3_pile_driver_anim", smv_down_down_skick, 3, G_BAM_HD_PILE, 20, 1 },
    { WM_ROSTER_BAM, "bam_hdhold_pogo", "bam_pogo_anim", smv_tow_tow_kick, 3, G_BAM_HD_POGO, 20, 1 },
    { WM_ROSTER_BAM, "bam_hdhold_combo2", "bam_combo_kick_anim", smv_tow_tow_kick, 3, G_BAM_HD_COMBO2, 20, 1 },
    { WM_ROSTER_BAM, "bam_grab_toss_air", "bam_hiptoss_anim", smv_away_away_punch, 3, G_BAM_GRAB_TOSS_AIR, 20, 1 },

    { WM_ROSTER_DOINK, "dnk_charge_flykick", "dnk_flying_kick_anim", 0, 0, G_DOINK_CHARGE_FLYKICK, 1, 1 },
    { WM_ROSTER_DOINK, "dnk_hdhold_slam", "dnk_slam_anim", smv_tow_tow_spunch, 3, G_DOINK_HD_SLAM, 20, 1 },
    { WM_ROSTER_DOINK, "dnk_hdhold_combo1", "dnk_combo_punch_anim", smv_tow_tow_spunch, 3, G_DOINK_HD_COMBO1, 20, 1 },
    { WM_ROSTER_DOINK, "dnk_hdhold_pile", "dnk_3_pile_driver_anim", smv_down_down_skick, 3, G_DOINK_HD_PILE, 20, 1 },
    { WM_ROSTER_DOINK, "dnk_hdhold_combo2", "dnk_combo_kick_anim", smv_tow_tow_kick, 3, G_DOINK_HD_COMBO2, 20, 1 },
    { WM_ROSTER_DOINK, "dnk_hdhold_buzz", "dnk_buzz_anim", smv_tow_tow_kick, 3, G_DOINK_HD_BUZZ, 20, 1 },
    { WM_ROSTER_DOINK, "dnk_grab_toss_air", "dnk_hiptoss_anim", smv_away_away_punch, 3, G_DOINK_GRAB_TOSS_AIR, 20, 1 },

    { WM_ROSTER_LEX, "lex_hdhold_pile", "lex_3_pile_driver_anim", smv_down_down_skick, 3, G_LEX_HD_PILE, 20, 1 },
    { WM_ROSTER_LEX, "lex_hdhold_elbow_face", "lex_elbow_face_anim", smv_tow_tow_punch, 3, G_LEX_HD_ELBOW_FACE, 20, 1 },
    { WM_ROSTER_LEX, "lex_hdhold_graboh", "lex_graboh_anim", smv_tow_tow_spunch, 3, G_LEX_HD_GRABOH, 20, 1 },
    { WM_ROSTER_LEX, "lex_grab_toss_air", "lex_hiptoss_anim", smv_away_away_punch, 3, G_LEX_GRAB_TOSS_AIR, 20, 1 },
    { WM_ROSTER_LEX, "lex_hdhold_combo1", "lex_combo_punch_anim", smv_tow_tow_spunch, 3, G_LEX_HD_COMBO1, 20, 1 },
    { WM_ROSTER_LEX, "lex_hdhold_combo2", "lex_combo_kick_anim", smv_tow_tow_kick, 3, G_LEX_HD_COMBO2, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_neckbrk", "und_neckbreaker_anim", und_hd_neck, 3, G_TAKER_HD_NECK, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_faceslam", "und_choke_face_slam_anim", und_hd_faceslam, 3, G_TAKER_HD_FACESLAM, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_pile", "und_pile_anim", und_hd_pile, 3, G_TAKER_HD_PILE, 20, 1 },
    { WM_ROSTER_TAKER, "und_choke_slide", "und_sliding_choke_anim", und_choke_slide, 3, G_TAKER_CHOKE_SLIDE, 20, 1 },
    { WM_ROSTER_TAKER, "und_spirit_push", "und_spirit_push_anim", und_spirit_push, 3, G_TAKER_SPIRIT_PUSH, 180, 1 },
    { WM_ROSTER_TAKER, "und_spirit_pull", "und_spirit_pull_anim", und_spirit_pull, 3, G_TAKER_SPIRIT_PULL, 180, 1 },
    { WM_ROSTER_TAKER, "und_grab_toss_air", "und_2_snapmirror_anim", und_grab_toss_air, 3, G_TAKER_GRAB_TOSS_AIR, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_combo1", "und_combo_punch_anim", und_combo1, 3, G_TAKER_COMBO1, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_combo2", "und_combo_kick_anim", und_combo2, 3, G_TAKER_COMBO2, 20, 1 },
    { WM_ROSTER_TAKER, "und_finish_move1", "und_finish_move1", und_finish1, 3, G_TAKER_FINISH1, 1, 1 }
};

size_t wm_arcade_smove_manifest_count(void)
{
    return sizeof manifest / sizeof manifest[0];
}

const wm_arcade_smove_entry_t *wm_arcade_smove_manifest_entry(size_t index)
{
    return index < wm_arcade_smove_manifest_count() ? &manifest[index] : 0;
}

int wm_arcade_smove_label_source_enabled(wm_arcade_roster_id_t wrestler,
                                         const char *label)
{
    if (!label) return 0;
    if (strstr(label, "finish_move") != 0) {
        return wrestler == WM_ROSTER_TAKER && strcmp(label, "und_finish_move1") == 0;
    }
    return 1;
}

const wm_arcade_smove_entry_t *wm_arcade_smove_lookup_entry(
    wm_arcade_roster_id_t wrestler,
    const char *process_label)
{
    size_t i;
    if (!process_label) return 0;
    for (i = 0; i < wm_arcade_smove_manifest_count(); ++i) {
        if (manifest[i].wrestler == wrestler &&
            strcmp(manifest[i].process_label, process_label) == 0)
            return &manifest[i];
    }
    return 0;
}

void wm_arcade_smove_runtime_init(wm_arcade_smove_runtime_t *rt)
{
    if (rt) memset(rt, 0, sizeof(*rt));
}

static void proc_rewind(wm_arcade_smove_runtime_t *rt, wm_arcade_smove_proc_t *p,
                        uint8_t sleep_ticks)
{
    if (!p) return;
    p->step_index = 0;
    p->timeout = 0;
    p->timeout_loaded_for = 0xffu;
    p->sleep_ticks = sleep_ticks;
    if (rt) ++rt->reset_count;
}

size_t wm_arcade_smove_init_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    wm_arcade_actor_t *owner,
    uint8_t owner_slot,
    const wm_arcade_wrestler_profile_t *profile)
{
    size_t i, made = 0;
    if (!rt || !owner || !profile) return 0;

    /*
     * Combat2ES R26B:
     * Runtime monitor creation must be driven from the strict source manifest,
     * not from the older profile->special_processes side table.  R25 proved the
     * manifest is the authoritative active SMOVE surface (63 source-exact active
     * bodies), and R26 exposed that the profile table can be stale/incomplete
     * for the zero-step charge monitors and disabled finisher filtering.
     */
    for (i = 0; i < wm_arcade_smove_manifest_count(); ++i) {
        const wm_arcade_smove_entry_t *entry = wm_arcade_smove_manifest_entry(i);
        wm_arcade_smove_proc_t *p;

        if (!entry || entry->wrestler != profile->id)
            continue;
        if (!wm_arcade_smove_label_source_enabled(profile->id, entry->process_label))
            continue;
        if (rt->proc_count >= WM_ARCADE_SMOVE_MAX_PROCS)
            break;

        p = &rt->proc[rt->proc_count++];
        memset(p, 0, sizeof(*p));
        p->active = 1;
        p->owner_slot = owner_slot;
        p->profile = profile;
        p->process_label = entry->process_label;
        p->entry = entry;
        p->unresolved = entry->source_exact_body == 0;
        proc_rewind(0, p, 1);
        ++made;
        ++rt->created;
        if (p->unresolved) ++rt->unresolved_created;
    }

    (void)owner;
    return made;
}


void wm_arcade_smove_reset_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    const wm_arcade_actor_t *owner)
{
    size_t i;
    if (!rt || !owner) return;
    for (i = 0; i < rt->proc_count; ++i) {
        wm_arcade_smove_proc_t *p = &rt->proc[i];
        if (!p->active || p->owner_slot != (uint8_t)owner->player_num) continue;
        proc_rewind(rt, p, 1);
    }
}

void wm_arcade_smove_kill_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    const wm_arcade_actor_t *owner)
{
    size_t i;
    if (!rt || !owner) return;
    for (i = 0; i < rt->proc_count; ++i) {
        wm_arcade_smove_proc_t *p = &rt->proc[i];
        if (!p->active || p->owner_slot != (uint8_t)owner->player_num) continue;
        p->active = 0;
        ++rt->kill_count;
    }
}

wm_arcade_smove_wait_result_t wm_arcade_smove_waitswitch_down(
    const wm_arcade_actor_t *actor,
    uint16_t expected,
    uint16_t ignore_mask,
    uint16_t *timeout_io)
{
    uint16_t timeout;
    uint16_t v;
    if (!actor || !timeout_io) return WM_SMOVE_WAIT_RESET;

    timeout = (uint16_t)(*timeout_io - 1u);
    *timeout_io = timeout;
    if (timeout == 0u) return WM_SMOVE_WAIT_RESET;
    if (actor->special_move_addr != (uintptr_t)0) return WM_SMOVE_WAIT_RESET;

    /* MACROS.H::WAITSWITCH_DWN uses raw (BUT_VAL_DOWN << 4) |
       STICK_REL_NEW, then andni MASK.  Do not pre-mask here. */
    v = (uint16_t)((actor->but_val_down << 4) | actor->stick_rel_new);
    v = (uint16_t)(v & (uint16_t)~ignore_mask);
    if (v == 0u) return WM_SMOVE_WAIT_STILL_WAITING;
    return v == expected ? WM_SMOVE_WAIT_ADVANCED : WM_SMOVE_WAIT_RESET;
}

static wm_arcade_actor_t *opponent_for(wm_arcade_actor_t **actors, size_t n,
                                       wm_arcade_actor_t *a)
{
    size_t i;
    if (!actors || !a) return 0;
    if (a->smart_target) return a->smart_target;
    for (i = 0; i < n; ++i) {
        if (actors[i] && actors[i] != a && actors[i]->active &&
            actors[i]->player_side != a->player_side)
            return actors[i];
    }
    return 0;
}

static int mode_is_headhold(uint16_t mode) { return mode == WM_PMODE_HEADHOLD; }
static int mode_is_headheld(uint16_t mode) { return mode == WM_PMODE_HEADHELD; }
static int mode_is_chokehold(uint16_t mode) { return mode == WM_PMODE_CHOKEHOLD; }
static int mode_is_ground_dead(uint16_t mode) { return mode == WM_PMODE_ONGROUND || mode == WM_PMODE_DEAD; }
static int mode_is_inair(uint16_t mode) { return mode == WM_PMODE_INAIR || mode == WM_PMODE_INAIR2; }

static void queue_result(wm_arcade_actor_t *a, const wm_arcade_smove_entry_t *e,
                         const wm_arcade_smove_callbacks_t *cb)
{
    uintptr_t tok = (uintptr_t)e->result_label;
    if (cb && cb->resolve_label_token)
        tok = cb->resolve_label_token(e->result_label, cb->user);
    a->special_move_addr = tok;
}


static int fire_bret_headhold_body(wm_arcade_actor_t *a, wm_arcade_actor_t *opp,
                                   const wm_arcade_smove_entry_t *e,
                                   const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *target = 0;
    int bonus = 0;
    if (!a || !e) return 0;

    if (a->player_mode != WM_PMODE_HEADHOLD &&
        a->player_mode != WM_PMODE_HEADHELD) return 0;

    if (a->player_mode == WM_PMODE_HEADHELD) {
        /* BRET.ASM reversal branch: reject if I_WILL_DIE, then
           DO_REVERSAL / DO_REVERSAL_MESS and target WHOHITME. */
        if (a->i_will_die != 0) return 0;
        if (a->immobilize_time != 0) return 0;
        if (cb && cb->do_reversal) cb->do_reversal(a, cb->user);
        if (cb && cb->do_reversal_message) cb->do_reversal_message(a, cb->user);
        target = a->who_hit_me ? a->who_hit_me : opp;
        a->smart_target = target;
    } else {
        /* BRET.ASM slam branch: BONUS_MESS score id then target WHOIHIT. */
        if (a->immobilize_time != 0) return 0;
        if (e->gate_kind == G_BRET_HD_PILE) bonus = 35;
        else if (e->gate_kind == G_BRET_HD_DDT) bonus = 16;
        else if (e->gate_kind == G_BRET_HD_FACESLAM) bonus = 20;
        if (bonus != 0 && cb && cb->bonus_message)
            cb->bonus_message(a, bonus, cb->user);
        target = a->who_i_hit ? a->who_i_hit : opp;
        a->smart_target = target;
    }

    if (!target) return 0;
    target->immobilize_time = 15;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);

    if (e->gate_kind == G_BRET_HD_PILE && cb && cb->sound_label)
        cb->sound_label(a, "GRABFLING_T1/GRABFLING_T2", cb->user);

    queue_result(a, e, cb);
    return 1;
}

static int fire_taker_headhold(wm_arcade_actor_t *a, wm_arcade_actor_t *opp,
                               const wm_arcade_smove_entry_t *e,
                               const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *target = 0;
    if (!a) return 0;
    if (mode_is_headheld(a->player_mode)) {
        if (a->i_will_die || a->immobilize_time != 0) return 0;
        target = a->who_hit_me ? a->who_hit_me : opp;
    } else if (mode_is_headhold(a->player_mode) ||
               (e->gate_kind == G_TAKER_HD_FACESLAM && mode_is_chokehold(a->player_mode))) {
        if (a->immobilize_time != 0) return 0;
        target = a->who_i_hit ? a->who_i_hit : opp;
    } else {
        return 0;
    }
    if (!target) return 0;
    target->immobilize_time = 15;
    a->smart_target = target;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label) cb->sound_label(a, "GRABHOLD_T1/GRABHOLD_T2", cb->user);
    return 1;
}


static int fire_bret_charge_face_rake(wm_arcade_smove_proc_t *p,
                                      wm_arcade_actor_t *a,
                                      const wm_arcade_smove_entry_t *e,
                                      const wm_arcade_smove_callbacks_t *cb)
{
    uint16_t charge;
    if (!p || !a || !e) return 0;

    /* BRET.ASM::hrt_charge_face_rake: count while PLAYER_PUNCH is held. */
    if ((a->but_val_cur & WM_BTN_PUNCH) != 0u) {
        if (p->timeout != 0xffffu) ++p->timeout;
        return 0;
    }

    charge = p->timeout;
    p->timeout = 0;
    if (charge < 100u) return 0;

    if (a->getup_time != 0) return 0;
    if (a->player_mode == WM_PMODE_HEADHELD ||
        a->player_mode == WM_PMODE_HEADHOLD ||
        a->player_mode == WM_PMODE_ONGROUND ||
        a->player_mode == WM_PMODE_DEAD) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;

    queue_result(a, e, cb);
    if (cb && cb->sound_label) cb->sound_label(a, "UPRCUT_T1/UPRCUT_T2", cb->user);
    return 1;
}

static int fire_bret_roll_uppercut(wm_arcade_actor_t *a,
                                   const wm_arcade_smove_entry_t *e,
                                   const wm_arcade_smove_callbacks_t *cb)
{
    if (!a || !e) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->immobilize_time != 0) return 0;
    if (a->player_mode == WM_PMODE_ONTURNBKL ||
        a->player_mode == WM_PMODE_HEADHOLD ||
        a->player_mode == WM_PMODE_HEADHELD) return 0;

    if (cb && cb->sound_label) cb->sound_label(a, "GRABFLING_T1/GRABFLING_T2", cb->user);
    queue_result(a, e, cb);
    a->run_time = 0;
    return 1;
}

static int fire_bret_charge_flying_kick(wm_arcade_smove_proc_t *p,
                                        wm_arcade_actor_t *a,
                                        wm_arcade_actor_t *opp,
                                        const wm_arcade_smove_entry_t *e,
                                        const wm_arcade_smove_callbacks_t *cb)
{
    uint16_t charge;
    if (!p || !a || !e) return 0;

    /* BRET.ASM::hrt_charge_flying_kick: count while PLAYER_SKICK is held. */
    if ((a->but_val_cur & WM_BTN_SKICK) != 0u) {
        if (p->timeout != 0xffffu) ++p->timeout;
        return 0;
    }

    charge = p->timeout;
    p->timeout = 0;
    if (charge < 100u) return 0;

    if (a->getup_time != 0) return 0;
    if (a->player_mode == WM_PMODE_HEADHELD ||
        a->player_mode == WM_PMODE_HEADHOLD ||
        a->player_mode == WM_PMODE_ONGROUND ||
        a->player_mode == WM_PMODE_DEAD) return 0;
    if (opp && (opp->player_mode == WM_PMODE_ONGROUND ||
                opp->player_mode == WM_PMODE_DEAD)) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (cb && cb->ck_ignore && cb->ck_ignore(a, cb->user)) return 0;

    queue_result(a, e, cb);
    if (a->player_mode != WM_PMODE_DEAD) a->player_mode = WM_PMODE_INAIR;
    if (cb && cb->sound_label) cb->sound_label(a, "FLYKICK_T1/FLYKICK_T2", cb->user);
    return 1;
}

static int fire_bret_grab_toss_air(wm_arcade_actor_t *a,
                                   wm_arcade_actor_t *opp,
                                   const wm_arcade_smove_entry_t *e,
                                   const wm_arcade_smove_callbacks_t *cb)
{
    const char *label;
    if (!a || !e) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->player_mode == WM_PMODE_HEADHOLD) return 0;
    if (opp && (opp->player_mode == WM_PMODE_ONGROUND ||
                opp->player_mode == WM_PMODE_DEAD)) return 0;

    if (opp && (mode_is_inair(opp->player_mode) || opp->attack_type == WM_SMOVE_AT_LEAPING)) {
        label = "hrt_hiptoss2_anim";
    } else {
        if (a->closest_dist > 0x70) return 0;
        label = "hrt_hiptoss_anim";
    }

    a->special_move_addr = (uintptr_t)label;
    if (cb && cb->resolve_label_token)
        a->special_move_addr = cb->resolve_label_token(label, cb->user);
    if (cb && cb->sound_label) cb->sound_label(a, "HIPTOSS_T1/PUNCH_T2", cb->user);
    return 1;
}

static int fire_bret_headhold_combo(wm_arcade_actor_t *a,
                                    wm_arcade_actor_t *opp,
                                    const wm_arcade_smove_entry_t *e,
                                    const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *target;
    if (!a || !e) return 0;
    if (a->player_mode != WM_PMODE_HEADHOLD &&
        a->player_mode != WM_PMODE_HEADHELD) return 0;
    if (cb && cb->check_combo_go && cb->check_combo_go(a, cb->user) < 0) return 0;
    if (a->immobilize_time != 0) return 0;

    target = a->who_i_hit ? a->who_i_hit : opp;
    a->smart_target = target;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    return 1;
}


static int fire_razor_charge_slashes(wm_arcade_smove_proc_t *p,
                                     wm_arcade_actor_t *a,
                                     const wm_arcade_smove_entry_t *e,
                                     const wm_arcade_smove_callbacks_t *cb)
{
    uint16_t charge;
    if (!p || !a || !e) return 0;

    /* RAZOR.ASM::rzr_charge_slashes:
       count CHARGE_TIME while PLAYER_PUNCH is held, then require >= 100. */
    if ((a->but_val_cur & WM_BTN_PUNCH) != 0u) {
        if (p->timeout != 0xffffu) ++p->timeout;
        return 0;
    }

    charge = p->timeout;
    p->timeout = 0;
    if (charge < 100u) return 0;

    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->getup_time != 0) return 0;
    if (a->player_mode == WM_PMODE_HEADHELD ||
        a->player_mode == WM_PMODE_HEADHOLD ||
        a->player_mode == WM_PMODE_ONGROUND ||
        a->player_mode == WM_PMODE_DEAD) return 0;

    queue_result(a, e, cb);
    if (cb && cb->sound_label) cb->sound_label(a, "KICK_T2", cb->user);
    return 1;
}

static int fire_razor_sliding_rug(wm_arcade_actor_t *a,
                                  wm_arcade_actor_t *opp,
                                  const wm_arcade_smove_entry_t *e,
                                  const wm_arcade_smove_callbacks_t *cb)
{
    if (!a || !e) return 0;

    /* RAZOR.ASM::rzr_sliding_rug:
       reject held/headhold/ground/dead, UNINT, GETUP_TIME, dead opponent,
       and ck_ignore carry before queueing rzr_sliding_rug_anim. */
    if (a->player_mode == WM_PMODE_HEADHELD ||
        a->player_mode == WM_PMODE_HEADHOLD ||
        a->player_mode == WM_PMODE_ONGROUND ||
        a->player_mode == WM_PMODE_DEAD) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->getup_time != 0) return 0;
    if (opp && opp->player_mode == WM_PMODE_DEAD) return 0;
    if (cb && cb->ck_ignore && cb->ck_ignore(a, cb->user)) return 0;

    if (cb && cb->sound_label) cb->sound_label(a, "GRABHOLD_T1/GRABHOLD_T2", cb->user);
    queue_result(a, e, cb);
    return 1;
}


static int fire_razor_headhold_throw(wm_arcade_actor_t *a, wm_arcade_actor_t *opp,
                                     const wm_arcade_smove_entry_t *e,
                                     const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *target = 0;
    int bonus = 0;
    if (!a || !e) return 0;

    if (a->player_mode != WM_PMODE_HEADHOLD &&
        a->player_mode != WM_PMODE_HEADHELD) return 0;

    if (a->player_mode == WM_PMODE_HEADHELD) {
        /* RAZOR.ASM reversal branch: DO_REVERSAL, DO_REVERSAL_MESS,
           SMRTTGT WHOHITME, then immobilize WHOHITME. */
        if (a->i_will_die != 0) return 0;
        if (a->immobilize_time != 0) return 0;
        if (cb && cb->do_reversal) cb->do_reversal(a, cb->user);
        if (cb && cb->do_reversal_message) cb->do_reversal_message(a, cb->user);
        target = a->who_hit_me ? a->who_hit_me : opp;
        a->smart_target = target;
    } else {
        /* RAZOR.ASM slam branch: BONUS_MESS ids are 7 pile, 33 edge, 6 rug,
           then SMRTTGT WHOIHIT and immobilize WHOIHIT. */
        if (a->immobilize_time != 0) return 0;
        if (e->gate_kind == G_RAZOR_HD_PILE) bonus = 7;
        else if (e->gate_kind == G_RAZOR_HD_EDGE) bonus = 33;
        else if (e->gate_kind == G_RAZOR_HD_RUG) bonus = 6;
        if (bonus != 0 && cb && cb->bonus_message)
            cb->bonus_message(a, bonus, cb->user);
        target = a->who_i_hit ? a->who_i_hit : opp;
        a->smart_target = target;
    }

    if (!target) return 0;
    target->immobilize_time = 15;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);

    queue_result(a, e, cb);
    if ((e->gate_kind == G_RAZOR_HD_PILE || e->gate_kind == G_RAZOR_HD_EDGE) &&
        cb && cb->sound_label)
        cb->sound_label(a, "GRABHOLD_T1/GRABHOLD_T2", cb->user);
    return 1;
}

static int fire_razor_grab_toss_air(wm_arcade_actor_t *a,
                                    wm_arcade_actor_t *opp,
                                    const wm_arcade_smove_entry_t *e,
                                    const wm_arcade_smove_callbacks_t *cb)
{
    const char *label;
    if (!a || !e) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->player_mode == WM_PMODE_HEADHOLD) return 0;
    if (opp && (opp->player_mode == WM_PMODE_ONGROUND ||
                opp->player_mode == WM_PMODE_DEAD)) return 0;

    if (opp && (mode_is_inair(opp->player_mode) ||
                opp->attack_type == WM_SMOVE_AT_LEAPING)) {
        label = ((a->facing_dir & WM_MOVE_RIGHT) != 0u) ?
            "rzr_2_hiptoss2_anim" : "rzr_4_hiptoss2_anim";
    } else {
        if (cb && cb->find_and_kill_endless)
            cb->find_and_kill_endless(a, cb->user);
        if (a->closest_dist > 0x68) return 0;
        label = ((a->facing_dir & WM_MOVE_RIGHT) != 0u) ?
            "rzr_2_hiptoss_anim" : "rzr_4_hiptoss_anim";
    }

    a->special_move_addr = (uintptr_t)label;
    if (cb && cb->resolve_label_token)
        a->special_move_addr = cb->resolve_label_token(label, cb->user);
    if (cb && cb->sound_label) cb->sound_label(a, "GRABFLING_T1/PUNCH_T2", cb->user);
    return 1;
}

static int fire_razor_headhold_combo(wm_arcade_actor_t *a,
                                     wm_arcade_actor_t *opp,
                                     const wm_arcade_smove_entry_t *e,
                                     const wm_arcade_smove_callbacks_t *cb)
{
    if (!a || !e) return 0;
    if (a->player_mode != WM_PMODE_HEADHOLD) return 0;
    if (cb && cb->check_combo_go && cb->check_combo_go(a, cb->user) < 0) return 0;

    a->smart_target = a->who_i_hit ? a->who_i_hit : opp;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    return 1;
}


static int fire_yoko_headhold_throw(wm_arcade_actor_t *a,
                                    wm_arcade_actor_t *opp,
                                    const wm_arcade_smove_entry_t *e,
                                    const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *target = 0;
    int bonus = 0;
    uint16_t immob = 15;
    if (!a || !e) return 0;

    if (a->player_mode != WM_PMODE_HEADHOLD &&
        a->player_mode != WM_PMODE_HEADHELD) return 0;

    if (a->player_mode == WM_PMODE_HEADHELD) {
        if (a->i_will_die != 0) return 0;
        if (a->immobilize_time != 0) return 0;
        if (cb && cb->do_reversal) cb->do_reversal(a, cb->user);
        if (cb && cb->do_reversal_message) cb->do_reversal_message(a, cb->user);
        target = a->who_hit_me ? a->who_hit_me : opp;
        a->smart_target = target;
    } else {
        if (a->immobilize_time != 0) return 0;
        if (e->gate_kind == G_YOKO_HD_SUPLEX) {
            bonus = 42;
            immob = 30;
        } else if (e->gate_kind == G_YOKO_HD_SCISSOR) {
            bonus = 34;
            immob = 32;
        }
        if (bonus != 0 && cb && cb->bonus_message)
            cb->bonus_message(a, bonus, cb->user);
        target = a->who_i_hit ? a->who_i_hit : opp;
        a->smart_target = target;
    }

    if (!target) return 0;
    target->immobilize_time = immob;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label)
        cb->sound_label(a, "GRABFLING_T1/GRABFLING_T2", cb->user);
    return 1;
}

static int fire_yoko_headhold_combo(wm_arcade_actor_t *a,
                                    wm_arcade_actor_t *opp,
                                    const wm_arcade_smove_entry_t *e,
                                    const wm_arcade_smove_callbacks_t *cb)
{
    if (!a || !e) return 0;
    if (a->player_mode != WM_PMODE_HEADHOLD) return 0;
    if (cb && cb->check_combo_go && cb->check_combo_go(a, cb->user) < 0) return 0;
    if (a->immobilize_time != 0) return 0;

    a->smart_target = a->who_i_hit ? a->who_i_hit : opp;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label)
        cb->sound_label(a, "UPRCUT_T1/UPRCUT_T2", cb->user);
    return 1;
}

static int fire_yoko_salt_throw(wm_arcade_actor_t *a,
                                const wm_arcade_smove_entry_t *e,
                                const wm_arcade_smove_callbacks_t *cb)
{
    if (!a || !e) return 0;
    if (a->player_mode != WM_PMODE_HEADHOLD &&
        a->player_mode != WM_PMODE_HEADHELD) return 0;

    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label)
        cb->sound_label(a, "LBOWDROP_T1/LBOWDROP_T2", cb->user);
    return 1;
}

static int fire_yoko_grab_toss_air(wm_arcade_actor_t *a,
                                   wm_arcade_actor_t *opp,
                                   const wm_arcade_smove_entry_t *e,
                                   const wm_arcade_smove_callbacks_t *cb)
{
    const char *label;
    if (!a || !e) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->player_mode == WM_PMODE_HEADHOLD) return 0;
    if (opp && (opp->player_mode == WM_PMODE_ONGROUND ||
                opp->player_mode == WM_PMODE_DEAD)) return 0;

    if (opp && (mode_is_inair(opp->player_mode) ||
                opp->attack_type == WM_SMOVE_AT_LEAPING)) {
        label = "yok_hiptoss2_anim";
    } else {
        if (a->closest_dist > 0x6c) return 0;
        label = "yok_hiptoss_anim";
    }

    a->special_move_addr = (uintptr_t)label;
    if (cb && cb->resolve_label_token)
        a->special_move_addr = cb->resolve_label_token(label, cb->user);
    if (cb && cb->sound_label)
        cb->sound_label(a, "GRABFLING_T1/PUNCH_T2", cb->user);
    return 1;
}


static int r21_is_headholdish(uint16_t mode)
{
    return mode == WM_PMODE_HEADHOLD || mode == WM_PMODE_HEADHELD;
}

static int r21_reject_basic_active(const wm_arcade_actor_t *a)
{
    if (!a) return 1;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 1;
    if (a->getup_time != 0) return 1;
    if (a->player_mode == WM_PMODE_HEADHELD ||
        a->player_mode == WM_PMODE_HEADHOLD ||
        a->player_mode == WM_PMODE_ONGROUND ||
        a->player_mode == WM_PMODE_DEAD) return 1;
    return 0;
}

static int r21_opp_ground_dead(const wm_arcade_actor_t *opp)
{
    return opp && (opp->player_mode == WM_PMODE_ONGROUND ||
                   opp->player_mode == WM_PMODE_DEAD);
}

static int r21_bonus_for_gate(uint8_t gate)
{
    switch (gate) {
    case G_SHAWN_HD_SUPLEX: return 42;
    case G_SHAWN_HD_FRANK: return 34;
    case G_SHAWN_HD_KICKTOSS: return 22;
    case G_SHAWN_HD_BUTTS: return 20;
    case G_BAM_HD_PILE: return 7;
    case G_BAM_HD_POGO: return 19;
    case G_DOINK_HD_SLAM: return 20;
    case G_DOINK_HD_PILE: return 7;
    case G_DOINK_HD_BUZZ: return 25;
    case G_LEX_HD_PILE: return 7;
    case G_LEX_HD_ELBOW_FACE: return 20;
    case G_LEX_HD_GRABOH: return 20;
    default: return 0;
    }
}

static uint16_t r21_immob_for_gate(uint8_t gate)
{
    switch (gate) {
    case G_SHAWN_HD_SUPLEX: return 30;
    case G_SHAWN_HD_FRANK: return 32;
    case G_SHAWN_HD_KICKTOSS: return 24;
    case G_SHAWN_HD_BUTTS: return 20;
    default: return 15;
    }
}

static const char *r21_sound_for_gate(uint8_t gate)
{
    switch (gate) {
    case G_SHAWN_SWIRL_SPEEDKICK:
    case G_SHAWN_SLIDING_KICKTOSS:
    case G_DOINK_CHARGE_FLYKICK:
        return "FLYKICK_T1/FLYKICK_T2";
    case G_SHAWN_CHARGE_SUPLEX:
    case G_BAM_CHARGE_NECKBREAKER:
        return "GRABFLING_T1/GRABFLING_T2";
    case G_DOINK_HD_BUZZ:
        return "BUZZ_T1/BUZZ_T2";
    default:
        return "GRABFLING_T1/PUNCH_T2";
    }
}

static int fire_remaining_charge(wm_arcade_smove_proc_t *p,
                                 wm_arcade_actor_t *a,
                                 wm_arcade_actor_t *opp,
                                 const wm_arcade_smove_entry_t *e,
                                 const wm_arcade_smove_callbacks_t *cb)
{
    uint16_t button;
    uint16_t charge;
    uint16_t min_ticks = 85u;
    if (!p || !a || !e) return 0;

    if (e->gate_kind == G_DOINK_CHARGE_FLYKICK)
        button = WM_BTN_SKICK;
    else if (e->gate_kind == G_SHAWN_CHARGE_SUPLEX)
        button = WM_BTN_SPUNCH;
    else
        button = WM_BTN_PUNCH;

    if ((a->but_val_cur & button) != 0u) {
        if (p->timeout != 0xffffu) ++p->timeout;
        return 0;
    }

    charge = p->timeout;
    p->timeout = 0;
    if (charge < min_ticks) return 0;
    if (r21_reject_basic_active(a)) return 0;
    if (r21_opp_ground_dead(opp)) return 0;
    if (cb && cb->ck_ignore && cb->ck_ignore(a, cb->user)) return 0;

    queue_result(a, e, cb);
    if (e->gate_kind == G_DOINK_CHARGE_FLYKICK && a->player_mode != WM_PMODE_DEAD)
        a->player_mode = WM_PMODE_INAIR;
    if (cb && cb->sound_label)
        cb->sound_label(a, r21_sound_for_gate(e->gate_kind), cb->user);
    return 1;
}

static int fire_remaining_headhold_throw(wm_arcade_actor_t *a,
                                         wm_arcade_actor_t *opp,
                                         const wm_arcade_smove_entry_t *e,
                                         const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *target = 0;
    int bonus;
    if (!a || !e) return 0;
    if (!r21_is_headholdish(a->player_mode)) return 0;

    if (a->player_mode == WM_PMODE_HEADHELD) {
        if (a->i_will_die != 0) return 0;
        if (a->immobilize_time != 0) return 0;
        if (cb && cb->do_reversal) cb->do_reversal(a, cb->user);
        if (cb && cb->do_reversal_message) cb->do_reversal_message(a, cb->user);
        target = a->who_hit_me ? a->who_hit_me : opp;
        a->smart_target = target;
    } else {
        if (a->immobilize_time != 0) return 0;
        bonus = r21_bonus_for_gate(e->gate_kind);
        if (bonus != 0 && cb && cb->bonus_message)
            cb->bonus_message(a, bonus, cb->user);
        target = a->who_i_hit ? a->who_i_hit : opp;
        a->smart_target = target;
    }

    if (!target) return 0;
    target->immobilize_time = r21_immob_for_gate(e->gate_kind);
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label)
        cb->sound_label(a, "GRABHOLD_T1/GRABHOLD_T2", cb->user);
    return 1;
}

static int fire_remaining_combo(wm_arcade_actor_t *a,
                                wm_arcade_actor_t *opp,
                                const wm_arcade_smove_entry_t *e,
                                const wm_arcade_smove_callbacks_t *cb)
{
    if (!a || !e) return 0;
    if (a->player_mode != WM_PMODE_HEADHOLD) return 0;
    if (cb && cb->check_combo_go && cb->check_combo_go(a, cb->user) < 0) return 0;
    if (a->immobilize_time != 0) return 0;
    a->smart_target = a->who_i_hit ? a->who_i_hit : opp;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label)
        cb->sound_label(a, "UPRCUT_T1/UPRCUT_T2", cb->user);
    return 1;
}

static int fire_remaining_grab_toss_air(wm_arcade_actor_t *a,
                                        wm_arcade_actor_t *opp,
                                        const wm_arcade_smove_entry_t *e,
                                        const wm_arcade_smove_callbacks_t *cb)
{
    const char *normal;
    const char *air;
    uint16_t limit = 0x70u;
    if (!a || !e) return 0;
    normal = e->result_label;
    air = normal;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->player_mode == WM_PMODE_HEADHOLD) return 0;
    if (r21_opp_ground_dead(opp)) return 0;

    switch (e->gate_kind) {
    case G_SHAWN_GRAB_TOSS_AIR: air = "shn_hiptoss2_anim"; break;
    case G_BAM_GRAB_TOSS_AIR: air = "bam_hiptoss2_anim"; break;
    case G_DOINK_GRAB_TOSS_AIR: air = "dnk_hiptoss2_anim"; break;
    case G_LEX_GRAB_TOSS_AIR: air = "lex_hiptoss2_anim"; break;
    default: break;
    }

    if (opp && (mode_is_inair(opp->player_mode) ||
                opp->attack_type == WM_SMOVE_AT_LEAPING)) {
        normal = air;
    } else {
        if (cb && cb->find_and_kill_endless)
            cb->find_and_kill_endless(a, cb->user);
        if (a->closest_dist > limit) return 0;
    }

    a->special_move_addr = (uintptr_t)normal;
    if (cb && cb->resolve_label_token)
        a->special_move_addr = cb->resolve_label_token(normal, cb->user);
    if (cb && cb->sound_label)
        cb->sound_label(a, "GRABFLING_T1/PUNCH_T2", cb->user);
    return 1;
}

static int fire_remaining_motion(wm_arcade_actor_t *a,
                                 wm_arcade_actor_t *opp,
                                 const wm_arcade_smove_entry_t *e,
                                 const wm_arcade_smove_callbacks_t *cb)
{
    if (!a || !e) return 0;
    if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
    if (a->player_mode == WM_PMODE_ONTURNBKL ||
        a->player_mode == WM_PMODE_HEADHELD) return 0;
    if (r21_opp_ground_dead(opp)) return 0;
    if (cb && cb->ck_ignore && cb->ck_ignore(a, cb->user)) return 0;

    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label)
        cb->sound_label(a, r21_sound_for_gate(e->gate_kind), cb->user);
    return 1;
}

static int fire_entry(wm_arcade_actor_t **actors, size_t n,
                      wm_arcade_actor_t *a,
                      wm_arcade_smove_proc_t *p,
                      const wm_arcade_smove_entry_t *e,
                      const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *opp = opponent_for(actors, n, a);
    if (!a || !e) return 0;
    switch (e->gate_kind) {
    case G_BRET_CHARGE_FACE_RAKE:
        return fire_bret_charge_face_rake(p, a, e, cb);
    case G_BRET_CHARGE_FLYING_KICK:
        return fire_bret_charge_flying_kick(p, a, opp, e, cb);
    case G_BRET_ROLL_UPPERCUT:
        return fire_bret_roll_uppercut(a, e, cb);
    case G_BRET_HD_PILE:
    case G_BRET_HD_DDT:
    case G_BRET_HD_FACESLAM:
        return fire_bret_headhold_body(a, opp, e, cb);
    case G_BRET_GRAB_TOSS_AIR:
        return fire_bret_grab_toss_air(a, opp, e, cb);
    case G_BRET_HD_COMBO1:
    case G_BRET_HD_COMBO2:
        return fire_bret_headhold_combo(a, opp, e, cb);
    case G_RAZOR_CHARGE_SLASHES:
        return fire_razor_charge_slashes(p, a, e, cb);
    case G_RAZOR_HD_PILE:
    case G_RAZOR_HD_EDGE:
    case G_RAZOR_HD_RUG:
        return fire_razor_headhold_throw(a, opp, e, cb);
    case G_RAZOR_GRAB_TOSS_AIR:
        return fire_razor_grab_toss_air(a, opp, e, cb);
    case G_RAZOR_HD_COMBO1:
    case G_RAZOR_HD_COMBO2:
        return fire_razor_headhold_combo(a, opp, e, cb);
    case G_RAZOR_SLIDING_RUG:
        return fire_razor_sliding_rug(a, opp, e, cb);
    case G_YOKO_HD_COMBO1:
    case G_YOKO_HD_COMBO2:
        return fire_yoko_headhold_combo(a, opp, e, cb);
    case G_YOKO_HD_SCISSOR:
    case G_YOKO_HD_SUPLEX:
        return fire_yoko_headhold_throw(a, opp, e, cb);
    case G_YOKO_SALT_THROW:
        return fire_yoko_salt_throw(a, e, cb);
    case G_YOKO_GRAB_TOSS_AIR:
        return fire_yoko_grab_toss_air(a, opp, e, cb);
    case G_SHAWN_CHARGE_SUPLEX:
    case G_BAM_CHARGE_NECKBREAKER:
    case G_DOINK_CHARGE_FLYKICK:
        return fire_remaining_charge(p, a, opp, e, cb);
    case G_SHAWN_HD_SUPLEX:
    case G_SHAWN_HD_FRANK:
    case G_SHAWN_HD_KICKTOSS:
    case G_SHAWN_HD_BUTTS:
    case G_BAM_HD_PILE:
    case G_BAM_HD_POGO:
    case G_DOINK_HD_SLAM:
    case G_DOINK_HD_PILE:
    case G_DOINK_HD_BUZZ:
    case G_LEX_HD_PILE:
    case G_LEX_HD_ELBOW_FACE:
    case G_LEX_HD_GRABOH:
        return fire_remaining_headhold_throw(a, opp, e, cb);
    case G_SHAWN_HD_COMBO1:
    case G_SHAWN_HD_COMBO2:
    case G_BAM_HD_COMBO1:
    case G_BAM_HD_COMBO2:
    case G_DOINK_HD_COMBO1:
    case G_DOINK_HD_COMBO2:
    case G_LEX_HD_COMBO1:
    case G_LEX_HD_COMBO2:
        return fire_remaining_combo(a, opp, e, cb);
    case G_SHAWN_GRAB_TOSS_AIR:
    case G_BAM_GRAB_TOSS_AIR:
    case G_DOINK_GRAB_TOSS_AIR:
    case G_LEX_GRAB_TOSS_AIR:
        return fire_remaining_grab_toss_air(a, opp, e, cb);
    case G_SHAWN_SWIRL_SPEEDKICK:
    case G_SHAWN_SLIDING_KICKTOSS:
    case G_SHAWN_FLIPSLAM:
        return fire_remaining_motion(a, opp, e, cb);
    case G_TAKER_HD_NECK:
    case G_TAKER_HD_FACESLAM:
    case G_TAKER_HD_PILE:
        return fire_taker_headhold(a, opp, e, cb);
    case G_TAKER_CHOKE_SLIDE:
        if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
        if (a->player_mode == WM_PMODE_ONTURNBKL || a->player_mode == WM_PMODE_RUNNING) return 0;
        if (a->i_will_die) return 0;
        if (opp && (mode_is_ground_dead(opp->player_mode) ||
                    opp->player_mode == WM_PMODE_HEADHELD ||
                    opp->player_mode == WM_PMODE_CHOKING)) return 0;
        queue_result(a, e, cb);
        return 1;
    case G_TAKER_SPIRIT_PUSH:
    case G_TAKER_SPIRIT_PULL:
        if (a->player_mode == WM_PMODE_HEADHOLD || a->player_mode == WM_PMODE_HEADHELD) return 0;
        if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
        if (a->immobilize_time != 0) return 0;
        if (opp && (opp->player_mode == WM_PMODE_CHOKING || opp->player_mode == WM_PMODE_HEADHELD)) return 0;
        a->run_time = 0;
        if (a->player_mode != WM_PMODE_DEAD) a->player_mode = WM_PMODE_NORMAL;
        queue_result(a, e, cb);
        if (cb && cb->sound_label) cb->sound_label(a, "SPIRIT", cb->user);
        return 1;
    case G_TAKER_GRAB_TOSS_AIR:
        if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
        if (a->player_mode == WM_PMODE_HEADHOLD) return 0;
        if (opp && mode_is_ground_dead(opp->player_mode)) return 0;
        if (opp && mode_is_inair(opp->player_mode)) {
            /* Source FACE24 snapmirror2: pick side using facing. */
            a->special_move_addr = (uintptr_t)(((a->facing_dir & WM_MOVE_RIGHT) != 0u) ?
                "und_2_snapmirror2_anim" : "und_4_snapmirror2_anim");
        } else {
            if (a->closest_dist > 0x68) return 0;
            a->special_move_addr = (uintptr_t)(((a->facing_dir & WM_MOVE_RIGHT) != 0u) ?
                "und_2_snapmirror_anim" : "und_4_snapmirror_anim");
        }
        if (cb && cb->resolve_label_token)
            a->special_move_addr = cb->resolve_label_token((const char *)a->special_move_addr, cb->user);
        if (cb && cb->sound_label) cb->sound_label(a, "HIPTOSS_T1/HIPTOSS_T2", cb->user);
        a->attach_proc = 0;
        if (a->player_mode != WM_PMODE_DEAD) a->player_mode = WM_PMODE_NORMAL;
        return 1;
    case G_TAKER_COMBO1:
    case G_TAKER_COMBO2:
        if (a->player_mode != WM_PMODE_HEADHOLD) return 0;
        if (cb && cb->check_combo_go && cb->check_combo_go(a, cb->user) < 0) return 0;
        if (a->immobilize_time != 0) return 0;
        a->smart_target = a->who_i_hit ? a->who_i_hit : opp;
        if (cb && cb->find_and_kill_endless)
            cb->find_and_kill_endless(a, cb->user);
        queue_result(a, e, cb);
        return 1;
    case G_TAKER_FINISH1:
        queue_result(a, e, cb);
        return 1;
    default:
        return 0;
    }
}

void wm_arcade_smove_runtime_tick(
    wm_arcade_smove_runtime_t *rt,
    wm_arcade_actor_t **actors,
    size_t actor_count,
    const wm_arcade_smove_callbacks_t *callbacks)
{
    size_t i;
    if (!rt || !actors) return;
    for (i = 0; i < rt->proc_count; ++i) {
        wm_arcade_smove_proc_t *p = &rt->proc[i];
        wm_arcade_actor_t *a;
        const wm_arcade_smove_wait_step_t *s;
        wm_arcade_smove_wait_result_t wr;
        if (!p->active || p->unresolved || !p->entry) continue;
        if (p->owner_slot >= actor_count) continue;
        a = actors[p->owner_slot];
        if (!a || !a->active) continue;
        if (p->sleep_ticks != 0u) { --p->sleep_ticks; continue; }
        if (a->special_move_addr != (uintptr_t)0) { proc_rewind(rt, p, 1); continue; }
        if (p->entry->step_count == 0u) {
            if (fire_entry(actors, actor_count, a, p, p->entry, callbacks)) {
                ++p->fires;
                ++rt->fire_count;
                proc_rewind(0, p, p->entry->post_fire_sleep);
            }
            continue;
        }
        if (p->step_index >= p->entry->step_count) { proc_rewind(rt, p, 1); continue; }
        s = &p->entry->steps[p->step_index];
        if (s->load_timeout != WM_ARCADE_SMOVE_TIMEOUT_KEEP &&
            p->timeout_loaded_for != p->step_index) {
            p->timeout = s->load_timeout;
            p->timeout_loaded_for = p->step_index;
        }
        wr = wm_arcade_smove_waitswitch_down(a, s->expected, s->ignore_mask, &p->timeout);
        if (wr == WM_SMOVE_WAIT_STILL_WAITING) continue;
        if (wr == WM_SMOVE_WAIT_RESET) { proc_rewind(rt, p, 1); continue; }
        ++p->step_index;
        p->timeout_loaded_for = 0xffu;
        if (p->step_index < p->entry->step_count) continue;
        if (fire_entry(actors, actor_count, a, p, p->entry, callbacks)) {
            ++p->fires;
            ++rt->fire_count;
            proc_rewind(0, p, p->entry->post_fire_sleep);
        } else {
            proc_rewind(rt, p, 1);
        }
    }
}
