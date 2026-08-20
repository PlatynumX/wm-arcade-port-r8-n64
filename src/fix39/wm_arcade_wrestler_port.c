#include "wm_arcade_wrestler_port.h"

#include <string.h>

#define STEP(v,i) {(uint16_t)(v),(uint16_t)(i)}
#define PAT(l,s,n,t) {l,s,n,t}

/* BRET.ASM secret table records. charge_ddt is executable probe code. */
static const wm_arcade_input_step_t bret_neck[] = {
    STEP(WM_B_SPUNCH, WM_J_ALL), STEP(WM_J_TOWARD, WM_J_REAL_LR), STEP(WM_J_TOWARD, WM_J_REAL_LR)
};
static const wm_arcade_input_step_t bret_grab[] = {
    STEP(WM_B_SPUNCH, WM_J_ALL), STEP(WM_J_AWAY, WM_J_REAL_LR), STEP(WM_J_AWAY, WM_J_REAL_LR)
};
static const wm_arcade_input_step_t bret_hip[] = {
    STEP(WM_B_PUNCH, WM_J_ALL), STEP(WM_J_AWAY, WM_J_REAL_LR), STEP(WM_J_AWAY, WM_J_REAL_LR)
};
static const wm_arcade_input_step_t bret_grab2[] = {
    STEP(WM_B_SPUNCH | WM_J_AWAY, WM_J_REAL_LR | WM_J_UP | WM_J_DOWN)
};
static const wm_arcade_input_step_t bret_hip2[] = {
    STEP(WM_B_PUNCH | WM_J_AWAY, WM_J_REAL_LR | WM_J_UP | WM_J_DOWN)
};
static const wm_arcade_input_step_t bret_rake[] = {
    STEP(WM_B_PUNCH, WM_J_ALL), STEP(WM_J_TOWARD, WM_J_REAL_LR),
    STEP(WM_J_DOWN_TOWARD, WM_J_REAL_LR), STEP(WM_J_DOWN, WM_J_REAL_LR)
};
static const wm_arcade_input_step_t bret_jump[] = {
    STEP(WM_B_SKICK, WM_J_ALL),
    STEP(WM_J_AWAY, WM_J_REAL_LR | WM_J_UP | WM_J_DOWN),
    STEP(WM_J_AWAY, WM_J_REAL_LR | WM_J_UP | WM_J_DOWN)
};
static const wm_arcade_input_step_t bret_supercut[] = {
    STEP(WM_B_PUNCH, WM_J_ALL), STEP(WM_J_DOWN, WM_J_REAL_LR), STEP(WM_J_DOWN, WM_J_REAL_LR)
};
static const wm_arcade_input_pattern_t bret_secrets[] = {
    PAT("charge_ddt", NULL, 0, 0),
    PAT("neck_grab", bret_neck, 3, 32),
    PAT("grab_fling", bret_grab, 3, 32),
    PAT("hip_toss", bret_hip, 3, 32),
    PAT("grab_fling2", bret_grab2, 1, 10),
    PAT("hip_toss2", bret_hip2, 1, 10),
    PAT("face_rake", bret_rake, 4, 30),
    PAT("jump_kick", bret_jump, 3, 32),
    PAT("supercut", bret_supercut, 3, 16)
};
static const char *const bret_smove[] = {
    "hrt_charge_flying_kick", "hrt_charge_face_rake", "hrt_hdhold_pile",
    "hrt_hdhold_ddt", "hrt_hdhold_faceslam", "hrt_grab_toss_air",
    "hrt_roll_uppercut", "hrt_hdhold_combo1", "hrt_hdhold_combo2",
    "std_walk_fast", "std_taunt", "hrt_finish_move1", "hrt_finish_move2"
};

