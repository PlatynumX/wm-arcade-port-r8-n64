#ifndef WM_ARCADE_DRONE_SOURCE_SCRIPTS_GENERATED_H
#define WM_ARCADE_DRONE_SOURCE_SCRIPTS_GENERATED_H

/* GENERATED DIRECTLY FROM historical DRONE.ASM. DO NOT HAND-EDIT. */
#define WM_FIX39_DRONE_SCRIPTS_GENERATED 1
#define WM_FIX39_DRONE_SCRIPT_COUNT 75
#define WM_FIX39_DRONE_SKILL_TABLE_COUNT 1
#define WM_FIX39_DRONE_C4_SEAM_COUNT 15

static const char *const wm_fix39_drone_c4_seam_labels[15] = {
    "drn_seek@EXGPC_0000",
    "drn_retreat@EXGPC_0000",
    "drone_chrg",
    "drn_climbtb@EXGPC_0000",
    "drn_taunt@EXGPC_0000",
    "drn_enterring@EXGPC_0000",
    "drn_opinair@EXGPC_0000",
    "drn_oprun@EXGPC_0000",
    "drn_roll@EXGPC_0000",
    "drn_inair@EXGPC_0000",
    "drn_ontb@EXGPC_0000",
    "drn_run@EXGPC_0000",
    "drn_combo@EXGPC_0000",
    "drn_seekclose@EXGPC_0000",
    "drn_oppdead@EXGPC_0000",
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_uddsk[5] = {
    { WM_DRONE_SC_INPUT, 0x0020u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_lrrsp[5] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_ucut[1] = {
    { WM_DRONE_SC_INPUT, 0x0044u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_jpx[4] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rp[1] = {
    { WM_DRONE_SC_INPUT, 0x0101u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rsp[1] = {
    { WM_DRONE_SC_INPUT, 0x0104u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_lrrp[5] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_k[1] = {
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_p[1] = {
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_sp[1] = {
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_sk[1] = {
    { WM_DRONE_SC_INPUT, 0x001fu, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_oghg[2] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_run[2] = {
    { WM_DRONE_SC_INPUT, 0x0089u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_spx[10] = {
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_hgrab[4] = {
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_flng[3] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0084u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_htoss[3] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0081u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_ddp[4] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_jp[4] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_j2sp[5] = {
    { WM_DRONE_SC_INPUT, 0x00c0u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_llsk[4] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rzup4[7] = {
    { WM_DRONE_SC_INPUT, 0x0021u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rzdp4[7] = {
    { WM_DRONE_SC_INPUT, 0x0041u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rzuddksp[19] = {
    { WM_DRONE_SC_INPUT, 0x0020u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0000u, 20, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0004u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 6, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rrk[4] = {
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_uthhrp[18] = {
    { WM_DRONE_SC_INPUT, 0x0101u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_RANDOM_JUMP, 0x0000u, 0, 50, 17u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_uttombhit[24] = {
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_ABORT_IF_BLOCKING, 0x0000u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_utdk[23] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0048u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_utchup[18] = {
    { WM_DRONE_SC_INPUT, 0x0021u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_RANDOM_JUMP, 0x0000u, 0, 50, 17u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_dsk[1] = {
    { WM_DRONE_SC_INPUT, 0x0050u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_utshootps[8] = {
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_WAIT_INTERRUPTIBLE, 0x0000u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_ABORT_IF_BLOCKING, 0x0000u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0009u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_utshootpl[8] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_WAIT_INTERRUPTIBLE, 0x0000u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_ABORT_IF_BLOCKING, 0x0000u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0044u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_j2p[5] = {
    { WM_DRONE_SC_INPUT, 0x00c0u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rrsk[4] = {
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rrp[4] = {
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_spsk[1] = {
    { WM_DRONE_SC_INPUT, 0x0014u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rsk4k[8] = {
    { WM_DRONE_SC_INPUT, 0x0110u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_jkk[12] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rsp4[7] = {
    { WM_DRONE_SC_INPUT, 0x0104u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_rk4[7] = {
    { WM_DRONE_SC_INPUT, 0x0108u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_lrrk[5] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_jisp[4] = {
    { WM_DRONE_SC_INPUT, 0x0020u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0120u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_bahhpg[24] = {
    { WM_DRONE_SC_INPUT, 0x0020u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 50, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 15, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 15, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_jk[4] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_bahhrsk[20] = {
    { WM_DRONE_SC_INPUT, 0x0110u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_RANDOM_JUMP, 0x0000u, 0, 33, 19u, NULL },
    { WM_DRONE_SC_RANDOM_JUMP, 0x0000u, 0, 33, 18u, NULL },
    { WM_DRONE_SC_INPUT, 0x0044u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_spsk2[4] = {
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_uddskk[25] = {
    { WM_DRONE_SC_INPUT, 0x0020u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0000u, 8, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_doham[23] = {
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 10, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 4, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0008u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_hhp3k[11] = {
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0108u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 10, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_hhp4[13] = {
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 10, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_hhp3pd[11] = {
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0101u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 37, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0044u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 10, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_hhsk3pd[11] = {
    { WM_DRONE_SC_INPUT, 0x0110u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0110u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0110u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0110u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 32, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 10, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_doeslap[18] = {
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_SKILL_ABORT, 0x0000u, 0, 0, 0u, "sklrep_t" },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 5, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_dopbig[13] = {
    { WM_DRONE_SC_INPUT, 0x0001u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_usp[1] = {
    { WM_DRONE_SC_INPUT, 0x0024u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_usk[1] = {
    { WM_DRONE_SC_INPUT, 0x0030u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_seek[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_seek@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_seeksp[2] = {
    { WM_DRONE_SC_SEEK, 0x0000u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0004u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_seeksk[2] = {
    { WM_DRONE_SC_SEEK, 0x0000u, 0, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0010u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_retreat[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_retreat@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_fast[8] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x00c0u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0040u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0140u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0100u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0120u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0020u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x00a0u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_chrg[2] = {
    { WM_DRONE_SC_CALL_CODE, 0x0000u, 0, 0, 0u, "drone_chrg" },
    { WM_DRONE_SC_INPUT, 0x0000u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_climbtb[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_climbtb@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_taunt[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_taunt@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_slhtoss[4] = {
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0000u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0080u, 2, 0, 0u, NULL },
    { WM_DRONE_SC_INPUT, 0x0001u, 0, 0, 0u, NULL },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_enterring[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_enterring@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_opinair[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_opinair@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_oprun[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_oprun@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_roll[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_roll@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_inair[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_inair@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_ontb[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_ontb@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_run[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_run@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_combo[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_combo@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_seekclose[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_seekclose@EXGPC_0000" },
};

static const wm_arcade_drone_script_op_t wm_fix39_drone_ops_drn_oppdead[1] = {
    { WM_DRONE_SC_CALL_FUNCTION, 0x0000u, 0, 0, 0u, "drn_oppdead@EXGPC_0000" },
};

static const wm_arcade_drone_script_t wm_fix39_drone_scripts[75] = {
    { "uddsk", wm_fix39_drone_ops_uddsk, 5u },
    { "lrrsp", wm_fix39_drone_ops_lrrsp, 5u },
    { "ucut", wm_fix39_drone_ops_ucut, 1u },
    { "jpx", wm_fix39_drone_ops_jpx, 4u },
    { "rp", wm_fix39_drone_ops_rp, 1u },
    { "rsp", wm_fix39_drone_ops_rsp, 1u },
    { "lrrp", wm_fix39_drone_ops_lrrp, 5u },
    { "k", wm_fix39_drone_ops_k, 1u },
    { "p", wm_fix39_drone_ops_p, 1u },
    { "sp", wm_fix39_drone_ops_sp, 1u },
    { "sk", wm_fix39_drone_ops_sk, 1u },
    { "oghg", wm_fix39_drone_ops_oghg, 2u },
    { "run", wm_fix39_drone_ops_run, 2u },
    { "spx", wm_fix39_drone_ops_spx, 10u },
    { "hgrab", wm_fix39_drone_ops_hgrab, 4u },
    { "flng", wm_fix39_drone_ops_flng, 3u },
    { "htoss", wm_fix39_drone_ops_htoss, 3u },
    { "ddp", wm_fix39_drone_ops_ddp, 4u },
    { "jp", wm_fix39_drone_ops_jp, 4u },
    { "j2sp", wm_fix39_drone_ops_j2sp, 5u },
    { "llsk", wm_fix39_drone_ops_llsk, 4u },
    { "rzup4", wm_fix39_drone_ops_rzup4, 7u },
    { "rzdp4", wm_fix39_drone_ops_rzdp4, 7u },
    { "rzuddksp", wm_fix39_drone_ops_rzuddksp, 19u },
    { "rrk", wm_fix39_drone_ops_rrk, 4u },
    { "uthhrp", wm_fix39_drone_ops_uthhrp, 18u },
    { "uttombhit", wm_fix39_drone_ops_uttombhit, 24u },
    { "utdk", wm_fix39_drone_ops_utdk, 23u },
    { "utchup", wm_fix39_drone_ops_utchup, 18u },
    { "dsk", wm_fix39_drone_ops_dsk, 1u },
    { "utshootps", wm_fix39_drone_ops_utshootps, 8u },
    { "utshootpl", wm_fix39_drone_ops_utshootpl, 8u },
    { "j2p", wm_fix39_drone_ops_j2p, 5u },
    { "rrsk", wm_fix39_drone_ops_rrsk, 4u },
    { "rrp", wm_fix39_drone_ops_rrp, 4u },
    { "spsk", wm_fix39_drone_ops_spsk, 1u },
    { "rsk4k", wm_fix39_drone_ops_rsk4k, 8u },
    { "jkk", wm_fix39_drone_ops_jkk, 12u },
    { "rsp4", wm_fix39_drone_ops_rsp4, 7u },
    { "rk4", wm_fix39_drone_ops_rk4, 7u },
    { "lrrk", wm_fix39_drone_ops_lrrk, 5u },
    { "jisp", wm_fix39_drone_ops_jisp, 4u },
    { "bahhpg", wm_fix39_drone_ops_bahhpg, 24u },
    { "jk", wm_fix39_drone_ops_jk, 4u },
    { "bahhrsk", wm_fix39_drone_ops_bahhrsk, 20u },
    { "spsk2", wm_fix39_drone_ops_spsk2, 4u },
    { "uddskk", wm_fix39_drone_ops_uddskk, 25u },
    { "doham", wm_fix39_drone_ops_doham, 23u },
    { "hhp3k", wm_fix39_drone_ops_hhp3k, 11u },
    { "hhp4", wm_fix39_drone_ops_hhp4, 13u },
    { "hhp3pd", wm_fix39_drone_ops_hhp3pd, 11u },
    { "hhsk3pd", wm_fix39_drone_ops_hhsk3pd, 11u },
    { "doeslap", wm_fix39_drone_ops_doeslap, 18u },
    { "dopbig", wm_fix39_drone_ops_dopbig, 13u },
    { "usp", wm_fix39_drone_ops_usp, 1u },
    { "usk", wm_fix39_drone_ops_usk, 1u },
    { "drn_seek", wm_fix39_drone_ops_drn_seek, 1u },
    { "seeksp", wm_fix39_drone_ops_seeksp, 2u },
    { "seeksk", wm_fix39_drone_ops_seeksk, 2u },
    { "drn_retreat", wm_fix39_drone_ops_drn_retreat, 1u },
    { "fast", wm_fix39_drone_ops_fast, 8u },
    { "chrg", wm_fix39_drone_ops_chrg, 2u },
    { "drn_climbtb", wm_fix39_drone_ops_drn_climbtb, 1u },
    { "drn_taunt", wm_fix39_drone_ops_drn_taunt, 1u },
    { "slhtoss", wm_fix39_drone_ops_slhtoss, 4u },
    { "drn_enterring", wm_fix39_drone_ops_drn_enterring, 1u },
    { "drn_opinair", wm_fix39_drone_ops_drn_opinair, 1u },
    { "drn_oprun", wm_fix39_drone_ops_drn_oprun, 1u },
    { "drn_roll", wm_fix39_drone_ops_drn_roll, 1u },
    { "drn_inair", wm_fix39_drone_ops_drn_inair, 1u },
    { "drn_ontb", wm_fix39_drone_ops_drn_ontb, 1u },
    { "drn_run", wm_fix39_drone_ops_drn_run, 1u },
    { "drn_combo", wm_fix39_drone_ops_drn_combo, 1u },
    { "drn_seekclose", wm_fix39_drone_ops_drn_seekclose, 1u },
    { "drn_oppdead", wm_fix39_drone_ops_drn_oppdead, 1u },
};

static const int16_t wm_fix39_drone_skill_sklrep_t[30] = {
    20, 24, 28, 32, 36, 45, 47, 49, 51, 53,
    55, 59, 63, 67, 71, 75, 77, 79, 81, 83,
    85, 87, 89, 91, 93, 90, 93, 96, 99, 102,
};

static const WmFix39DroneSkillTable wm_fix39_drone_skill_tables[1] = {
    { "sklrep_t", wm_fix39_drone_skill_sklrep_t },
};

#endif