/* RAZOR.ASM secret table records. charge_flying_kick is executable probe code. */
static const wm_arcade_input_step_t razor_neck[] = {
    STEP(WM_B_SPUNCH, WM_J_REAL_LR | WM_J_TOWARD | WM_J_AWAY | WM_J_UP),
    STEP(WM_J_TOWARD, WM_J_REAL_LR), STEP(WM_J_TOWARD, WM_J_REAL_LR)
};
static const wm_arcade_input_step_t razor_grab[] = {
    STEP(WM_B_SPUNCH, WM_J_ALL), STEP(WM_J_AWAY, WM_J_REAL_LR), STEP(WM_J_AWAY, WM_J_REAL_LR)
};
static const wm_arcade_input_step_t razor_hip[] = {
    STEP(WM_B_PUNCH, WM_J_ALL), STEP(WM_J_AWAY, WM_J_REAL_LR), STEP(WM_J_AWAY, WM_J_REAL_LR)
};
static const wm_arcade_input_step_t razor_grab2[] = {
    STEP(WM_B_SPUNCH | WM_J_AWAY, WM_J_REAL_LR | WM_J_UP | WM_J_DOWN)
};
static const wm_arcade_input_step_t razor_hip2[] = {
    STEP(WM_B_PUNCH | WM_J_AWAY, WM_J_REAL_LR | WM_J_UP | WM_J_DOWN)
};
static const wm_arcade_input_step_t razor_dslash[] = {
    STEP(WM_B_PUNCH, WM_J_ALL), STEP(WM_J_TOWARD, WM_J_REAL_LR),
    STEP(WM_J_DOWN_TOWARD, WM_J_REAL_LR), STEP(WM_J_DOWN, WM_J_REAL_LR)
};
static const wm_arcade_input_pattern_t razor_secrets[] = {
    PAT("charge_flying_kick", NULL, 0, 0),
    PAT("neck_grab", razor_neck, 3, 30),
    PAT("grab_fling", razor_grab, 3, 32),
    PAT("hip_toss", razor_hip, 3, 32),
    PAT("grab_fling2", razor_grab2, 1, 10),
    PAT("hip_toss2", razor_hip2, 1, 10),
    PAT("down_slash", razor_dslash, 4, 50)
};
static const char *const razor_smove[] = {
    "rzr_charge_slashes", "rzr_hdhold_pile", "rzr_hdhold_combo1",
    "rzr_hdhold_edge", "rzr_hdhold_rug", "rzr_grab_toss_air",
    "rzr_hdhold_combo2", "std_walk_fast", "std_taunt", "rzr_sliding_rug",
    "rzr_finish_move1", "rzr_finish_move2"
};

const wm_arcade_wrestler_profile_t wm_arcade_profile_bret = {
    WM_ROSTER_BRET, "Bret Hart", "BRET.ASM", 2973, "hrt",
    WM_BTN_SPUNCH, 100,
    bret_secrets, sizeof bret_secrets / sizeof bret_secrets[0],
    bret_smove, sizeof bret_smove / sizeof bret_smove[0]
};
const wm_arcade_wrestler_profile_t wm_arcade_profile_razor = {
    WM_ROSTER_RAZOR, "Razor Ramon", "RAZOR.ASM", 2713, "rzr",
    WM_BTN_SKICK, 85,
    razor_secrets, sizeof razor_secrets / sizeof razor_secrets[0],
    razor_smove, sizeof razor_smove / sizeof razor_smove[0]
};

static wm_arcade_bret_env_t bret_env(const wm_arcade_roster_env_t *e)
{
    wm_arcade_bret_env_t r = {0,0,0,0,0};
    if (e) { r.pcnt=e->pcnt; r.hyper_speed_on=e->hyper_speed_on; r.blocking_off=e->blocking_off; r.p1rounds=e->p1rounds; r.p2rounds=e->p2rounds; }
    return r;
}
static wm_arcade_razor_env_t razor_env(const wm_arcade_roster_env_t *e)
{
    wm_arcade_razor_env_t r = {0,0,0,0,0};
    if (e) { r.pcnt=e->pcnt; r.hyper_speed_on=e->hyper_speed_on; r.blocking_off=e->blocking_off; r.p1rounds=e->p1rounds; r.p2rounds=e->p2rounds; }
    return r;
}


static int profile_secret_index(const wm_arcade_wrestler_profile_t *p, const char *s, size_t *idx)
{
    size_t i;
    if (!p || !s || !idx) return 0;
    for (i = 0; i < p->secret_count; ++i) {
        if (strcmp(p->secrets[i].source_label, s) == 0) { *idx = i; return 1; }
    }
    return 0;
}

static int profile_monitor_index(const wm_arcade_wrestler_profile_t *p, const char *s, size_t *idx)
{
    size_t i;
    if (!p || !s || !idx) return 0;
    for (i = 0; i < p->special_process_count; ++i) {
        if (strcmp(p->special_processes[i], s) == 0) { *idx = i; return 1; }
    }
    return 0;
}

static wm_arcade_roster_step_result_t convert_step(int r)
{
    return r == 1 ? WM_ROSTER_STEP_ACTION : r == 2 ? WM_ROSTER_STEP_EXTERNAL : WM_ROSTER_STEP_IDLE;
}

wm_arcade_roster_step_result_t wm_arcade_move_ported_wrestler(
    const wm_arcade_wrestler_profile_t *p, wm_arcade_actor_t *a,
    wm_arcade_actor_t *o, const wm_arcade_roster_env_t *e,
    const wm_arcade_wrestler_port_bindings_t *b)
{
    if (!p || !a) return WM_ROSTER_STEP_IDLE;
    switch (p->id) {
    case WM_ROSTER_BRET: {
        wm_arcade_bret_env_t be = bret_env(e);
        return convert_step((int)wm_arcade_move_bret(a,o,&be,b?b->bret:NULL));
    }
    case WM_ROSTER_RAZOR: {
        wm_arcade_razor_env_t re = razor_env(e);
        return convert_step((int)wm_arcade_move_razor(a,o,&re,b?b->razor:NULL));
    }
    case WM_ROSTER_TAKER: return convert_step((int)wm_arcade_move_taker(a,o,e,b?b->taker:NULL));
    case WM_ROSTER_YOKO: return convert_step((int)wm_arcade_move_yoko(a,o,e,b?b->yoko:NULL));
    case WM_ROSTER_SHAWN: return convert_step((int)wm_arcade_move_shawn(a,o,e,b?b->shawn:NULL));
    case WM_ROSTER_BAM: return convert_step((int)wm_arcade_move_bam(a,o,e,b?b->bam:NULL));
    case WM_ROSTER_DOINK: return convert_step((int)wm_arcade_move_doink(a,o,e,b?b->doink:NULL));
    case WM_ROSTER_LEX: return convert_step((int)wm_arcade_move_lex(a,o,e,b?b->lex:NULL));
    default: return WM_ROSTER_STEP_IDLE;
    }
}

static int bret_secret_id(const char *s, wm_arcade_bret_secret_id_t *id)
{
    struct item { const char *s; wm_arcade_bret_secret_id_t id; };
    static const struct item t[] = {
        {"neck_grab",WM_BRET_SECRET_NECK_GRAB},{"grab_fling",WM_BRET_SECRET_GRAB_FLING},
        {"hip_toss",WM_BRET_SECRET_HIP_TOSS},{"grab_fling2",WM_BRET_SECRET_GRAB_FLING2},
        {"hip_toss2",WM_BRET_SECRET_HIP_TOSS2},{"face_rake",WM_BRET_SECRET_FACE_RAKE},
        {"jump_kick",WM_BRET_SECRET_JUMP_KICK},{"supercut",WM_BRET_SECRET_SUPERCUT}
    };
    size_t i; for (i=0;i<sizeof t/sizeof t[0];++i) if (!strcmp(s,t[i].s)) { *id=t[i].id; return 1; } return 0;
}
static int razor_secret_id(const char *s, wm_arcade_razor_secret_id_t *id)
{
    struct item { const char *s; wm_arcade_razor_secret_id_t id; };
    static const struct item t[] = {
        {"neck_grab",WM_RZR_SECRET_NECK_GRAB},{"grab_fling",WM_RZR_SECRET_GRAB_FLING},
        {"hip_toss",WM_RZR_SECRET_HIP_TOSS},{"grab_fling2",WM_RZR_SECRET_GRAB_FLING2},
        {"hip_toss2",WM_RZR_SECRET_HIP_TOSS2},{"down_slash",WM_RZR_SECRET_DOWN_SLASH}
    };
    size_t i; for (i=0;i<sizeof t/sizeof t[0];++i) if (!strcmp(s,t[i].s)) { *id=t[i].id; return 1; } return 0;
}

int wm_arcade_port_fire_secret(const wm_arcade_wrestler_profile_t *p,
    wm_arcade_actor_t *a, wm_arcade_actor_t *o, const char *s, uint32_t pcnt,
    const wm_arcade_wrestler_port_bindings_t *b)
{
    size_t i;
    if (!p || !a || !s) return 0;
    switch (p->id) {
    case WM_ROSTER_BRET: {
        wm_arcade_bret_secret_id_t id; if(!bret_secret_id(s,&id))return 0;
        return wm_arcade_bret_fire_secret(a,o,id,pcnt,b?b->bret:NULL);
    }
    case WM_ROSTER_RAZOR: {
        wm_arcade_razor_secret_id_t id; if(!razor_secret_id(s,&id))return 0;
        return wm_arcade_razor_fire_secret(a,o,id,pcnt,b?b->razor:NULL);
    }
    case WM_ROSTER_TAKER: if(!profile_secret_index(p,s,&i))return 0; return wm_arcade_taker_fire_secret(a,o,(wm_arcade_taker_secret_id_t)i,pcnt,b?b->taker:NULL);
    case WM_ROSTER_YOKO: if(!profile_secret_index(p,s,&i))return 0; return wm_arcade_yoko_fire_secret(a,o,(wm_arcade_yoko_secret_id_t)i,pcnt,b?b->yoko:NULL);
    case WM_ROSTER_SHAWN: if(!profile_secret_index(p,s,&i))return 0; return wm_arcade_shawn_fire_secret(a,o,(wm_arcade_shawn_secret_id_t)i,pcnt,b?b->shawn:NULL);
    case WM_ROSTER_BAM: if(!profile_secret_index(p,s,&i))return 0; return wm_arcade_bam_fire_secret(a,o,(wm_arcade_bam_secret_id_t)i,pcnt,b?b->bam:NULL);
    case WM_ROSTER_DOINK: if(!profile_secret_index(p,s,&i))return 0; return wm_arcade_doink_fire_secret(a,o,(wm_arcade_doink_secret_id_t)i,pcnt,b?b->doink:NULL);
    case WM_ROSTER_LEX: if(!profile_secret_index(p,s,&i))return 0; return wm_arcade_lex_fire_secret(a,o,(wm_arcade_lex_secret_id_t)i,pcnt,b?b->lex:NULL);
    default:return 0;
    }
}

int wm_arcade_port_release_charge(const wm_arcade_wrestler_profile_t *p,
    wm_arcade_actor_t *a, wm_arcade_actor_t *o, const char *s,
    uint16_t ticks, const wm_arcade_wrestler_port_bindings_t *b)
{
    if (!p || !a || !s) return 0;
    switch (p->id) {
    case WM_ROSTER_BRET:
        if (!strcmp(s,"charge_ddt")) return wm_arcade_bret_try_charge_ddt(a,o,ticks,b?b->bret:NULL);
        if (!strcmp(s,"hrt_charge_flying_kick")) return wm_arcade_bret_release_charge_flying_kick(a,o,ticks,b?b->bret:NULL);
        if (!strcmp(s,"hrt_charge_face_rake")) return wm_arcade_bret_release_charge_face_rake(a,ticks,b?b->bret:NULL);
        return 0;
    case WM_ROSTER_RAZOR:
        if (!strcmp(s,"charge_flying_kick")) return wm_arcade_razor_release_charge_flying_kick(a,o,ticks,b?b->razor:NULL);
        if (!strcmp(s,"rzr_charge_slashes")) return wm_arcade_razor_release_charge_slashes(a,ticks,b?b->razor:NULL);
        return 0;
    case WM_ROSTER_TAKER: return !strcmp(s,"button_hold") ? wm_arcade_taker_release_charge(a,o,ticks,b?b->taker:NULL) : 0;
    case WM_ROSTER_YOKO: return !strcmp(s,"charge_salt") ? wm_arcade_yoko_release_charge(a,o,ticks,b?b->yoko:NULL) : 0;
    case WM_ROSTER_SHAWN: return !strcmp(s,"charge_flying_kick") ? wm_arcade_shawn_release_charge(a,o,ticks,b?b->shawn:NULL) : 0;
    case WM_ROSTER_BAM: return !strcmp(s,"firepnch") ? wm_arcade_bam_release_charge(a,o,ticks,b?b->bam:NULL) : 0;
    case WM_ROSTER_DOINK: return !strcmp(s,"charge_buzz") ? wm_arcade_doink_release_charge(a,o,ticks,b?b->doink:NULL) : 0;
    case WM_ROSTER_LEX: return !strcmp(s,"charge_clobber") ? wm_arcade_lex_release_charge(a,o,ticks,b?b->lex:NULL) : 0;
    default:return 0;
    }
}

static int bret_monitor_id(const char *s, wm_arcade_bret_monitor_id_t *id)
{
    struct item { const char *s; wm_arcade_bret_monitor_id_t id; };
    static const struct item t[] = {
        {"hrt_roll_uppercut",WM_BRET_MON_ROLL_UPPERCUT},{"hrt_hdhold_combo1",WM_BRET_MON_HEADHOLD_COMBO1},
        {"hrt_hdhold_combo2",WM_BRET_MON_HEADHOLD_COMBO2},{"hrt_hdhold_pile",WM_BRET_MON_HEADHOLD_PILE},
        {"hrt_hdhold_ddt",WM_BRET_MON_HEADHOLD_DDT},{"hrt_hdhold_faceslam",WM_BRET_MON_HEADHOLD_FACESLAM},
        {"hrt_grab_toss_air",WM_BRET_MON_GRAB_TOSS_AIR},{"hrt_finish_move1",WM_BRET_MON_FINISH1},{"hrt_finish_move2",WM_BRET_MON_FINISH2}
    };
    size_t i; for (i=0;i<sizeof t/sizeof t[0];++i) if (!strcmp(s,t[i].s)) { *id=t[i].id; return 1; } return 0;
}
static int razor_monitor_id(const char *s, wm_arcade_razor_monitor_id_t *id)
{
    struct item { const char *s; wm_arcade_razor_monitor_id_t id; };
    static const struct item t[] = {
        {"rzr_hdhold_pile",WM_RZR_MON_HEADHOLD_PILE},{"rzr_hdhold_combo1",WM_RZR_MON_HEADHOLD_COMBO1},
        {"rzr_hdhold_edge",WM_RZR_MON_HEADHOLD_EDGE},{"rzr_hdhold_rug",WM_RZR_MON_HEADHOLD_RUG},
        {"rzr_grab_toss_air",WM_RZR_MON_GRAB_TOSS_AIR},{"rzr_hdhold_combo2",WM_RZR_MON_HEADHOLD_COMBO2},
        {"rzr_sliding_rug",WM_RZR_MON_SLIDING_RUG},{"rzr_finish_move1",WM_RZR_MON_FINISH1},{"rzr_finish_move2",WM_RZR_MON_FINISH2}
    };
    size_t i; for (i=0;i<sizeof t/sizeof t[0];++i) if (!strcmp(s,t[i].s)) { *id=t[i].id; return 1; } return 0;
}

int wm_arcade_port_fire_monitor(const wm_arcade_wrestler_profile_t *p,
    wm_arcade_actor_t *a, wm_arcade_actor_t *o, const char *s,
    const wm_arcade_roster_env_t *e, int opp_leaping,
    const wm_arcade_wrestler_port_bindings_t *b)
{
    size_t i;
    if (!p || !a || !s) return 0;
    switch (p->id) {
    case WM_ROSTER_BRET: {
        wm_arcade_bret_monitor_id_t id; wm_arcade_bret_env_t be;
        if(!bret_monitor_id(s,&id))return 0;
        be=bret_env(e);
        return wm_arcade_bret_fire_monitor(a,o,id,&be,opp_leaping,b?b->bret:NULL);
    }
    case WM_ROSTER_RAZOR: {
        wm_arcade_razor_monitor_id_t id; wm_arcade_razor_env_t re;
        if(!razor_monitor_id(s,&id))return 0;
        re=razor_env(e);
        return wm_arcade_razor_fire_monitor(a,o,id,&re,opp_leaping,b?b->razor:NULL);
    }
    case WM_ROSTER_TAKER: if(!profile_monitor_index(p,s,&i))return 0; return wm_arcade_taker_fire_monitor(a,o,(wm_arcade_taker_monitor_id_t)i,e,opp_leaping,b?b->taker:NULL);
    case WM_ROSTER_YOKO: if(!profile_monitor_index(p,s,&i))return 0; return wm_arcade_yoko_fire_monitor(a,o,(wm_arcade_yoko_monitor_id_t)i,e,opp_leaping,b?b->yoko:NULL);
    case WM_ROSTER_SHAWN: if(!profile_monitor_index(p,s,&i))return 0; return wm_arcade_shawn_fire_monitor(a,o,(wm_arcade_shawn_monitor_id_t)i,e,opp_leaping,b?b->shawn:NULL);
    case WM_ROSTER_BAM: if(!profile_monitor_index(p,s,&i))return 0; return wm_arcade_bam_fire_monitor(a,o,(wm_arcade_bam_monitor_id_t)i,e,opp_leaping,b?b->bam:NULL);
    case WM_ROSTER_DOINK: if(!profile_monitor_index(p,s,&i))return 0; return wm_arcade_doink_fire_monitor(a,o,(wm_arcade_doink_monitor_id_t)i,e,opp_leaping,b?b->doink:NULL);
    case WM_ROSTER_LEX: if(!profile_monitor_index(p,s,&i))return 0; return wm_arcade_lex_fire_monitor(a,o,(wm_arcade_lex_monitor_id_t)i,e,opp_leaping,b?b->lex:NULL);
    default:return 0;
    }
}
