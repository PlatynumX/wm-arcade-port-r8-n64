#include "wmania_rope_source_data.h"

#include <string.h>

static const WmRopeSourceImagePair source_pairs[] = {
    { "ROPE_S_R", "ROPE_S_Ra", "ROPE_S_Rb" },
    { "ROPE_S_W", "ROPE_S_Wa", "ROPE_S_Wb" },
    { "ROPE_S_B", "ROPE_S_Ba", "ROPE_S_Bb" },
    { "RPSBUP01", "RPSBUP01a", "RPSBUP01b" },
    { "RPSBUP02", "RPSBUP02a", "RPSBUP02b" },
    { "RPSBUP03", "RPSBUP03a", "RPSBUP03b" },
    { "RPSBUP04", "RPSBUP04a", "RPSBUP04b" },
    { "RPSBUP05", "RPSBUP05a", "RPSBUP05b" },
    { "RPSBUP06", "RPSBUP06a", "RPSBUP06b" },
    { "RPSBDN01", "RPSBDN01a", "RPSBDN01b" },
    { "RPSBDN02", "RPSBDN02a", "RPSBDN02b" },
    { "RPSBDN03", "RPSBDN03a", "RPSBDN03b" },
    { "RPSBDN04", "RPSBDN04a", "RPSBDN04b" },
    { "RPSBDN05", "RPSBDN05a", "RPSBDN05b" },
    { "RPSBDN06", "RPSBDN06a", "RPSBDN06b" },
    { "RPSBIN01", "RPSBIN01a", "RPSBIN01b" },
    { "RPSBIN02", "RPSBIN02a", "RPSBIN02b" },
    { "RPSBIN03", "RPSBIN03a", "RPSBIN03b" },
    { "RPSBIN04", "RPSBIN04a", "RPSBIN04b" },
    { "RPSBIN05", "RPSBIN05a", "RPSBIN05b" },
    { "RPSBIN06", "RPSBIN06a", "RPSBIN06b" },
    { "RPSBIN07", "RPSBIN07a", "RPSBIN07b" },
    { "RPSBIN08", "RPSBIN08a", "RPSBIN08b" },
    { "RPSBOU01", "RPSBOU01a", "RPSBOU01b" },
    { "RPSBOU02", "RPSBOU02a", "RPSBOU02b" },
    { "RPSBOU03", "RPSBOU03a", "RPSBOU03b" },
    { "RPSBOU04", "RPSBOU04a", "RPSBOU04b" },
    { "RPSBOU05", "RPSBOU05a", "RPSBOU05b" },
    { "RPSBOU06", "RPSBOU06a", "RPSBOU06b" },
    { "RPSBOU07", "RPSBOU07a", "RPSBOU07b" },
    { "RPSBOU08", "RPSBOU08a", "RPSBOU08b" },
    { "RPSS1_01", "RPSS1_01a", "RPSS1_01b" },
    { "RPSS1_02", "RPSS1_02a", "RPSS1_02b" },
    { "RPSS1_03", "RPSS1_03a", "RPSS1_03b" },
    { "RPSS1_04", "RPSS1_04a", "RPSS1_04b" },
    { "RPSS1_05", "RPSS1_05a", "RPSS1_05b" },
    { "RPSS1_06", "RPSS1_06a", "RPSS1_06b" },
    { "RPSS2_01", "RPSS2_01a", "RPSS2_01b" },
    { "RPSS2_02", "RPSS2_02a", "RPSS2_02b" },
    { "RPSS2_03", "RPSS2_03a", "RPSS2_03b" },
    { "RPSS2_04", "RPSS2_04a", "RPSS2_04b" },
    { "RPSS2_05", "RPSS2_05a", "RPSS2_05b" },
    { "RPSS2_06", "RPSS2_06a", "RPSS2_06b" },
    { "RPSS3_01", "RPSS3_01a", "RPSS3_01b" },
    { "RPSS3_02", "RPSS3_02a", "RPSS3_02b" },
    { "RPSS3_03", "RPSS3_03a", "RPSS3_03b" },
    { "RPSS3_04", "RPSS3_04a", "RPSS3_04b" },
    { "RPSS3_05", "RPSS3_05a", "RPSS3_05b" },
    { "RPSS3_06", "RPSS3_06a", "RPSS3_06b" },
    { "RPSS4_01", "RPSS4_01a", "RPSS4_01b" },
    { "RPSS4_02", "RPSS4_02a", "RPSS4_02b" },
    { "RPSS4_03", "RPSS4_03a", "RPSS4_03b" },
    { "RPSS4_04", "RPSS4_04a", "RPSS4_04b" },
    { "RPSS4_05", "RPSS4_05a", "RPSS4_05b" },
    { "RPSS4_06", "RPSS4_06a", "RPSS4_06b" },
    { "RPSS5_01", "RPSS5_01a", "RPSS5_01b" },
    { "RPSS5_02", "RPSS5_02a", "RPSS5_02b" },
    { "RPSS5_03", "RPSS5_03a", "RPSS5_03b" },
    { "RPSS5_04", "RPSS5_04a", "RPSS5_04b" },
    { "RPSS5_05", "RPSS5_05a", "RPSS5_05b" },
    { "RPSS5_06", "RPSS5_06a", "RPSS5_06b" },
    { "RPDS1_01", "RPDS1_01a", "RPDS1_01b" },
    { "RPDS1_02", "RPDS1_02a", "RPDS1_02b" },
    { "RPDS1_03", "RPDS1_03a", "RPDS1_03b" },
    { "RPDS1_04", "RPDS1_04a", "RPDS1_04b" },
    { "RPDS1_05", "RPDS1_05a", "RPDS1_05b" },
    { "RPDS1_06", "RPDS1_06a", "RPDS1_06b" },
    { "RPDS1_07", "RPDS1_07a", "RPDS1_07b" },
    { "RPDS1_08", "RPDS1_08a", "RPDS1_08b" },
    { "RPDS2_01", "RPDS2_01a", "RPDS2_01b" },
    { "RPDS2_02", "RPDS2_02a", "RPDS2_02b" },
    { "RPDS2_03", "RPDS2_03a", "RPDS2_03b" },
    { "RPDS2_04", "RPDS2_04a", "RPDS2_04b" },
    { "RPDS2_05", "RPDS2_05a", "RPDS2_05b" },
    { "RPDS2_06", "RPDS2_06a", "RPDS2_06b" },
    { "RPDS2_07", "RPDS2_07a", "RPDS2_07b" },
    { "RPDS2_08", "RPDS2_08a", "RPDS2_08b" },
    { "RPDS3_01", "RPDS3_01a", "RPDS3_01b" },
    { "RPDS3_02", "RPDS3_02a", "RPDS3_02b" },
    { "RPDS3_03", "RPDS3_03a", "RPDS3_03b" },
    { "RPDS3_04", "RPDS3_04a", "RPDS3_04b" },
    { "RPDS3_05", "RPDS3_05a", "RPDS3_05b" },
    { "RPDS3_06", "RPDS3_06a", "RPDS3_06b" },
    { "RPDS3_07", "RPDS3_07a", "RPDS3_07b" },
    { "RPDS3_08", "RPDS3_08a", "RPDS3_08b" },
    { "RPDS4_01", "RPDS4_01a", "RPDS4_01b" },
    { "RPDS4_02", "RPDS4_02a", "RPDS4_02b" },
    { "RPDS4_03", "RPDS4_03a", "RPDS4_03b" },
    { "RPDS4_04", "RPDS4_04a", "RPDS4_04b" },
    { "RPDS4_05", "RPDS4_05a", "RPDS4_05b" },
    { "RPDS4_06", "RPDS4_06a", "RPDS4_06b" },
    { "RPDS4_07", "RPDS4_07a", "RPDS4_07b" },
    { "RPDS4_08", "RPDS4_08a", "RPDS4_08b" },
    { "RPDS5_01", "RPDS5_01a", "RPDS5_01b" },
    { "RPDS5_02", "RPDS5_02a", "RPDS5_02b" },
    { "RPDS5_03", "RPDS5_03a", "RPDS5_03b" },
    { "RPDS5_04", "RPDS5_04a", "RPDS5_04b" },
    { "RPDS5_05", "RPDS5_05a", "RPDS5_05b" },
    { "RPDS5_06", "RPDS5_06a", "RPDS5_06b" },
    { "RPDS5_07", "RPDS5_07a", "RPDS5_07b" },
    { "RPDS5_08", "RPDS5_08a", "RPDS5_08b" },
    { "ROPSHAD", "ROPSHADA", "ROPSHADB" },
    { "RCSH1_01", "RCSH1_01A", "RCSH1_01B" },
    { "RCSH1_02", "RCSH1_02A", "RCSH1_02B" },
    { "RCSH1_03", "RCSH1_03A", "RCSH1_03B" },
    { "RCSH1_04", "RCSH1_04A", "RCSH1_04B" },
    { "RCSH1_05", "RCSH1_05A", "RCSH1_05B" },
    { "RCSH2_01", "RCSH2_01A", "RCSH2_01B" },
    { "RCSH2_02", "RCSH2_02A", "RCSH2_02B" },
    { "RCSH2_03", "RCSH2_03A", "RCSH2_03B" },
    { "RCSH2_04", "RCSH2_04A", "RCSH2_04B" },
    { "RCSH2_05", "RCSH2_05A", "RCSH2_05B" },
    { "RCSH3_01", "RCSH3_01A", "RCSH3_01B" },
    { "RCSH3_02", "RCSH3_02A", "RCSH3_02B" },
    { "RCSH3_03", "RCSH3_03A", "RCSH3_03B" },
    { "RCSH3_04", "RCSH3_04A", "RCSH3_04B" },
    { "RCSH3_05", "RCSH3_05A", "RCSH3_05B" },
    { "RCSH4_01", "RCSH4_01A", "RCSH4_01B" },
    { "RCSH4_02", "RCSH4_02A", "RCSH4_02B" },
    { "RCSH4_03", "RCSH4_03A", "RCSH4_03B" },
    { "RCSH4_04", "RCSH4_04A", "RCSH4_04B" },
    { "RCSH4_05", "RCSH4_05A", "RCSH4_05B" },
    { "RCSH5_01", "RCSH5_01A", "RCSH5_01B" },
    { "RCSH5_02", "RCSH5_02A", "RCSH5_02B" },
    { "RCSH5_03", "RCSH5_03A", "RCSH5_03B" },
    { "RCSH5_04", "RCSH5_04A", "RCSH5_04B" },
    { "RCSH5_05", "RCSH5_05A", "RCSH5_05B" },
    { "RBSH_01", "RBSH_01A", "RBSH_01B" },
    { "RBSH_02", "RBSH_02A", "RBSH_02B" },
    { "RBSH_03", "RBSH_03A", "RBSH_03B" },
    { "RBSH_04", "RBSH_04A", "RBSH_04B" },
    { "RBSH_05", "RBSH_05A", "RBSH_05B" },
    { "RBSH_06", "RBSH_06A", "RBSH_06B" },
    { "RBSH_07", "RBSH_07A", "RBSH_07B" },
};

static const WmRopeSequence seq_hash_s_stop;
static const WmRopeSequence seq_hash_s_stop_shadow;
static const WmRopeSequence seq_hash_f_bncud1_W;
static const WmRopeSequence seq_hash_f_bncud2_W;
static const WmRopeSequence seq_hash_f_bncud3;
static const WmRopeSequence seq_hash_f_bncud4;
static const WmRopeSequence seq_hash_f_bncud1_B;
static const WmRopeSequence seq_hash_f_bncud1_R;
static const WmRopeSequence seq_hash_f_bncud2_B;
static const WmRopeSequence seq_hash_f_bncud2_R;
static const WmRopeSequence seq_hash_b_bncud1_W;
static const WmRopeSequence seq_hash_b_bncud1_R;
static const WmRopeSequence seq_hash_b_bncud1_B;
static const WmRopeSequence seq_hash_b_bncud2_W;
static const WmRopeSequence seq_hash_b_bncud2_R;
static const WmRopeSequence seq_hash_b_bncud2_B;
static const WmRopeSequence seq_hash_b_bncud3_B;
static const WmRopeSequence seq_hash_b_bncud4_R;
static const WmRopeSequence seq_hash_b_bncud3_R;
static const WmRopeSequence seq_hash_b_bncud3_W;
static const WmRopeSequence seq_hash_b_bncud4_W;
static const WmRopeSequence seq_hash_b_bncud4_B;
static const WmRopeSequence seq_hash_s_bncud1_R;
static const WmRopeSequence seq_hash_s_bncud1_W;
static const WmRopeSequence seq_hash_s_bncud1_B;
static const WmRopeSequence seq_hash_s_bncud2_R;
static const WmRopeSequence seq_hash_s_bncud2_W;
static const WmRopeSequence seq_hash_s_bncud2_B;
static const WmRopeSequence seq_hash_s_bncud3_B;
static const WmRopeSequence seq_hash_s_bncud4_W;
static const WmRopeSequence seq_hash_s_bncud3_R;
static const WmRopeSequence seq_hash_s_bncud3_W;
static const WmRopeSequence seq_hash_s_bncud4_R;
static const WmRopeSequence seq_hash_s_bncud4_B;
static const WmRopeSequence seq_hash_s_bncud_S;
static const WmRopeSequence seq_hash_s_bncio1_W;
static const WmRopeSequence seq_hash_s_bncio1_B;
static const WmRopeSequence seq_hash_s_bncio1_R;
static const WmRopeSequence seq_hash_s_bncio2_W;
static const WmRopeSequence seq_hash_s_bncio2_B;
static const WmRopeSequence seq_hash_s_bncio2_R;
static const WmRopeSequence seq_hash_s_bncio3;
static const WmRopeSequence seq_hash_s_bncio4;
static const WmRopeSequence seq_hash_s_bncio_S;
static const WmRopeSequence seq_hash_s_sspr11;
static const WmRopeSequence seq_hash_s_sspr12;
static const WmRopeSequence seq_hash_s_sspr13;
static const WmRopeSequence seq_hash_s_sspr14;
static const WmRopeSequence seq_hash_s_sspr15;
static const WmRopeSequence seq_hash_s_sspr16;
static const WmRopeSequence seq_hash_s_sspr21;
static const WmRopeSequence seq_hash_s_sspr22;
static const WmRopeSequence seq_hash_s_sspr23;
static const WmRopeSequence seq_hash_s_sspr24;
static const WmRopeSequence seq_hash_s_sspr25;
static const WmRopeSequence seq_hash_s_sspr26;
static const WmRopeSequence seq_hash_s_sspr31;
static const WmRopeSequence seq_hash_s_sspr32;
static const WmRopeSequence seq_hash_s_sspr33;
static const WmRopeSequence seq_hash_s_sspr34;
static const WmRopeSequence seq_hash_s_sspr35;
static const WmRopeSequence seq_hash_s_sspr36;
static const WmRopeSequence seq_hash_s_sspr41;
static const WmRopeSequence seq_hash_s_sspr42;
static const WmRopeSequence seq_hash_s_sspr43;
static const WmRopeSequence seq_hash_s_sspr44;
static const WmRopeSequence seq_hash_s_sspr45;
static const WmRopeSequence seq_hash_s_sspr46;
static const WmRopeSequence seq_hash_s_sspr51;
static const WmRopeSequence seq_hash_s_sspr52;
static const WmRopeSequence seq_hash_s_sspr53;
static const WmRopeSequence seq_hash_s_sspr54;
static const WmRopeSequence seq_hash_s_sspr55;
static const WmRopeSequence seq_hash_s_sspr56;
static const WmRopeSequence seq_hash_s_sprshad11;
static const WmRopeSequence seq_hash_s_sprshad12;
static const WmRopeSequence seq_hash_s_sprshad13;
static const WmRopeSequence seq_hash_s_sprshad14;
static const WmRopeSequence seq_hash_s_sprshad15;
static const WmRopeSequence seq_hash_s_sprshad21;
static const WmRopeSequence seq_hash_s_sprshad22;
static const WmRopeSequence seq_hash_s_sprshad23;
static const WmRopeSequence seq_hash_s_sprshad24;
static const WmRopeSequence seq_hash_s_sprshad25;
static const WmRopeSequence seq_hash_s_sprshad31;
static const WmRopeSequence seq_hash_s_sprshad32;
static const WmRopeSequence seq_hash_s_sprshad33;
static const WmRopeSequence seq_hash_s_sprshad34;
static const WmRopeSequence seq_hash_s_sprshad35;
static const WmRopeSequence seq_hash_s_sprshad41;
static const WmRopeSequence seq_hash_s_sprshad42;
static const WmRopeSequence seq_hash_s_sprshad43;
static const WmRopeSequence seq_hash_s_sprshad44;
static const WmRopeSequence seq_hash_s_sprshad45;
static const WmRopeSequence seq_hash_s_sprshad51;
static const WmRopeSequence seq_hash_s_sprshad52;
static const WmRopeSequence seq_hash_s_sprshad53;
static const WmRopeSequence seq_hash_s_sprshad54;
static const WmRopeSequence seq_hash_s_sprshad55;
static const WmRopeSequence seq_hash_s_sspr_trans_W;
static const WmRopeSequence seq_hash_s_sspr_trans_R;
static const WmRopeSequence seq_hash_s_sprshad_trans;
static const WmRopeSequence seq_hash_s_dsprshad;
static const WmRopeSequence seq_hash_s_dspr11;
static const WmRopeSequence seq_hash_s_dspr12;
static const WmRopeSequence seq_hash_s_dspr13;
static const WmRopeSequence seq_hash_s_dspr14;
static const WmRopeSequence seq_hash_s_dspr15;
static const WmRopeSequence seq_hash_s_dspr16;
static const WmRopeSequence seq_hash_s_dspr17;
static const WmRopeSequence seq_hash_s_dspr21;
static const WmRopeSequence seq_hash_s_dspr22;
static const WmRopeSequence seq_hash_s_dspr23;
static const WmRopeSequence seq_hash_s_dspr24;
static const WmRopeSequence seq_hash_s_dspr25;
static const WmRopeSequence seq_hash_s_dspr26;
static const WmRopeSequence seq_hash_s_dspr27;
static const WmRopeSequence seq_hash_s_dspr31;
static const WmRopeSequence seq_hash_s_dspr32;
static const WmRopeSequence seq_hash_s_dspr33;
static const WmRopeSequence seq_hash_s_dspr34;
static const WmRopeSequence seq_hash_s_dspr35;
static const WmRopeSequence seq_hash_s_dspr36;
static const WmRopeSequence seq_hash_s_dspr37;
static const WmRopeSequence seq_hash_s_dspr41;
static const WmRopeSequence seq_hash_s_dspr42;
static const WmRopeSequence seq_hash_s_dspr43;
static const WmRopeSequence seq_hash_s_dspr44;
static const WmRopeSequence seq_hash_s_dspr45;
static const WmRopeSequence seq_hash_s_dspr46;
static const WmRopeSequence seq_hash_s_dspr47;
static const WmRopeSequence seq_hash_s_dspr51;
static const WmRopeSequence seq_hash_s_dspr52;
static const WmRopeSequence seq_hash_s_dspr53;
static const WmRopeSequence seq_hash_s_dspr54;
static const WmRopeSequence seq_hash_s_dspr55;
static const WmRopeSequence seq_hash_s_dspr56;
static const WmRopeSequence seq_hash_s_dspr57;
static const WmRopeSequence seq_hash_s_dspr_trans_W;
static const WmRopeSequence seq_hash_s_dspr_trans_R;

static const WmRopeFrame frames_hash_s_stop[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_stop = { "#s_stop", frames_hash_s_stop, sizeof(frames_hash_s_stop)/sizeof(frames_hash_s_stop[0]) };

static const WmRopeFrame frames_hash_s_stop_shadow[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPSHADA", "ROPSHADB", 0 },
};
static const WmRopeSequence seq_hash_s_stop_shadow = { "#s_stop_shadow", frames_hash_s_stop_shadow, sizeof(frames_hash_s_stop_shadow)/sizeof(frames_hash_s_stop_shadow[0]) };

static const WmRopeFrame frames_hash_f_bncud1_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud1_W = { "#f_bncud1_W", frames_hash_f_bncud1_W, sizeof(frames_hash_f_bncud1_W)/sizeof(frames_hash_f_bncud1_W[0]) };

static const WmRopeFrame frames_hash_f_bncud2_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud2_W = { "#f_bncud2_W", frames_hash_f_bncud2_W, sizeof(frames_hash_f_bncud2_W)/sizeof(frames_hash_f_bncud2_W[0]) };

static const WmRopeFrame frames_hash_f_bncud3[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud3 = { "#f_bncud3", frames_hash_f_bncud3, sizeof(frames_hash_f_bncud3)/sizeof(frames_hash_f_bncud3[0]) };

static const WmRopeFrame frames_hash_f_bncud4[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud4 = { "#f_bncud4", frames_hash_f_bncud4, sizeof(frames_hash_f_bncud4)/sizeof(frames_hash_f_bncud4[0]) };

static const WmRopeFrame frames_hash_f_bncud1_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud1_B = { "#f_bncud1_B", frames_hash_f_bncud1_B, sizeof(frames_hash_f_bncud1_B)/sizeof(frames_hash_f_bncud1_B[0]) };

static const WmRopeFrame frames_hash_f_bncud1_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud1_R = { "#f_bncud1_R", frames_hash_f_bncud1_R, sizeof(frames_hash_f_bncud1_R)/sizeof(frames_hash_f_bncud1_R[0]) };

static const WmRopeFrame frames_hash_f_bncud2_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud2_B = { "#f_bncud2_B", frames_hash_f_bncud2_B, sizeof(frames_hash_f_bncud2_B)/sizeof(frames_hash_f_bncud2_B[0]) };

static const WmRopeFrame frames_hash_f_bncud2_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPFBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_F_R", 0, 0 },
};
static const WmRopeSequence seq_hash_f_bncud2_R = { "#f_bncud2_R", frames_hash_f_bncud2_R, sizeof(frames_hash_f_bncud2_R)/sizeof(frames_hash_f_bncud2_R[0]) };

static const WmRopeFrame frames_hash_b_bncud1_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud1_W = { "#b_bncud1_W", frames_hash_b_bncud1_W, sizeof(frames_hash_b_bncud1_W)/sizeof(frames_hash_b_bncud1_W[0]) };

static const WmRopeFrame frames_hash_b_bncud1_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud1_R = { "#b_bncud1_R", frames_hash_b_bncud1_R, sizeof(frames_hash_b_bncud1_R)/sizeof(frames_hash_b_bncud1_R[0]) };

static const WmRopeFrame frames_hash_b_bncud1_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN05", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud1_B = { "#b_bncud1_B", frames_hash_b_bncud1_B, sizeof(frames_hash_b_bncud1_B)/sizeof(frames_hash_b_bncud1_B[0]) };

static const WmRopeFrame frames_hash_b_bncud2_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud2_W = { "#b_bncud2_W", frames_hash_b_bncud2_W, sizeof(frames_hash_b_bncud2_W)/sizeof(frames_hash_b_bncud2_W[0]) };

static const WmRopeFrame frames_hash_b_bncud2_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud2_R = { "#b_bncud2_R", frames_hash_b_bncud2_R, sizeof(frames_hash_b_bncud2_R)/sizeof(frames_hash_b_bncud2_R[0]) };

static const WmRopeFrame frames_hash_b_bncud2_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN04", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud2_B = { "#b_bncud2_B", frames_hash_b_bncud2_B, sizeof(frames_hash_b_bncud2_B)/sizeof(frames_hash_b_bncud2_B[0]) };

static const WmRopeFrame frames_hash_b_bncud3_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud3_B = { "#b_bncud3_B", frames_hash_b_bncud3_B, sizeof(frames_hash_b_bncud3_B)/sizeof(frames_hash_b_bncud3_B[0]) };

static const WmRopeFrame frames_hash_b_bncud4_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud4_R = { "#b_bncud4_R", frames_hash_b_bncud4_R, sizeof(frames_hash_b_bncud4_R)/sizeof(frames_hash_b_bncud4_R[0]) };

static const WmRopeFrame frames_hash_b_bncud3_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud3_R = { "#b_bncud3_R", frames_hash_b_bncud3_R, sizeof(frames_hash_b_bncud3_R)/sizeof(frames_hash_b_bncud3_R[0]) };

static const WmRopeFrame frames_hash_b_bncud3_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN03", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud3_W = { "#b_bncud3_W", frames_hash_b_bncud3_W, sizeof(frames_hash_b_bncud3_W)/sizeof(frames_hash_b_bncud3_W[0]) };

static const WmRopeFrame frames_hash_b_bncud4_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud4_W = { "#b_bncud4_W", frames_hash_b_bncud4_W, sizeof(frames_hash_b_bncud4_W)/sizeof(frames_hash_b_bncud4_W[0]) };

static const WmRopeFrame frames_hash_b_bncud4_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBUP01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN02", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPBBDN01", 0, 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_B_R", 0, 0 },
};
static const WmRopeSequence seq_hash_b_bncud4_B = { "#b_bncud4_B", frames_hash_b_bncud4_B, sizeof(frames_hash_b_bncud4_B)/sizeof(frames_hash_b_bncud4_B[0]) };

static const WmRopeFrame frames_hash_s_bncud1_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP05a", "RPSBUP05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP04a", "RPSBUP04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN05a", "RPSBDN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud1_R = { "#s_bncud1_R", frames_hash_s_bncud1_R, sizeof(frames_hash_s_bncud1_R)/sizeof(frames_hash_s_bncud1_R[0]) };

static const WmRopeFrame frames_hash_s_bncud1_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP05a", "RPSBUP05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP04a", "RPSBUP04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN05a", "RPSBDN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud1_W = { "#s_bncud1_W", frames_hash_s_bncud1_W, sizeof(frames_hash_s_bncud1_W)/sizeof(frames_hash_s_bncud1_W[0]) };

static const WmRopeFrame frames_hash_s_bncud1_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP05a", "RPSBUP05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP04a", "RPSBUP04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN05a", "RPSBDN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud1_B = { "#s_bncud1_B", frames_hash_s_bncud1_B, sizeof(frames_hash_s_bncud1_B)/sizeof(frames_hash_s_bncud1_B[0]) };

static const WmRopeFrame frames_hash_s_bncud2_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP04a", "RPSBUP04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud2_R = { "#s_bncud2_R", frames_hash_s_bncud2_R, sizeof(frames_hash_s_bncud2_R)/sizeof(frames_hash_s_bncud2_R[0]) };

static const WmRopeFrame frames_hash_s_bncud2_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP04a", "RPSBUP04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud2_W = { "#s_bncud2_W", frames_hash_s_bncud2_W, sizeof(frames_hash_s_bncud2_W)/sizeof(frames_hash_s_bncud2_W[0]) };

static const WmRopeFrame frames_hash_s_bncud2_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP04a", "RPSBUP04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud2_B = { "#s_bncud2_B", frames_hash_s_bncud2_B, sizeof(frames_hash_s_bncud2_B)/sizeof(frames_hash_s_bncud2_B[0]) };

static const WmRopeFrame frames_hash_s_bncud3_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud3_B = { "#s_bncud3_B", frames_hash_s_bncud3_B, sizeof(frames_hash_s_bncud3_B)/sizeof(frames_hash_s_bncud3_B[0]) };

static const WmRopeFrame frames_hash_s_bncud4_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud4_W = { "#s_bncud4_W", frames_hash_s_bncud4_W, sizeof(frames_hash_s_bncud4_W)/sizeof(frames_hash_s_bncud4_W[0]) };

static const WmRopeFrame frames_hash_s_bncud3_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud3_R = { "#s_bncud3_R", frames_hash_s_bncud3_R, sizeof(frames_hash_s_bncud3_R)/sizeof(frames_hash_s_bncud3_R[0]) };

static const WmRopeFrame frames_hash_s_bncud3_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP03a", "RPSBUP03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud3_W = { "#s_bncud3_W", frames_hash_s_bncud3_W, sizeof(frames_hash_s_bncud3_W)/sizeof(frames_hash_s_bncud3_W[0]) };

static const WmRopeFrame frames_hash_s_bncud4_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud4_R = { "#s_bncud4_R", frames_hash_s_bncud4_R, sizeof(frames_hash_s_bncud4_R)/sizeof(frames_hash_s_bncud4_R[0]) };

static const WmRopeFrame frames_hash_s_bncud4_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP01a", "RPSBUP01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncud4_B = { "#s_bncud4_B", frames_hash_s_bncud4_B, sizeof(frames_hash_s_bncud4_B)/sizeof(frames_hash_s_bncud4_B[0]) };

static const WmRopeFrame frames_hash_s_bncud_S[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPSHADA", "ROPSHADB", 0 },
};
static const WmRopeSequence seq_hash_s_bncud_S = { "#s_bncud_S", frames_hash_s_bncud_S, sizeof(frames_hash_s_bncud_S)/sizeof(frames_hash_s_bncud_S[0]) };

static const WmRopeFrame frames_hash_s_bncio1_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN05a", "RPSBIN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN04a", "RPSBIN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU01a", "RPSBOU01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU05a", "RPSBOU05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio1_W = { "#s_bncio1_W", frames_hash_s_bncio1_W, sizeof(frames_hash_s_bncio1_W)/sizeof(frames_hash_s_bncio1_W[0]) };

static const WmRopeFrame frames_hash_s_bncio1_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN05a", "RPSBIN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN04a", "RPSBIN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU01a", "RPSBOU01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU05a", "RPSBOU05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio1_B = { "#s_bncio1_B", frames_hash_s_bncio1_B, sizeof(frames_hash_s_bncio1_B)/sizeof(frames_hash_s_bncio1_B[0]) };

static const WmRopeFrame frames_hash_s_bncio1_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN02a", "RPSBIN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN05a", "RPSBIN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN04a", "RPSBIN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU01a", "RPSBOU01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU05a", "RPSBOU05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio1_R = { "#s_bncio1_R", frames_hash_s_bncio1_R, sizeof(frames_hash_s_bncio1_R)/sizeof(frames_hash_s_bncio1_R[0]) };

static const WmRopeFrame frames_hash_s_bncio2_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN04a", "RPSBIN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio2_W = { "#s_bncio2_W", frames_hash_s_bncio2_W, sizeof(frames_hash_s_bncio2_W)/sizeof(frames_hash_s_bncio2_W[0]) };

static const WmRopeFrame frames_hash_s_bncio2_B[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN04a", "RPSBIN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio2_B = { "#s_bncio2_B", frames_hash_s_bncio2_B, sizeof(frames_hash_s_bncio2_B)/sizeof(frames_hash_s_bncio2_B[0]) };

static const WmRopeFrame frames_hash_s_bncio2_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN02a", "RPSBIN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN04a", "RPSBIN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio2_R = { "#s_bncio2_R", frames_hash_s_bncio2_R, sizeof(frames_hash_s_bncio2_R)/sizeof(frames_hash_s_bncio2_R[0]) };

static const WmRopeFrame frames_hash_s_bncio3[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN02a", "RPSBIN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN03a", "RPSBIN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN02a", "RPSBIN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio3 = { "#s_bncio3", frames_hash_s_bncio3, sizeof(frames_hash_s_bncio3)/sizeof(frames_hash_s_bncio3[0]) };

static const WmRopeFrame frames_hash_s_bncio4[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN02a", "RPSBIN02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN01a", "RPSBIN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU01a", "RPSBOU01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU01a", "RPSBOU01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
};
static const WmRopeSequence seq_hash_s_bncio4 = { "#s_bncio4", frames_hash_s_bncio4, sizeof(frames_hash_s_bncio4)/sizeof(frames_hash_s_bncio4[0]) };

static const WmRopeFrame frames_hash_s_bncio_S[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPSHADA", "ROPSHADB", 0 },
};
static const WmRopeSequence seq_hash_s_bncio_S = { "#s_bncio_S", frames_hash_s_bncio_S, sizeof(frames_hash_s_bncio_S)/sizeof(frames_hash_s_bncio_S[0]) };

static const WmRopeFrame frames_hash_s_sspr11[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS1_01a", "RPSS1_01b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr11 = { "#s_sspr11", frames_hash_s_sspr11, sizeof(frames_hash_s_sspr11)/sizeof(frames_hash_s_sspr11[0]) };

static const WmRopeFrame frames_hash_s_sspr12[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS1_02a", "RPSS1_02b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr12 = { "#s_sspr12", frames_hash_s_sspr12, sizeof(frames_hash_s_sspr12)/sizeof(frames_hash_s_sspr12[0]) };

static const WmRopeFrame frames_hash_s_sspr13[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS1_03a", "RPSS1_03b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr13 = { "#s_sspr13", frames_hash_s_sspr13, sizeof(frames_hash_s_sspr13)/sizeof(frames_hash_s_sspr13[0]) };

static const WmRopeFrame frames_hash_s_sspr14[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS1_04a", "RPSS1_04b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr14 = { "#s_sspr14", frames_hash_s_sspr14, sizeof(frames_hash_s_sspr14)/sizeof(frames_hash_s_sspr14[0]) };

static const WmRopeFrame frames_hash_s_sspr15[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS1_05a", "RPSS1_05b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr15 = { "#s_sspr15", frames_hash_s_sspr15, sizeof(frames_hash_s_sspr15)/sizeof(frames_hash_s_sspr15[0]) };

static const WmRopeFrame frames_hash_s_sspr16[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS1_06a", "RPSS1_06b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr16 = { "#s_sspr16", frames_hash_s_sspr16, sizeof(frames_hash_s_sspr16)/sizeof(frames_hash_s_sspr16[0]) };

static const WmRopeFrame frames_hash_s_sspr21[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS2_01a", "RPSS2_01b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr21 = { "#s_sspr21", frames_hash_s_sspr21, sizeof(frames_hash_s_sspr21)/sizeof(frames_hash_s_sspr21[0]) };

static const WmRopeFrame frames_hash_s_sspr22[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS2_02a", "RPSS2_02b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr22 = { "#s_sspr22", frames_hash_s_sspr22, sizeof(frames_hash_s_sspr22)/sizeof(frames_hash_s_sspr22[0]) };

static const WmRopeFrame frames_hash_s_sspr23[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS2_03a", "RPSS2_03b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr23 = { "#s_sspr23", frames_hash_s_sspr23, sizeof(frames_hash_s_sspr23)/sizeof(frames_hash_s_sspr23[0]) };

static const WmRopeFrame frames_hash_s_sspr24[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS2_04a", "RPSS2_04b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr24 = { "#s_sspr24", frames_hash_s_sspr24, sizeof(frames_hash_s_sspr24)/sizeof(frames_hash_s_sspr24[0]) };

static const WmRopeFrame frames_hash_s_sspr25[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS2_05a", "RPSS2_05b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr25 = { "#s_sspr25", frames_hash_s_sspr25, sizeof(frames_hash_s_sspr25)/sizeof(frames_hash_s_sspr25[0]) };

static const WmRopeFrame frames_hash_s_sspr26[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS2_06a", "RPSS2_06b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr26 = { "#s_sspr26", frames_hash_s_sspr26, sizeof(frames_hash_s_sspr26)/sizeof(frames_hash_s_sspr26[0]) };

static const WmRopeFrame frames_hash_s_sspr31[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS3_01a", "RPSS3_01b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr31 = { "#s_sspr31", frames_hash_s_sspr31, sizeof(frames_hash_s_sspr31)/sizeof(frames_hash_s_sspr31[0]) };

static const WmRopeFrame frames_hash_s_sspr32[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS3_02a", "RPSS3_02b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr32 = { "#s_sspr32", frames_hash_s_sspr32, sizeof(frames_hash_s_sspr32)/sizeof(frames_hash_s_sspr32[0]) };

static const WmRopeFrame frames_hash_s_sspr33[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS3_03a", "RPSS3_03b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr33 = { "#s_sspr33", frames_hash_s_sspr33, sizeof(frames_hash_s_sspr33)/sizeof(frames_hash_s_sspr33[0]) };

static const WmRopeFrame frames_hash_s_sspr34[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS3_04a", "RPSS3_04b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr34 = { "#s_sspr34", frames_hash_s_sspr34, sizeof(frames_hash_s_sspr34)/sizeof(frames_hash_s_sspr34[0]) };

static const WmRopeFrame frames_hash_s_sspr35[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS3_05a", "RPSS3_05b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr35 = { "#s_sspr35", frames_hash_s_sspr35, sizeof(frames_hash_s_sspr35)/sizeof(frames_hash_s_sspr35[0]) };

static const WmRopeFrame frames_hash_s_sspr36[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS3_06a", "RPSS3_06b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr36 = { "#s_sspr36", frames_hash_s_sspr36, sizeof(frames_hash_s_sspr36)/sizeof(frames_hash_s_sspr36[0]) };

static const WmRopeFrame frames_hash_s_sspr41[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS4_01a", "RPSS4_01b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr41 = { "#s_sspr41", frames_hash_s_sspr41, sizeof(frames_hash_s_sspr41)/sizeof(frames_hash_s_sspr41[0]) };

static const WmRopeFrame frames_hash_s_sspr42[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS4_02a", "RPSS4_02b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr42 = { "#s_sspr42", frames_hash_s_sspr42, sizeof(frames_hash_s_sspr42)/sizeof(frames_hash_s_sspr42[0]) };

static const WmRopeFrame frames_hash_s_sspr43[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS4_03a", "RPSS4_03b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr43 = { "#s_sspr43", frames_hash_s_sspr43, sizeof(frames_hash_s_sspr43)/sizeof(frames_hash_s_sspr43[0]) };

static const WmRopeFrame frames_hash_s_sspr44[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS4_04a", "RPSS4_04b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr44 = { "#s_sspr44", frames_hash_s_sspr44, sizeof(frames_hash_s_sspr44)/sizeof(frames_hash_s_sspr44[0]) };

static const WmRopeFrame frames_hash_s_sspr45[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS4_05a", "RPSS4_05b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr45 = { "#s_sspr45", frames_hash_s_sspr45, sizeof(frames_hash_s_sspr45)/sizeof(frames_hash_s_sspr45[0]) };

static const WmRopeFrame frames_hash_s_sspr46[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS4_06a", "RPSS4_06b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr46 = { "#s_sspr46", frames_hash_s_sspr46, sizeof(frames_hash_s_sspr46)/sizeof(frames_hash_s_sspr46[0]) };

static const WmRopeFrame frames_hash_s_sspr51[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS5_01a", "RPSS5_01b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr51 = { "#s_sspr51", frames_hash_s_sspr51, sizeof(frames_hash_s_sspr51)/sizeof(frames_hash_s_sspr51[0]) };

static const WmRopeFrame frames_hash_s_sspr52[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS5_02a", "RPSS5_02b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr52 = { "#s_sspr52", frames_hash_s_sspr52, sizeof(frames_hash_s_sspr52)/sizeof(frames_hash_s_sspr52[0]) };

static const WmRopeFrame frames_hash_s_sspr53[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS5_03a", "RPSS5_03b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr53 = { "#s_sspr53", frames_hash_s_sspr53, sizeof(frames_hash_s_sspr53)/sizeof(frames_hash_s_sspr53[0]) };

static const WmRopeFrame frames_hash_s_sspr54[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS5_04a", "RPSS5_04b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr54 = { "#s_sspr54", frames_hash_s_sspr54, sizeof(frames_hash_s_sspr54)/sizeof(frames_hash_s_sspr54[0]) };

static const WmRopeFrame frames_hash_s_sspr55[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS5_05a", "RPSS5_05b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr55 = { "#s_sspr55", frames_hash_s_sspr55, sizeof(frames_hash_s_sspr55)/sizeof(frames_hash_s_sspr55[0]) };

static const WmRopeFrame frames_hash_s_sspr56[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RPSS5_06a", "RPSS5_06b", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop },
};
static const WmRopeSequence seq_hash_s_sspr56 = { "#s_sspr56", frames_hash_s_sspr56, sizeof(frames_hash_s_sspr56)/sizeof(frames_hash_s_sspr56[0]) };

static const WmRopeFrame frames_hash_s_sprshad11[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH1_01A", "RCSH1_01B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad11 = { "#s_sprshad11", frames_hash_s_sprshad11, sizeof(frames_hash_s_sprshad11)/sizeof(frames_hash_s_sprshad11[0]) };

static const WmRopeFrame frames_hash_s_sprshad12[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH1_02A", "RCSH1_02B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad12 = { "#s_sprshad12", frames_hash_s_sprshad12, sizeof(frames_hash_s_sprshad12)/sizeof(frames_hash_s_sprshad12[0]) };

static const WmRopeFrame frames_hash_s_sprshad13[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH1_03A", "RCSH1_03B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad13 = { "#s_sprshad13", frames_hash_s_sprshad13, sizeof(frames_hash_s_sprshad13)/sizeof(frames_hash_s_sprshad13[0]) };

static const WmRopeFrame frames_hash_s_sprshad14[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH1_04A", "RCSH1_04B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad14 = { "#s_sprshad14", frames_hash_s_sprshad14, sizeof(frames_hash_s_sprshad14)/sizeof(frames_hash_s_sprshad14[0]) };

static const WmRopeFrame frames_hash_s_sprshad15[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH1_05A", "RCSH1_05B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad15 = { "#s_sprshad15", frames_hash_s_sprshad15, sizeof(frames_hash_s_sprshad15)/sizeof(frames_hash_s_sprshad15[0]) };

static const WmRopeFrame frames_hash_s_sprshad21[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH2_01A", "RCSH2_01B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad21 = { "#s_sprshad21", frames_hash_s_sprshad21, sizeof(frames_hash_s_sprshad21)/sizeof(frames_hash_s_sprshad21[0]) };

static const WmRopeFrame frames_hash_s_sprshad22[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH2_02A", "RCSH2_02B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad22 = { "#s_sprshad22", frames_hash_s_sprshad22, sizeof(frames_hash_s_sprshad22)/sizeof(frames_hash_s_sprshad22[0]) };

static const WmRopeFrame frames_hash_s_sprshad23[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH2_03A", "RCSH2_03B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad23 = { "#s_sprshad23", frames_hash_s_sprshad23, sizeof(frames_hash_s_sprshad23)/sizeof(frames_hash_s_sprshad23[0]) };

static const WmRopeFrame frames_hash_s_sprshad24[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH2_04A", "RCSH2_04B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad24 = { "#s_sprshad24", frames_hash_s_sprshad24, sizeof(frames_hash_s_sprshad24)/sizeof(frames_hash_s_sprshad24[0]) };

static const WmRopeFrame frames_hash_s_sprshad25[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH2_05A", "RCSH2_05B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad25 = { "#s_sprshad25", frames_hash_s_sprshad25, sizeof(frames_hash_s_sprshad25)/sizeof(frames_hash_s_sprshad25[0]) };

static const WmRopeFrame frames_hash_s_sprshad31[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH3_01A", "RCSH3_01B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad31 = { "#s_sprshad31", frames_hash_s_sprshad31, sizeof(frames_hash_s_sprshad31)/sizeof(frames_hash_s_sprshad31[0]) };

static const WmRopeFrame frames_hash_s_sprshad32[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH3_02A", "RCSH3_02B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad32 = { "#s_sprshad32", frames_hash_s_sprshad32, sizeof(frames_hash_s_sprshad32)/sizeof(frames_hash_s_sprshad32[0]) };

static const WmRopeFrame frames_hash_s_sprshad33[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH3_03A", "RCSH3_03B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad33 = { "#s_sprshad33", frames_hash_s_sprshad33, sizeof(frames_hash_s_sprshad33)/sizeof(frames_hash_s_sprshad33[0]) };

static const WmRopeFrame frames_hash_s_sprshad34[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH3_04A", "RCSH3_04B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad34 = { "#s_sprshad34", frames_hash_s_sprshad34, sizeof(frames_hash_s_sprshad34)/sizeof(frames_hash_s_sprshad34[0]) };

static const WmRopeFrame frames_hash_s_sprshad35[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH3_05A", "RCSH3_05B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad35 = { "#s_sprshad35", frames_hash_s_sprshad35, sizeof(frames_hash_s_sprshad35)/sizeof(frames_hash_s_sprshad35[0]) };

static const WmRopeFrame frames_hash_s_sprshad41[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH4_01A", "RCSH4_01B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad41 = { "#s_sprshad41", frames_hash_s_sprshad41, sizeof(frames_hash_s_sprshad41)/sizeof(frames_hash_s_sprshad41[0]) };

static const WmRopeFrame frames_hash_s_sprshad42[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH4_02A", "RCSH4_02B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad42 = { "#s_sprshad42", frames_hash_s_sprshad42, sizeof(frames_hash_s_sprshad42)/sizeof(frames_hash_s_sprshad42[0]) };

static const WmRopeFrame frames_hash_s_sprshad43[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH4_03A", "RCSH4_03B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad43 = { "#s_sprshad43", frames_hash_s_sprshad43, sizeof(frames_hash_s_sprshad43)/sizeof(frames_hash_s_sprshad43[0]) };

static const WmRopeFrame frames_hash_s_sprshad44[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH4_04A", "RCSH4_04B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad44 = { "#s_sprshad44", frames_hash_s_sprshad44, sizeof(frames_hash_s_sprshad44)/sizeof(frames_hash_s_sprshad44[0]) };

static const WmRopeFrame frames_hash_s_sprshad45[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH4_05A", "RCSH4_05B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad45 = { "#s_sprshad45", frames_hash_s_sprshad45, sizeof(frames_hash_s_sprshad45)/sizeof(frames_hash_s_sprshad45[0]) };

static const WmRopeFrame frames_hash_s_sprshad51[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH5_01A", "RCSH5_01B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad51 = { "#s_sprshad51", frames_hash_s_sprshad51, sizeof(frames_hash_s_sprshad51)/sizeof(frames_hash_s_sprshad51[0]) };

static const WmRopeFrame frames_hash_s_sprshad52[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH5_02A", "RCSH5_02B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad52 = { "#s_sprshad52", frames_hash_s_sprshad52, sizeof(frames_hash_s_sprshad52)/sizeof(frames_hash_s_sprshad52[0]) };

static const WmRopeFrame frames_hash_s_sprshad53[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH5_03A", "RCSH5_03B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad53 = { "#s_sprshad53", frames_hash_s_sprshad53, sizeof(frames_hash_s_sprshad53)/sizeof(frames_hash_s_sprshad53[0]) };

static const WmRopeFrame frames_hash_s_sprshad54[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH5_04A", "RCSH5_04B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad54 = { "#s_sprshad54", frames_hash_s_sprshad54, sizeof(frames_hash_s_sprshad54)/sizeof(frames_hash_s_sprshad54[0]) };

static const WmRopeFrame frames_hash_s_sprshad55[] = {
    { WM_ROPE_FRAME_IMAGE, 10u, "RCSH5_05A", "RCSH5_05B", 0 },
    { WM_ROPE_FRAME_GOTO, 0u, 0, 0, &seq_hash_s_stop_shadow },
};
static const WmRopeSequence seq_hash_s_sprshad55 = { "#s_sprshad55", frames_hash_s_sprshad55, sizeof(frames_hash_s_sprshad55)/sizeof(frames_hash_s_sprshad55[0]) };

static const WmRopeFrame frames_hash_s_sspr_trans_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
};
static const WmRopeSequence seq_hash_s_sspr_trans_W = { "#s_sspr_trans_W", frames_hash_s_sspr_trans_W, sizeof(frames_hash_s_sspr_trans_W)/sizeof(frames_hash_s_sspr_trans_W[0]) };

static const WmRopeFrame frames_hash_s_sspr_trans_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN06a", "RPSBIN06b", 0 },
    { WM_ROPE_FRAME_IMAGE, 2u, "RPSBIN08a", "RPSBIN08b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN07a", "RPSBIN07b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBIN06a", "RPSBIN06b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPE_S_Ra", "ROPE_S_Rb", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU01a", "RPSBOU01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU06a", "RPSBOU06b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU04a", "RPSBOU04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU03a", "RPSBOU03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBOU02a", "RPSBOU02b", 0 },
};
static const WmRopeSequence seq_hash_s_sspr_trans_R = { "#s_sspr_trans_R", frames_hash_s_sspr_trans_R, sizeof(frames_hash_s_sspr_trans_R)/sizeof(frames_hash_s_sspr_trans_R[0]) };

static const WmRopeFrame frames_hash_s_sprshad_trans[] = {
    { WM_ROPE_FRAME_IMAGE, 2u, "RBSH_02A", "RBSH_02B", 0 },
    { WM_ROPE_FRAME_IMAGE, 2u, "RBSH_03A", "RBSH_03B", 0 },
    { WM_ROPE_FRAME_IMAGE, 2u, "RBSH_04A", "RBSH_04B", 0 },
    { WM_ROPE_FRAME_IMAGE, 2u, "RBSH_05A", "RBSH_05B", 0 },
    { WM_ROPE_FRAME_IMAGE, 2u, "RBSH_06A", "RBSH_06B", 0 },
    { WM_ROPE_FRAME_IMAGE, 2u, "RBSH_07A", "RBSH_07B", 0 },
    { WM_ROPE_FRAME_IMAGE, 2u, "RBSH_01A", "RBSH_01B", 0 },
};
static const WmRopeSequence seq_hash_s_sprshad_trans = { "#s_sprshad_trans", frames_hash_s_sprshad_trans, sizeof(frames_hash_s_sprshad_trans)/sizeof(frames_hash_s_sprshad_trans[0]) };

static const WmRopeFrame frames_hash_s_dsprshad[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "ROPSHADA", "ROPSHADB", 0 },
};
static const WmRopeSequence seq_hash_s_dsprshad = { "#s_dsprshad", frames_hash_s_dsprshad, sizeof(frames_hash_s_dsprshad)/sizeof(frames_hash_s_dsprshad[0]) };

static const WmRopeFrame frames_hash_s_dspr11[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS1_01a", "RPDS1_01b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr11 = { "#s_dspr11", frames_hash_s_dspr11, sizeof(frames_hash_s_dspr11)/sizeof(frames_hash_s_dspr11[0]) };

static const WmRopeFrame frames_hash_s_dspr12[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS1_02a", "RPDS1_02b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr12 = { "#s_dspr12", frames_hash_s_dspr12, sizeof(frames_hash_s_dspr12)/sizeof(frames_hash_s_dspr12[0]) };

static const WmRopeFrame frames_hash_s_dspr13[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS1_03a", "RPDS1_03b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr13 = { "#s_dspr13", frames_hash_s_dspr13, sizeof(frames_hash_s_dspr13)/sizeof(frames_hash_s_dspr13[0]) };

static const WmRopeFrame frames_hash_s_dspr14[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS1_04a", "RPDS1_04b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr14 = { "#s_dspr14", frames_hash_s_dspr14, sizeof(frames_hash_s_dspr14)/sizeof(frames_hash_s_dspr14[0]) };

static const WmRopeFrame frames_hash_s_dspr15[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS1_05a", "RPDS1_05b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr15 = { "#s_dspr15", frames_hash_s_dspr15, sizeof(frames_hash_s_dspr15)/sizeof(frames_hash_s_dspr15[0]) };

static const WmRopeFrame frames_hash_s_dspr16[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS1_06a", "RPDS1_06b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr16 = { "#s_dspr16", frames_hash_s_dspr16, sizeof(frames_hash_s_dspr16)/sizeof(frames_hash_s_dspr16[0]) };

static const WmRopeFrame frames_hash_s_dspr17[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS1_07a", "RPDS1_07b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr17 = { "#s_dspr17", frames_hash_s_dspr17, sizeof(frames_hash_s_dspr17)/sizeof(frames_hash_s_dspr17[0]) };

static const WmRopeFrame frames_hash_s_dspr21[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS2_01a", "RPDS2_01b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr21 = { "#s_dspr21", frames_hash_s_dspr21, sizeof(frames_hash_s_dspr21)/sizeof(frames_hash_s_dspr21[0]) };

static const WmRopeFrame frames_hash_s_dspr22[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS2_02a", "RPDS2_02b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr22 = { "#s_dspr22", frames_hash_s_dspr22, sizeof(frames_hash_s_dspr22)/sizeof(frames_hash_s_dspr22[0]) };

static const WmRopeFrame frames_hash_s_dspr23[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS2_03a", "RPDS2_03b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr23 = { "#s_dspr23", frames_hash_s_dspr23, sizeof(frames_hash_s_dspr23)/sizeof(frames_hash_s_dspr23[0]) };

static const WmRopeFrame frames_hash_s_dspr24[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS2_04a", "RPDS2_04b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr24 = { "#s_dspr24", frames_hash_s_dspr24, sizeof(frames_hash_s_dspr24)/sizeof(frames_hash_s_dspr24[0]) };

static const WmRopeFrame frames_hash_s_dspr25[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS2_05a", "RPDS2_05b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr25 = { "#s_dspr25", frames_hash_s_dspr25, sizeof(frames_hash_s_dspr25)/sizeof(frames_hash_s_dspr25[0]) };

static const WmRopeFrame frames_hash_s_dspr26[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS2_06a", "RPDS2_06b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr26 = { "#s_dspr26", frames_hash_s_dspr26, sizeof(frames_hash_s_dspr26)/sizeof(frames_hash_s_dspr26[0]) };

static const WmRopeFrame frames_hash_s_dspr27[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS2_07a", "RPDS2_07b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr27 = { "#s_dspr27", frames_hash_s_dspr27, sizeof(frames_hash_s_dspr27)/sizeof(frames_hash_s_dspr27[0]) };

static const WmRopeFrame frames_hash_s_dspr31[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS3_01a", "RPDS3_01b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr31 = { "#s_dspr31", frames_hash_s_dspr31, sizeof(frames_hash_s_dspr31)/sizeof(frames_hash_s_dspr31[0]) };

static const WmRopeFrame frames_hash_s_dspr32[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS3_02a", "RPDS3_02b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr32 = { "#s_dspr32", frames_hash_s_dspr32, sizeof(frames_hash_s_dspr32)/sizeof(frames_hash_s_dspr32[0]) };

static const WmRopeFrame frames_hash_s_dspr33[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS3_03a", "RPDS3_03b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr33 = { "#s_dspr33", frames_hash_s_dspr33, sizeof(frames_hash_s_dspr33)/sizeof(frames_hash_s_dspr33[0]) };

static const WmRopeFrame frames_hash_s_dspr34[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS3_04a", "RPDS3_04b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr34 = { "#s_dspr34", frames_hash_s_dspr34, sizeof(frames_hash_s_dspr34)/sizeof(frames_hash_s_dspr34[0]) };

static const WmRopeFrame frames_hash_s_dspr35[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS3_05a", "RPDS3_05b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr35 = { "#s_dspr35", frames_hash_s_dspr35, sizeof(frames_hash_s_dspr35)/sizeof(frames_hash_s_dspr35[0]) };

static const WmRopeFrame frames_hash_s_dspr36[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS3_06a", "RPDS3_06b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr36 = { "#s_dspr36", frames_hash_s_dspr36, sizeof(frames_hash_s_dspr36)/sizeof(frames_hash_s_dspr36[0]) };

static const WmRopeFrame frames_hash_s_dspr37[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS3_07a", "RPDS3_07b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr37 = { "#s_dspr37", frames_hash_s_dspr37, sizeof(frames_hash_s_dspr37)/sizeof(frames_hash_s_dspr37[0]) };

static const WmRopeFrame frames_hash_s_dspr41[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS4_01a", "RPDS4_01b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr41 = { "#s_dspr41", frames_hash_s_dspr41, sizeof(frames_hash_s_dspr41)/sizeof(frames_hash_s_dspr41[0]) };

static const WmRopeFrame frames_hash_s_dspr42[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS4_02a", "RPDS4_02b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr42 = { "#s_dspr42", frames_hash_s_dspr42, sizeof(frames_hash_s_dspr42)/sizeof(frames_hash_s_dspr42[0]) };

static const WmRopeFrame frames_hash_s_dspr43[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS4_03a", "RPDS4_03b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr43 = { "#s_dspr43", frames_hash_s_dspr43, sizeof(frames_hash_s_dspr43)/sizeof(frames_hash_s_dspr43[0]) };

static const WmRopeFrame frames_hash_s_dspr44[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS4_04a", "RPDS4_04b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr44 = { "#s_dspr44", frames_hash_s_dspr44, sizeof(frames_hash_s_dspr44)/sizeof(frames_hash_s_dspr44[0]) };

static const WmRopeFrame frames_hash_s_dspr45[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS4_05a", "RPDS4_05b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr45 = { "#s_dspr45", frames_hash_s_dspr45, sizeof(frames_hash_s_dspr45)/sizeof(frames_hash_s_dspr45[0]) };

static const WmRopeFrame frames_hash_s_dspr46[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS4_06a", "RPDS4_06b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr46 = { "#s_dspr46", frames_hash_s_dspr46, sizeof(frames_hash_s_dspr46)/sizeof(frames_hash_s_dspr46[0]) };

static const WmRopeFrame frames_hash_s_dspr47[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS4_07a", "RPDS4_07b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr47 = { "#s_dspr47", frames_hash_s_dspr47, sizeof(frames_hash_s_dspr47)/sizeof(frames_hash_s_dspr47[0]) };

static const WmRopeFrame frames_hash_s_dspr51[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS5_01a", "RPDS5_01b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr51 = { "#s_dspr51", frames_hash_s_dspr51, sizeof(frames_hash_s_dspr51)/sizeof(frames_hash_s_dspr51[0]) };

static const WmRopeFrame frames_hash_s_dspr52[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS5_02a", "RPDS5_02b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr52 = { "#s_dspr52", frames_hash_s_dspr52, sizeof(frames_hash_s_dspr52)/sizeof(frames_hash_s_dspr52[0]) };

static const WmRopeFrame frames_hash_s_dspr53[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS5_03a", "RPDS5_03b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr53 = { "#s_dspr53", frames_hash_s_dspr53, sizeof(frames_hash_s_dspr53)/sizeof(frames_hash_s_dspr53[0]) };

static const WmRopeFrame frames_hash_s_dspr54[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS5_04a", "RPDS5_04b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr54 = { "#s_dspr54", frames_hash_s_dspr54, sizeof(frames_hash_s_dspr54)/sizeof(frames_hash_s_dspr54[0]) };

static const WmRopeFrame frames_hash_s_dspr55[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS5_05a", "RPDS5_05b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr55 = { "#s_dspr55", frames_hash_s_dspr55, sizeof(frames_hash_s_dspr55)/sizeof(frames_hash_s_dspr55[0]) };

static const WmRopeFrame frames_hash_s_dspr56[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS5_06a", "RPDS5_06b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr56 = { "#s_dspr56", frames_hash_s_dspr56, sizeof(frames_hash_s_dspr56)/sizeof(frames_hash_s_dspr56[0]) };

static const WmRopeFrame frames_hash_s_dspr57[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPDS5_07a", "RPDS5_07b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr57 = { "#s_dspr57", frames_hash_s_dspr57, sizeof(frames_hash_s_dspr57)/sizeof(frames_hash_s_dspr57[0]) };

static const WmRopeFrame frames_hash_s_dspr_trans_W[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN05a", "RPSBDN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr_trans_W = { "#s_dspr_trans_W", frames_hash_s_dspr_trans_W, sizeof(frames_hash_s_dspr_trans_W)/sizeof(frames_hash_s_dspr_trans_W[0]) };

static const WmRopeFrame frames_hash_s_dspr_trans_R[] = {
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP05a", "RPSBUP05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBUP02a", "RPSBUP02b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN01a", "RPSBDN01b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN05a", "RPSBDN05b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN04a", "RPSBDN04b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN03a", "RPSBDN03b", 0 },
    { WM_ROPE_FRAME_IMAGE, 1u, "RPSBDN02a", "RPSBDN02b", 0 },
};
static const WmRopeSequence seq_hash_s_dspr_trans_R = { "#s_dspr_trans_R", frames_hash_s_dspr_trans_R, sizeof(frames_hash_s_dspr_trans_R)/sizeof(frames_hash_s_dspr_trans_R[0]) };

static const WmRopeScript script_hash_ssprXX;
static const WmRopeScript script_hash_dsprXX;
static const WmRopeScript script_front_bounceud4_R;
static const WmRopeScript script_front_bounceud3_R;
static const WmRopeScript script_front_bounceud2_R;
static const WmRopeScript script_front_bounceud1_R;
static const WmRopeScript script_back_bounceud4_R;
static const WmRopeScript script_back_bounceud3_R;
static const WmRopeScript script_back_bounceud2_R;
static const WmRopeScript script_back_bounceud1_R;
static const WmRopeScript script_side_bounceud4_R;
static const WmRopeScript script_side_bounceud3_R;
static const WmRopeScript script_side_bounceud2_R;
static const WmRopeScript script_side_bounceud1_R;
static const WmRopeScript script_front_bounceud4_W;
static const WmRopeScript script_front_bounceud3_W;
static const WmRopeScript script_front_bounceud2_W;
static const WmRopeScript script_front_bounceud1_W;
static const WmRopeScript script_back_bounceud4_W;
static const WmRopeScript script_back_bounceud3_W;
static const WmRopeScript script_back_bounceud2_W;
static const WmRopeScript script_back_bounceud1_W;
static const WmRopeScript script_side_bounceud4_W;
static const WmRopeScript script_side_bounceud3_W;
static const WmRopeScript script_side_bounceud2_W;
static const WmRopeScript script_side_bounceud1_W;
static const WmRopeScript script_front_bounceud4_B;
static const WmRopeScript script_front_bounceud3_B;
static const WmRopeScript script_front_bounceud2_B;
static const WmRopeScript script_front_bounceud1_B;
static const WmRopeScript script_back_bounceud4_B;
static const WmRopeScript script_back_bounceud3_B;
static const WmRopeScript script_back_bounceud2_B;
static const WmRopeScript script_back_bounceud1_B;
static const WmRopeScript script_side_bounceud4_B;
static const WmRopeScript script_side_bounceud3_B;
static const WmRopeScript script_side_bounceud2_B;
static const WmRopeScript script_side_bounceud1_B;
static const WmRopeScript script_side_bounceud_S;
static const WmRopeScript script_side_bounceio_R;
static const WmRopeScript script_side_bounceio2_R;
static const WmRopeScript script_side_bounceio_W;
static const WmRopeScript script_side_bounceio2_W;
static const WmRopeScript script_side_bounceio_B;
static const WmRopeScript script_side_bounceio_S;
static const WmRopeScript script_hash_sspr11;
static const WmRopeScript script_hash_sspr12;
static const WmRopeScript script_hash_sspr13;
static const WmRopeScript script_hash_sspr14;
static const WmRopeScript script_hash_sspr15;
static const WmRopeScript script_hash_sspr16;
static const WmRopeScript script_hash_sspr21;
static const WmRopeScript script_hash_sspr22;
static const WmRopeScript script_hash_sspr23;
static const WmRopeScript script_hash_sspr24;
static const WmRopeScript script_hash_sspr25;
static const WmRopeScript script_hash_sspr26;
static const WmRopeScript script_hash_sspr31;
static const WmRopeScript script_hash_sspr32;
static const WmRopeScript script_hash_sspr33;
static const WmRopeScript script_hash_sspr34;
static const WmRopeScript script_hash_sspr35;
static const WmRopeScript script_hash_sspr36;
static const WmRopeScript script_hash_sspr41;
static const WmRopeScript script_hash_sspr42;
static const WmRopeScript script_hash_sspr43;
static const WmRopeScript script_hash_sspr44;
static const WmRopeScript script_hash_sspr45;
static const WmRopeScript script_hash_sspr46;
static const WmRopeScript script_hash_sspr51;
static const WmRopeScript script_hash_sspr52;
static const WmRopeScript script_hash_sspr53;
static const WmRopeScript script_hash_sspr54;
static const WmRopeScript script_hash_sspr55;
static const WmRopeScript script_hash_sspr56;
static const WmRopeScript script_hash_sprshad11;
static const WmRopeScript script_hash_sprshad12;
static const WmRopeScript script_hash_sprshad13;
static const WmRopeScript script_hash_sprshad14;
static const WmRopeScript script_hash_sprshad15;
static const WmRopeScript script_hash_sprshad21;
static const WmRopeScript script_hash_sprshad22;
static const WmRopeScript script_hash_sprshad23;
static const WmRopeScript script_hash_sprshad24;
static const WmRopeScript script_hash_sprshad25;
static const WmRopeScript script_hash_sprshad31;
static const WmRopeScript script_hash_sprshad32;
static const WmRopeScript script_hash_sprshad33;
static const WmRopeScript script_hash_sprshad34;
static const WmRopeScript script_hash_sprshad35;
static const WmRopeScript script_hash_sprshad41;
static const WmRopeScript script_hash_sprshad42;
static const WmRopeScript script_hash_sprshad43;
static const WmRopeScript script_hash_sprshad44;
static const WmRopeScript script_hash_sprshad45;
static const WmRopeScript script_hash_sprshad51;
static const WmRopeScript script_hash_sprshad52;
static const WmRopeScript script_hash_sprshad53;
static const WmRopeScript script_hash_sprshad54;
static const WmRopeScript script_hash_sprshad55;
static const WmRopeScript script_hash_sspr_trans_R;
static const WmRopeScript script_hash_sspr_trans_W;
static const WmRopeScript script_hash_sspr_trans_B;
static const WmRopeScript script_hash_sprshad_trans;
static const WmRopeScript script_hash_dsprshad;
static const WmRopeScript script_hash_dspr11;
static const WmRopeScript script_hash_dspr12;
static const WmRopeScript script_hash_dspr13;
static const WmRopeScript script_hash_dspr14;
static const WmRopeScript script_hash_dspr15;
static const WmRopeScript script_hash_dspr16;
static const WmRopeScript script_hash_dspr17;
static const WmRopeScript script_hash_dspr21;
static const WmRopeScript script_hash_dspr22;
static const WmRopeScript script_hash_dspr23;
static const WmRopeScript script_hash_dspr24;
static const WmRopeScript script_hash_dspr25;
static const WmRopeScript script_hash_dspr26;
static const WmRopeScript script_hash_dspr27;
static const WmRopeScript script_hash_dspr31;
static const WmRopeScript script_hash_dspr32;
static const WmRopeScript script_hash_dspr33;
static const WmRopeScript script_hash_dspr34;
static const WmRopeScript script_hash_dspr35;
static const WmRopeScript script_hash_dspr36;
static const WmRopeScript script_hash_dspr37;
static const WmRopeScript script_hash_dspr41;
static const WmRopeScript script_hash_dspr42;
static const WmRopeScript script_hash_dspr43;
static const WmRopeScript script_hash_dspr44;
static const WmRopeScript script_hash_dspr45;
static const WmRopeScript script_hash_dspr46;
static const WmRopeScript script_hash_dspr47;
static const WmRopeScript script_hash_dspr51;
static const WmRopeScript script_hash_dspr52;
static const WmRopeScript script_hash_dspr53;
static const WmRopeScript script_hash_dspr54;
static const WmRopeScript script_hash_dspr55;
static const WmRopeScript script_hash_dspr56;
static const WmRopeScript script_hash_dspr57;
static const WmRopeScript script_hash_dspr_trans_R;
static const WmRopeScript script_hash_dspr_trans_W;
static const WmRopeScript script_hash_dspr_trans_B;

static const WmRopeScriptEntry entries_hash_ssprXX[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_stop, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_ssprXX = { "#ssprXX", entries_hash_ssprXX, sizeof(entries_hash_ssprXX)/sizeof(entries_hash_ssprXX[0]) };

static const WmRopeScriptEntry entries_hash_dsprXX[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_stop, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dsprXX = { "#dsprXX", entries_hash_dsprXX, sizeof(entries_hash_dsprXX)/sizeof(entries_hash_dsprXX[0]) };

static const WmRopeScriptEntry entries_front_bounceud4_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud1_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud4_R = { "front_bounceud4_R", entries_front_bounceud4_R, sizeof(entries_front_bounceud4_R)/sizeof(entries_front_bounceud4_R[0]) };

static const WmRopeScriptEntry entries_front_bounceud3_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud3_R = { "front_bounceud3_R", entries_front_bounceud3_R, sizeof(entries_front_bounceud3_R)/sizeof(entries_front_bounceud3_R[0]) };

static const WmRopeScriptEntry entries_front_bounceud2_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud2_R = { "front_bounceud2_R", entries_front_bounceud2_R, sizeof(entries_front_bounceud2_R)/sizeof(entries_front_bounceud2_R[0]) };

static const WmRopeScriptEntry entries_front_bounceud1_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud1_R = { "front_bounceud1_R", entries_front_bounceud1_R, sizeof(entries_front_bounceud1_R)/sizeof(entries_front_bounceud1_R[0]) };

static const WmRopeScriptEntry entries_back_bounceud4_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud1_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud4_R = { "back_bounceud4_R", entries_back_bounceud4_R, sizeof(entries_back_bounceud4_R)/sizeof(entries_back_bounceud4_R[0]) };

static const WmRopeScriptEntry entries_back_bounceud3_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud3_R = { "back_bounceud3_R", entries_back_bounceud3_R, sizeof(entries_back_bounceud3_R)/sizeof(entries_back_bounceud3_R[0]) };

static const WmRopeScriptEntry entries_back_bounceud2_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud2_R = { "back_bounceud2_R", entries_back_bounceud2_R, sizeof(entries_back_bounceud2_R)/sizeof(entries_back_bounceud2_R[0]) };

static const WmRopeScriptEntry entries_back_bounceud1_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud1_R = { "back_bounceud1_R", entries_back_bounceud1_R, sizeof(entries_back_bounceud1_R)/sizeof(entries_back_bounceud1_R[0]) };

static const WmRopeScriptEntry entries_side_bounceud4_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud1_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud4_R = { "side_bounceud4_R", entries_side_bounceud4_R, sizeof(entries_side_bounceud4_R)/sizeof(entries_side_bounceud4_R[0]) };

static const WmRopeScriptEntry entries_side_bounceud3_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud3_R = { "side_bounceud3_R", entries_side_bounceud3_R, sizeof(entries_side_bounceud3_R)/sizeof(entries_side_bounceud3_R[0]) };

static const WmRopeScriptEntry entries_side_bounceud2_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud2_R = { "side_bounceud2_R", entries_side_bounceud2_R, sizeof(entries_side_bounceud2_R)/sizeof(entries_side_bounceud2_R[0]) };

static const WmRopeScriptEntry entries_side_bounceud1_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_R, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud1_R = { "side_bounceud1_R", entries_side_bounceud1_R, sizeof(entries_side_bounceud1_R)/sizeof(entries_side_bounceud1_R[0]) };

static const WmRopeScriptEntry entries_front_bounceud4_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud1_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud4_W = { "front_bounceud4_W", entries_front_bounceud4_W, sizeof(entries_front_bounceud4_W)/sizeof(entries_front_bounceud4_W[0]) };

static const WmRopeScriptEntry entries_front_bounceud3_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud3_W = { "front_bounceud3_W", entries_front_bounceud3_W, sizeof(entries_front_bounceud3_W)/sizeof(entries_front_bounceud3_W[0]) };

static const WmRopeScriptEntry entries_front_bounceud2_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud2_W = { "front_bounceud2_W", entries_front_bounceud2_W, sizeof(entries_front_bounceud2_W)/sizeof(entries_front_bounceud2_W[0]) };

static const WmRopeScriptEntry entries_front_bounceud1_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud1_W = { "front_bounceud1_W", entries_front_bounceud1_W, sizeof(entries_front_bounceud1_W)/sizeof(entries_front_bounceud1_W[0]) };

static const WmRopeScriptEntry entries_back_bounceud4_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud1_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud4_W = { "back_bounceud4_W", entries_back_bounceud4_W, sizeof(entries_back_bounceud4_W)/sizeof(entries_back_bounceud4_W[0]) };

static const WmRopeScriptEntry entries_back_bounceud3_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud3_W = { "back_bounceud3_W", entries_back_bounceud3_W, sizeof(entries_back_bounceud3_W)/sizeof(entries_back_bounceud3_W[0]) };

static const WmRopeScriptEntry entries_back_bounceud2_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud2_W = { "back_bounceud2_W", entries_back_bounceud2_W, sizeof(entries_back_bounceud2_W)/sizeof(entries_back_bounceud2_W[0]) };

static const WmRopeScriptEntry entries_back_bounceud1_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud1_W = { "back_bounceud1_W", entries_back_bounceud1_W, sizeof(entries_back_bounceud1_W)/sizeof(entries_back_bounceud1_W[0]) };

static const WmRopeScriptEntry entries_side_bounceud4_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud1_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud4_W = { "side_bounceud4_W", entries_side_bounceud4_W, sizeof(entries_side_bounceud4_W)/sizeof(entries_side_bounceud4_W[0]) };

static const WmRopeScriptEntry entries_side_bounceud3_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud3_W = { "side_bounceud3_W", entries_side_bounceud3_W, sizeof(entries_side_bounceud3_W)/sizeof(entries_side_bounceud3_W[0]) };

static const WmRopeScriptEntry entries_side_bounceud2_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud2_W = { "side_bounceud2_W", entries_side_bounceud2_W, sizeof(entries_side_bounceud2_W)/sizeof(entries_side_bounceud2_W[0]) };

static const WmRopeScriptEntry entries_side_bounceud1_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_W, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud1_W = { "side_bounceud1_W", entries_side_bounceud1_W, sizeof(entries_side_bounceud1_W)/sizeof(entries_side_bounceud1_W[0]) };

static const WmRopeScriptEntry entries_front_bounceud4_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud1_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud2_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud4_B = { "front_bounceud4_B", entries_front_bounceud4_B, sizeof(entries_front_bounceud4_B)/sizeof(entries_front_bounceud4_B[0]) };

static const WmRopeScriptEntry entries_front_bounceud3_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_f_bncud2_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud3_B = { "front_bounceud3_B", entries_front_bounceud3_B, sizeof(entries_front_bounceud3_B)/sizeof(entries_front_bounceud3_B[0]) };

static const WmRopeScriptEntry entries_front_bounceud2_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_f_bncud3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud2_B = { "front_bounceud2_B", entries_front_bounceud2_B, sizeof(entries_front_bounceud2_B)/sizeof(entries_front_bounceud2_B[0]) };

static const WmRopeScriptEntry entries_front_bounceud1_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_f_bncud4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_front_bounceud1_B = { "front_bounceud1_B", entries_front_bounceud1_B, sizeof(entries_front_bounceud1_B)/sizeof(entries_front_bounceud1_B[0]) };

static const WmRopeScriptEntry entries_back_bounceud4_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud1_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud2_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud4_B = { "back_bounceud4_B", entries_back_bounceud4_B, sizeof(entries_back_bounceud4_B)/sizeof(entries_back_bounceud4_B[0]) };

static const WmRopeScriptEntry entries_back_bounceud3_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_b_bncud2_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud3_B = { "back_bounceud3_B", entries_back_bounceud3_B, sizeof(entries_back_bounceud3_B)/sizeof(entries_back_bounceud3_B[0]) };

static const WmRopeScriptEntry entries_back_bounceud2_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_b_bncud3_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud2_B = { "back_bounceud2_B", entries_back_bounceud2_B, sizeof(entries_back_bounceud2_B)/sizeof(entries_back_bounceud2_B[0]) };

static const WmRopeScriptEntry entries_back_bounceud1_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_b_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_back_bounceud1_B = { "back_bounceud1_B", entries_back_bounceud1_B, sizeof(entries_back_bounceud1_B)/sizeof(entries_back_bounceud1_B[0]) };

static const WmRopeScriptEntry entries_side_bounceud4_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud1_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud2_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud4_B = { "side_bounceud4_B", entries_side_bounceud4_B, sizeof(entries_side_bounceud4_B)/sizeof(entries_side_bounceud4_B[0]) };

static const WmRopeScriptEntry entries_side_bounceud3_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud2_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud3_B = { "side_bounceud3_B", entries_side_bounceud3_B, sizeof(entries_side_bounceud3_B)/sizeof(entries_side_bounceud3_B[0]) };

static const WmRopeScriptEntry entries_side_bounceud2_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncud3_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud2_B = { "side_bounceud2_B", entries_side_bounceud2_B, sizeof(entries_side_bounceud2_B)/sizeof(entries_side_bounceud2_B[0]) };

static const WmRopeScriptEntry entries_side_bounceud1_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 3u, &seq_hash_s_bncud4_B, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud1_B = { "side_bounceud1_B", entries_side_bounceud1_B, sizeof(entries_side_bounceud1_B)/sizeof(entries_side_bounceud1_B[0]) };

static const WmRopeScriptEntry entries_side_bounceud_S[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncud_S, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceud_S = { "side_bounceud_S", entries_side_bounceud_S, sizeof(entries_side_bounceud_S)/sizeof(entries_side_bounceud_S[0]) };

static const WmRopeScriptEntry entries_side_bounceio_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio1_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncio4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceio_R = { "side_bounceio_R", entries_side_bounceio_R, sizeof(entries_side_bounceio_R)/sizeof(entries_side_bounceio_R[0]) };

static const WmRopeScriptEntry entries_side_bounceio2_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio2_R, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncio4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceio2_R = { "side_bounceio2_R", entries_side_bounceio2_R, sizeof(entries_side_bounceio2_R)/sizeof(entries_side_bounceio2_R[0]) };

static const WmRopeScriptEntry entries_side_bounceio_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio1_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncio4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceio_W = { "side_bounceio_W", entries_side_bounceio_W, sizeof(entries_side_bounceio_W)/sizeof(entries_side_bounceio_W[0]) };

static const WmRopeScriptEntry entries_side_bounceio2_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio2_W, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncio4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceio2_W = { "side_bounceio2_W", entries_side_bounceio2_W, sizeof(entries_side_bounceio2_W)/sizeof(entries_side_bounceio2_W[0]) };

static const WmRopeScriptEntry entries_side_bounceio_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio1_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio2_B, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio3, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncio4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceio_B = { "side_bounceio_B", entries_side_bounceio_B, sizeof(entries_side_bounceio_B)/sizeof(entries_side_bounceio_B[0]) };

static const WmRopeScriptEntry entries_side_bounceio_S[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_bncio_S, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_side_bounceio_S = { "side_bounceio_S", entries_side_bounceio_S, sizeof(entries_side_bounceio_S)/sizeof(entries_side_bounceio_S[0]) };

static const WmRopeScriptEntry entries_hash_sspr11[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr11, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr11 = { "#sspr11", entries_hash_sspr11, sizeof(entries_hash_sspr11)/sizeof(entries_hash_sspr11[0]) };

static const WmRopeScriptEntry entries_hash_sspr12[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr12, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr12 = { "#sspr12", entries_hash_sspr12, sizeof(entries_hash_sspr12)/sizeof(entries_hash_sspr12[0]) };

static const WmRopeScriptEntry entries_hash_sspr13[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr13, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr13 = { "#sspr13", entries_hash_sspr13, sizeof(entries_hash_sspr13)/sizeof(entries_hash_sspr13[0]) };

static const WmRopeScriptEntry entries_hash_sspr14[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr14, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr14 = { "#sspr14", entries_hash_sspr14, sizeof(entries_hash_sspr14)/sizeof(entries_hash_sspr14[0]) };

static const WmRopeScriptEntry entries_hash_sspr15[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr15, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr15 = { "#sspr15", entries_hash_sspr15, sizeof(entries_hash_sspr15)/sizeof(entries_hash_sspr15[0]) };

static const WmRopeScriptEntry entries_hash_sspr16[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr16, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr16 = { "#sspr16", entries_hash_sspr16, sizeof(entries_hash_sspr16)/sizeof(entries_hash_sspr16[0]) };

static const WmRopeScriptEntry entries_hash_sspr21[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr21, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr21 = { "#sspr21", entries_hash_sspr21, sizeof(entries_hash_sspr21)/sizeof(entries_hash_sspr21[0]) };

static const WmRopeScriptEntry entries_hash_sspr22[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr22, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr22 = { "#sspr22", entries_hash_sspr22, sizeof(entries_hash_sspr22)/sizeof(entries_hash_sspr22[0]) };

static const WmRopeScriptEntry entries_hash_sspr23[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr23, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr23 = { "#sspr23", entries_hash_sspr23, sizeof(entries_hash_sspr23)/sizeof(entries_hash_sspr23[0]) };

static const WmRopeScriptEntry entries_hash_sspr24[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr24, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr24 = { "#sspr24", entries_hash_sspr24, sizeof(entries_hash_sspr24)/sizeof(entries_hash_sspr24[0]) };

static const WmRopeScriptEntry entries_hash_sspr25[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr25, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr25 = { "#sspr25", entries_hash_sspr25, sizeof(entries_hash_sspr25)/sizeof(entries_hash_sspr25[0]) };

static const WmRopeScriptEntry entries_hash_sspr26[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr26, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr26 = { "#sspr26", entries_hash_sspr26, sizeof(entries_hash_sspr26)/sizeof(entries_hash_sspr26[0]) };

static const WmRopeScriptEntry entries_hash_sspr31[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr31, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr31 = { "#sspr31", entries_hash_sspr31, sizeof(entries_hash_sspr31)/sizeof(entries_hash_sspr31[0]) };

static const WmRopeScriptEntry entries_hash_sspr32[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr32, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr32 = { "#sspr32", entries_hash_sspr32, sizeof(entries_hash_sspr32)/sizeof(entries_hash_sspr32[0]) };

static const WmRopeScriptEntry entries_hash_sspr33[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr33, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr33 = { "#sspr33", entries_hash_sspr33, sizeof(entries_hash_sspr33)/sizeof(entries_hash_sspr33[0]) };

static const WmRopeScriptEntry entries_hash_sspr34[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr34, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr34 = { "#sspr34", entries_hash_sspr34, sizeof(entries_hash_sspr34)/sizeof(entries_hash_sspr34[0]) };

static const WmRopeScriptEntry entries_hash_sspr35[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr35, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr35 = { "#sspr35", entries_hash_sspr35, sizeof(entries_hash_sspr35)/sizeof(entries_hash_sspr35[0]) };

static const WmRopeScriptEntry entries_hash_sspr36[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr36, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr36 = { "#sspr36", entries_hash_sspr36, sizeof(entries_hash_sspr36)/sizeof(entries_hash_sspr36[0]) };

static const WmRopeScriptEntry entries_hash_sspr41[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr41, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr41 = { "#sspr41", entries_hash_sspr41, sizeof(entries_hash_sspr41)/sizeof(entries_hash_sspr41[0]) };

static const WmRopeScriptEntry entries_hash_sspr42[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr42, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr42 = { "#sspr42", entries_hash_sspr42, sizeof(entries_hash_sspr42)/sizeof(entries_hash_sspr42[0]) };

static const WmRopeScriptEntry entries_hash_sspr43[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr43, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr43 = { "#sspr43", entries_hash_sspr43, sizeof(entries_hash_sspr43)/sizeof(entries_hash_sspr43[0]) };

static const WmRopeScriptEntry entries_hash_sspr44[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr44, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr44 = { "#sspr44", entries_hash_sspr44, sizeof(entries_hash_sspr44)/sizeof(entries_hash_sspr44[0]) };

static const WmRopeScriptEntry entries_hash_sspr45[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr45, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr45 = { "#sspr45", entries_hash_sspr45, sizeof(entries_hash_sspr45)/sizeof(entries_hash_sspr45[0]) };

static const WmRopeScriptEntry entries_hash_sspr46[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr46, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr46 = { "#sspr46", entries_hash_sspr46, sizeof(entries_hash_sspr46)/sizeof(entries_hash_sspr46[0]) };

static const WmRopeScriptEntry entries_hash_sspr51[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr51, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr51 = { "#sspr51", entries_hash_sspr51, sizeof(entries_hash_sspr51)/sizeof(entries_hash_sspr51[0]) };

static const WmRopeScriptEntry entries_hash_sspr52[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr52, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr52 = { "#sspr52", entries_hash_sspr52, sizeof(entries_hash_sspr52)/sizeof(entries_hash_sspr52[0]) };

static const WmRopeScriptEntry entries_hash_sspr53[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr53, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr53 = { "#sspr53", entries_hash_sspr53, sizeof(entries_hash_sspr53)/sizeof(entries_hash_sspr53[0]) };

static const WmRopeScriptEntry entries_hash_sspr54[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr54, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr54 = { "#sspr54", entries_hash_sspr54, sizeof(entries_hash_sspr54)/sizeof(entries_hash_sspr54[0]) };

static const WmRopeScriptEntry entries_hash_sspr55[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr55, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr55 = { "#sspr55", entries_hash_sspr55, sizeof(entries_hash_sspr55)/sizeof(entries_hash_sspr55[0]) };

static const WmRopeScriptEntry entries_hash_sspr56[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr56, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr56 = { "#sspr56", entries_hash_sspr56, sizeof(entries_hash_sspr56)/sizeof(entries_hash_sspr56[0]) };

static const WmRopeScriptEntry entries_hash_sprshad11[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad11, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad11 = { "#sprshad11", entries_hash_sprshad11, sizeof(entries_hash_sprshad11)/sizeof(entries_hash_sprshad11[0]) };

static const WmRopeScriptEntry entries_hash_sprshad12[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad12, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad12 = { "#sprshad12", entries_hash_sprshad12, sizeof(entries_hash_sprshad12)/sizeof(entries_hash_sprshad12[0]) };

static const WmRopeScriptEntry entries_hash_sprshad13[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad13, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad13 = { "#sprshad13", entries_hash_sprshad13, sizeof(entries_hash_sprshad13)/sizeof(entries_hash_sprshad13[0]) };

static const WmRopeScriptEntry entries_hash_sprshad14[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad14, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad14 = { "#sprshad14", entries_hash_sprshad14, sizeof(entries_hash_sprshad14)/sizeof(entries_hash_sprshad14[0]) };

static const WmRopeScriptEntry entries_hash_sprshad15[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad15, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad15 = { "#sprshad15", entries_hash_sprshad15, sizeof(entries_hash_sprshad15)/sizeof(entries_hash_sprshad15[0]) };

static const WmRopeScriptEntry entries_hash_sprshad21[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad21, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad21 = { "#sprshad21", entries_hash_sprshad21, sizeof(entries_hash_sprshad21)/sizeof(entries_hash_sprshad21[0]) };

static const WmRopeScriptEntry entries_hash_sprshad22[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad22, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad22 = { "#sprshad22", entries_hash_sprshad22, sizeof(entries_hash_sprshad22)/sizeof(entries_hash_sprshad22[0]) };

static const WmRopeScriptEntry entries_hash_sprshad23[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad23, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad23 = { "#sprshad23", entries_hash_sprshad23, sizeof(entries_hash_sprshad23)/sizeof(entries_hash_sprshad23[0]) };

static const WmRopeScriptEntry entries_hash_sprshad24[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad24, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad24 = { "#sprshad24", entries_hash_sprshad24, sizeof(entries_hash_sprshad24)/sizeof(entries_hash_sprshad24[0]) };

static const WmRopeScriptEntry entries_hash_sprshad25[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad25, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad25 = { "#sprshad25", entries_hash_sprshad25, sizeof(entries_hash_sprshad25)/sizeof(entries_hash_sprshad25[0]) };

static const WmRopeScriptEntry entries_hash_sprshad31[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad31, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad31 = { "#sprshad31", entries_hash_sprshad31, sizeof(entries_hash_sprshad31)/sizeof(entries_hash_sprshad31[0]) };

static const WmRopeScriptEntry entries_hash_sprshad32[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad32, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad32 = { "#sprshad32", entries_hash_sprshad32, sizeof(entries_hash_sprshad32)/sizeof(entries_hash_sprshad32[0]) };

static const WmRopeScriptEntry entries_hash_sprshad33[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad33, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad33 = { "#sprshad33", entries_hash_sprshad33, sizeof(entries_hash_sprshad33)/sizeof(entries_hash_sprshad33[0]) };

static const WmRopeScriptEntry entries_hash_sprshad34[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad34, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad34 = { "#sprshad34", entries_hash_sprshad34, sizeof(entries_hash_sprshad34)/sizeof(entries_hash_sprshad34[0]) };

static const WmRopeScriptEntry entries_hash_sprshad35[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad35, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad35 = { "#sprshad35", entries_hash_sprshad35, sizeof(entries_hash_sprshad35)/sizeof(entries_hash_sprshad35[0]) };

static const WmRopeScriptEntry entries_hash_sprshad41[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad41, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad41 = { "#sprshad41", entries_hash_sprshad41, sizeof(entries_hash_sprshad41)/sizeof(entries_hash_sprshad41[0]) };

static const WmRopeScriptEntry entries_hash_sprshad42[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad42, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad42 = { "#sprshad42", entries_hash_sprshad42, sizeof(entries_hash_sprshad42)/sizeof(entries_hash_sprshad42[0]) };

static const WmRopeScriptEntry entries_hash_sprshad43[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad43, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad43 = { "#sprshad43", entries_hash_sprshad43, sizeof(entries_hash_sprshad43)/sizeof(entries_hash_sprshad43[0]) };

static const WmRopeScriptEntry entries_hash_sprshad44[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad44, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad44 = { "#sprshad44", entries_hash_sprshad44, sizeof(entries_hash_sprshad44)/sizeof(entries_hash_sprshad44[0]) };

static const WmRopeScriptEntry entries_hash_sprshad45[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad45, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad45 = { "#sprshad45", entries_hash_sprshad45, sizeof(entries_hash_sprshad45)/sizeof(entries_hash_sprshad45[0]) };

static const WmRopeScriptEntry entries_hash_sprshad51[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad51, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad51 = { "#sprshad51", entries_hash_sprshad51, sizeof(entries_hash_sprshad51)/sizeof(entries_hash_sprshad51[0]) };

static const WmRopeScriptEntry entries_hash_sprshad52[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad52, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad52 = { "#sprshad52", entries_hash_sprshad52, sizeof(entries_hash_sprshad52)/sizeof(entries_hash_sprshad52[0]) };

static const WmRopeScriptEntry entries_hash_sprshad53[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad53, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad53 = { "#sprshad53", entries_hash_sprshad53, sizeof(entries_hash_sprshad53)/sizeof(entries_hash_sprshad53[0]) };

static const WmRopeScriptEntry entries_hash_sprshad54[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad54, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad54 = { "#sprshad54", entries_hash_sprshad54, sizeof(entries_hash_sprshad54)/sizeof(entries_hash_sprshad54[0]) };

static const WmRopeScriptEntry entries_hash_sprshad55[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad55, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad55 = { "#sprshad55", entries_hash_sprshad55, sizeof(entries_hash_sprshad55)/sizeof(entries_hash_sprshad55[0]) };

static const WmRopeScriptEntry entries_hash_sspr_trans_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr_trans_R, 0u, 0 },
    { WM_ROPE_SCRIPT_GOTO, 0u, 0, 0u, &script_side_bounceio2_R },
};
static const WmRopeScript script_hash_sspr_trans_R = { "#sspr_trans_R", entries_hash_sspr_trans_R, sizeof(entries_hash_sspr_trans_R)/sizeof(entries_hash_sspr_trans_R[0]) };

static const WmRopeScriptEntry entries_hash_sspr_trans_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sspr_trans_W, 0u, 0 },
    { WM_ROPE_SCRIPT_GOTO, 0u, 0, 0u, &script_side_bounceio2_W },
};
static const WmRopeScript script_hash_sspr_trans_W = { "#sspr_trans_W", entries_hash_sspr_trans_W, sizeof(entries_hash_sspr_trans_W)/sizeof(entries_hash_sspr_trans_W[0]) };

static const WmRopeScriptEntry entries_hash_sspr_trans_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncio4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sspr_trans_B = { "#sspr_trans_B", entries_hash_sspr_trans_B, sizeof(entries_hash_sspr_trans_B)/sizeof(entries_hash_sspr_trans_B[0]) };

static const WmRopeScriptEntry entries_hash_sprshad_trans[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_sprshad_trans, 0u, 0 },
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_stop_shadow, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_sprshad_trans = { "#sprshad_trans", entries_hash_sprshad_trans, sizeof(entries_hash_sprshad_trans)/sizeof(entries_hash_sprshad_trans[0]) };

static const WmRopeScriptEntry entries_hash_dsprshad[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dsprshad, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dsprshad = { "#dsprshad", entries_hash_dsprshad, sizeof(entries_hash_dsprshad)/sizeof(entries_hash_dsprshad[0]) };

static const WmRopeScriptEntry entries_hash_dspr11[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr11, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr11 = { "#dspr11", entries_hash_dspr11, sizeof(entries_hash_dspr11)/sizeof(entries_hash_dspr11[0]) };

static const WmRopeScriptEntry entries_hash_dspr12[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr12, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr12 = { "#dspr12", entries_hash_dspr12, sizeof(entries_hash_dspr12)/sizeof(entries_hash_dspr12[0]) };

static const WmRopeScriptEntry entries_hash_dspr13[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr13, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr13 = { "#dspr13", entries_hash_dspr13, sizeof(entries_hash_dspr13)/sizeof(entries_hash_dspr13[0]) };

static const WmRopeScriptEntry entries_hash_dspr14[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr14, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr14 = { "#dspr14", entries_hash_dspr14, sizeof(entries_hash_dspr14)/sizeof(entries_hash_dspr14[0]) };

static const WmRopeScriptEntry entries_hash_dspr15[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr15, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr15 = { "#dspr15", entries_hash_dspr15, sizeof(entries_hash_dspr15)/sizeof(entries_hash_dspr15[0]) };

static const WmRopeScriptEntry entries_hash_dspr16[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr16, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr16 = { "#dspr16", entries_hash_dspr16, sizeof(entries_hash_dspr16)/sizeof(entries_hash_dspr16[0]) };

static const WmRopeScriptEntry entries_hash_dspr17[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr17, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr17 = { "#dspr17", entries_hash_dspr17, sizeof(entries_hash_dspr17)/sizeof(entries_hash_dspr17[0]) };

static const WmRopeScriptEntry entries_hash_dspr21[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr21, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr21 = { "#dspr21", entries_hash_dspr21, sizeof(entries_hash_dspr21)/sizeof(entries_hash_dspr21[0]) };

static const WmRopeScriptEntry entries_hash_dspr22[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr22, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr22 = { "#dspr22", entries_hash_dspr22, sizeof(entries_hash_dspr22)/sizeof(entries_hash_dspr22[0]) };

static const WmRopeScriptEntry entries_hash_dspr23[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr23, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr23 = { "#dspr23", entries_hash_dspr23, sizeof(entries_hash_dspr23)/sizeof(entries_hash_dspr23[0]) };

static const WmRopeScriptEntry entries_hash_dspr24[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr24, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr24 = { "#dspr24", entries_hash_dspr24, sizeof(entries_hash_dspr24)/sizeof(entries_hash_dspr24[0]) };

static const WmRopeScriptEntry entries_hash_dspr25[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr25, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr25 = { "#dspr25", entries_hash_dspr25, sizeof(entries_hash_dspr25)/sizeof(entries_hash_dspr25[0]) };

static const WmRopeScriptEntry entries_hash_dspr26[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr26, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr26 = { "#dspr26", entries_hash_dspr26, sizeof(entries_hash_dspr26)/sizeof(entries_hash_dspr26[0]) };

static const WmRopeScriptEntry entries_hash_dspr27[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr27, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr27 = { "#dspr27", entries_hash_dspr27, sizeof(entries_hash_dspr27)/sizeof(entries_hash_dspr27[0]) };

static const WmRopeScriptEntry entries_hash_dspr31[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr31, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr31 = { "#dspr31", entries_hash_dspr31, sizeof(entries_hash_dspr31)/sizeof(entries_hash_dspr31[0]) };

static const WmRopeScriptEntry entries_hash_dspr32[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr32, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr32 = { "#dspr32", entries_hash_dspr32, sizeof(entries_hash_dspr32)/sizeof(entries_hash_dspr32[0]) };

static const WmRopeScriptEntry entries_hash_dspr33[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr33, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr33 = { "#dspr33", entries_hash_dspr33, sizeof(entries_hash_dspr33)/sizeof(entries_hash_dspr33[0]) };

static const WmRopeScriptEntry entries_hash_dspr34[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr34, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr34 = { "#dspr34", entries_hash_dspr34, sizeof(entries_hash_dspr34)/sizeof(entries_hash_dspr34[0]) };

static const WmRopeScriptEntry entries_hash_dspr35[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr35, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr35 = { "#dspr35", entries_hash_dspr35, sizeof(entries_hash_dspr35)/sizeof(entries_hash_dspr35[0]) };

static const WmRopeScriptEntry entries_hash_dspr36[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr36, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr36 = { "#dspr36", entries_hash_dspr36, sizeof(entries_hash_dspr36)/sizeof(entries_hash_dspr36[0]) };

static const WmRopeScriptEntry entries_hash_dspr37[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr37, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr37 = { "#dspr37", entries_hash_dspr37, sizeof(entries_hash_dspr37)/sizeof(entries_hash_dspr37[0]) };

static const WmRopeScriptEntry entries_hash_dspr41[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr41, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr41 = { "#dspr41", entries_hash_dspr41, sizeof(entries_hash_dspr41)/sizeof(entries_hash_dspr41[0]) };

static const WmRopeScriptEntry entries_hash_dspr42[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr42, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr42 = { "#dspr42", entries_hash_dspr42, sizeof(entries_hash_dspr42)/sizeof(entries_hash_dspr42[0]) };

static const WmRopeScriptEntry entries_hash_dspr43[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr43, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr43 = { "#dspr43", entries_hash_dspr43, sizeof(entries_hash_dspr43)/sizeof(entries_hash_dspr43[0]) };

static const WmRopeScriptEntry entries_hash_dspr44[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr44, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr44 = { "#dspr44", entries_hash_dspr44, sizeof(entries_hash_dspr44)/sizeof(entries_hash_dspr44[0]) };

static const WmRopeScriptEntry entries_hash_dspr45[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr45, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr45 = { "#dspr45", entries_hash_dspr45, sizeof(entries_hash_dspr45)/sizeof(entries_hash_dspr45[0]) };

static const WmRopeScriptEntry entries_hash_dspr46[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr46, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr46 = { "#dspr46", entries_hash_dspr46, sizeof(entries_hash_dspr46)/sizeof(entries_hash_dspr46[0]) };

static const WmRopeScriptEntry entries_hash_dspr47[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr47, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr47 = { "#dspr47", entries_hash_dspr47, sizeof(entries_hash_dspr47)/sizeof(entries_hash_dspr47[0]) };

static const WmRopeScriptEntry entries_hash_dspr51[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr51, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr51 = { "#dspr51", entries_hash_dspr51, sizeof(entries_hash_dspr51)/sizeof(entries_hash_dspr51[0]) };

static const WmRopeScriptEntry entries_hash_dspr52[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr52, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr52 = { "#dspr52", entries_hash_dspr52, sizeof(entries_hash_dspr52)/sizeof(entries_hash_dspr52[0]) };

static const WmRopeScriptEntry entries_hash_dspr53[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr53, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr53 = { "#dspr53", entries_hash_dspr53, sizeof(entries_hash_dspr53)/sizeof(entries_hash_dspr53[0]) };

static const WmRopeScriptEntry entries_hash_dspr54[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr54, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr54 = { "#dspr54", entries_hash_dspr54, sizeof(entries_hash_dspr54)/sizeof(entries_hash_dspr54[0]) };

static const WmRopeScriptEntry entries_hash_dspr55[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr55, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr55 = { "#dspr55", entries_hash_dspr55, sizeof(entries_hash_dspr55)/sizeof(entries_hash_dspr55[0]) };

static const WmRopeScriptEntry entries_hash_dspr56[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr56, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr56 = { "#dspr56", entries_hash_dspr56, sizeof(entries_hash_dspr56)/sizeof(entries_hash_dspr56[0]) };

static const WmRopeScriptEntry entries_hash_dspr57[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr57, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr57 = { "#dspr57", entries_hash_dspr57, sizeof(entries_hash_dspr57)/sizeof(entries_hash_dspr57[0]) };

static const WmRopeScriptEntry entries_hash_dspr_trans_R[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr_trans_R, 0u, 0 },
    { WM_ROPE_SCRIPT_GOTO, 0u, 0, 0u, &script_side_bounceud2_R },
};
static const WmRopeScript script_hash_dspr_trans_R = { "#dspr_trans_R", entries_hash_dspr_trans_R, sizeof(entries_hash_dspr_trans_R)/sizeof(entries_hash_dspr_trans_R[0]) };

static const WmRopeScriptEntry entries_hash_dspr_trans_W[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq_hash_s_dspr_trans_W, 0u, 0 },
    { WM_ROPE_SCRIPT_GOTO, 0u, 0, 0u, &script_side_bounceud2_W },
};
static const WmRopeScript script_hash_dspr_trans_W = { "#dspr_trans_W", entries_hash_dspr_trans_W, sizeof(entries_hash_dspr_trans_W)/sizeof(entries_hash_dspr_trans_W[0]) };

static const WmRopeScriptEntry entries_hash_dspr_trans_B[] = {
    { WM_ROPE_SCRIPT_SEQUENCE, 2u, &seq_hash_s_bncio4, 0u, 0 },
    { WM_ROPE_SCRIPT_END, 0u, 0, 0u, 0 },
};
static const WmRopeScript script_hash_dspr_trans_B = { "#dspr_trans_B", entries_hash_dspr_trans_B, sizeof(entries_hash_dspr_trans_B)/sizeof(entries_hash_dspr_trans_B[0]) };

static const WmRopeCommandProgram program_front_bounceud4_t = {
    "front_bounceud4_t", 3u,
    {
        { 5u, &script_front_bounceud4_R },
        { 5u, &script_front_bounceud4_W },
        { 5u, &script_front_bounceud4_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_back_bounceud4_t = {
    "back_bounceud4_t", 3u,
    {
        { 5u, &script_back_bounceud4_R },
        { 5u, &script_back_bounceud4_W },
        { 5u, &script_back_bounceud4_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_side_bounceud4_t = {
    "side_bounceud4_t", 4u,
    {
        { 5u, &script_side_bounceud4_R },
        { 5u, &script_side_bounceud4_W },
        { 5u, &script_side_bounceud4_B },
        { 5u, &script_side_bounceud_S },
    }
};

static const WmRopeCommandProgram program_front_bounceud3_t = {
    "front_bounceud3_t", 3u,
    {
        { 5u, &script_front_bounceud3_R },
        { 5u, &script_front_bounceud3_W },
        { 5u, &script_front_bounceud3_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_back_bounceud3_t = {
    "back_bounceud3_t", 3u,
    {
        { 5u, &script_back_bounceud3_R },
        { 5u, &script_back_bounceud3_W },
        { 5u, &script_back_bounceud3_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_side_bounceud3_t = {
    "side_bounceud3_t", 4u,
    {
        { 5u, &script_side_bounceud3_R },
        { 5u, &script_side_bounceud3_W },
        { 5u, &script_side_bounceud3_B },
        { 5u, &script_side_bounceud_S },
    }
};

static const WmRopeCommandProgram program_front_bounceud2_t = {
    "front_bounceud2_t", 3u,
    {
        { 5u, &script_front_bounceud2_R },
        { 5u, &script_front_bounceud2_W },
        { 5u, &script_front_bounceud2_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_back_bounceud2_t = {
    "back_bounceud2_t", 3u,
    {
        { 5u, &script_back_bounceud2_R },
        { 5u, &script_back_bounceud2_W },
        { 5u, &script_back_bounceud2_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_side_bounceud2_t = {
    "side_bounceud2_t", 4u,
    {
        { 5u, &script_side_bounceud2_R },
        { 5u, &script_side_bounceud2_W },
        { 5u, &script_side_bounceud2_B },
        { 5u, &script_side_bounceud_S },
    }
};

static const WmRopeCommandProgram program_front_bounceud1_t = {
    "front_bounceud1_t", 3u,
    {
        { 5u, &script_front_bounceud1_R },
        { 5u, &script_front_bounceud1_W },
        { 5u, &script_front_bounceud1_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_back_bounceud1_t = {
    "back_bounceud1_t", 3u,
    {
        { 5u, &script_back_bounceud1_R },
        { 5u, &script_back_bounceud1_W },
        { 5u, &script_back_bounceud1_B },
        { 0u, 0 },
    }
};

static const WmRopeCommandProgram program_side_bounceud1_t = {
    "side_bounceud1_t", 4u,
    {
        { 5u, &script_side_bounceud1_R },
        { 5u, &script_side_bounceud1_W },
        { 5u, &script_side_bounceud1_B },
        { 5u, &script_side_bounceud_S },
    }
};

static const WmRopeCommandProgram program_side_bounceio_t = {
    "side_bounceio_t", 4u,
    {
        { 5u, &script_side_bounceio_R },
        { 5u, &script_side_bounceio_W },
        { 5u, &script_side_bounceio_B },
        { 5u, &script_side_bounceio_S },
    }
};

static const WmRopeCommandProgram program_sspr11_t = {
    "sspr11_t", 4u,
    {
        { 10u, &script_hash_sspr11 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad11 },
    }
};

static const WmRopeCommandProgram program_sspr12_t = {
    "sspr12_t", 4u,
    {
        { 10u, &script_hash_sspr12 },
        { 10u, &script_hash_sspr11 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad12 },
    }
};

static const WmRopeCommandProgram program_sspr13_t = {
    "sspr13_t", 4u,
    {
        { 10u, &script_hash_sspr13 },
        { 10u, &script_hash_sspr12 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad13 },
    }
};

static const WmRopeCommandProgram program_sspr14_t = {
    "sspr14_t", 4u,
    {
        { 10u, &script_hash_sspr14 },
        { 10u, &script_hash_sspr12 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad13 },
    }
};

static const WmRopeCommandProgram program_sspr15_t = {
    "sspr15_t", 4u,
    {
        { 10u, &script_hash_sspr15 },
        { 10u, &script_hash_sspr11 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad12 },
    }
};

static const WmRopeCommandProgram program_sspr21_t = {
    "sspr21_t", 4u,
    {
        { 10u, &script_hash_sspr21 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad21 },
    }
};

static const WmRopeCommandProgram program_sspr22_t = {
    "sspr22_t", 4u,
    {
        { 10u, &script_hash_sspr22 },
        { 10u, &script_hash_sspr21 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad22 },
    }
};

static const WmRopeCommandProgram program_sspr23_t = {
    "sspr23_t", 4u,
    {
        { 10u, &script_hash_sspr23 },
        { 10u, &script_hash_sspr22 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad23 },
    }
};

static const WmRopeCommandProgram program_sspr24_t = {
    "sspr24_t", 4u,
    {
        { 10u, &script_hash_sspr24 },
        { 10u, &script_hash_sspr22 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad23 },
    }
};

static const WmRopeCommandProgram program_sspr25_t = {
    "sspr25_t", 4u,
    {
        { 10u, &script_hash_sspr25 },
        { 10u, &script_hash_sspr21 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad22 },
    }
};

static const WmRopeCommandProgram program_sspr31_t = {
    "sspr31_t", 4u,
    {
        { 10u, &script_hash_sspr31 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad31 },
    }
};

static const WmRopeCommandProgram program_sspr32_t = {
    "sspr32_t", 4u,
    {
        { 10u, &script_hash_sspr32 },
        { 10u, &script_hash_sspr31 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad32 },
    }
};

static const WmRopeCommandProgram program_sspr33_t = {
    "sspr33_t", 4u,
    {
        { 10u, &script_hash_sspr33 },
        { 10u, &script_hash_sspr32 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad33 },
    }
};

static const WmRopeCommandProgram program_sspr34_t = {
    "sspr34_t", 4u,
    {
        { 10u, &script_hash_sspr34 },
        { 10u, &script_hash_sspr32 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad33 },
    }
};

static const WmRopeCommandProgram program_sspr35_t = {
    "sspr35_t", 4u,
    {
        { 10u, &script_hash_sspr35 },
        { 10u, &script_hash_sspr31 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad32 },
    }
};

static const WmRopeCommandProgram program_sspr41_t = {
    "sspr41_t", 4u,
    {
        { 10u, &script_hash_sspr41 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad41 },
    }
};

static const WmRopeCommandProgram program_sspr42_t = {
    "sspr42_t", 4u,
    {
        { 10u, &script_hash_sspr42 },
        { 10u, &script_hash_sspr41 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad42 },
    }
};

static const WmRopeCommandProgram program_sspr43_t = {
    "sspr43_t", 4u,
    {
        { 10u, &script_hash_sspr43 },
        { 10u, &script_hash_sspr42 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad43 },
    }
};

static const WmRopeCommandProgram program_sspr44_t = {
    "sspr44_t", 4u,
    {
        { 10u, &script_hash_sspr44 },
        { 10u, &script_hash_sspr42 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad43 },
    }
};

static const WmRopeCommandProgram program_sspr45_t = {
    "sspr45_t", 4u,
    {
        { 10u, &script_hash_sspr45 },
        { 10u, &script_hash_sspr41 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad42 },
    }
};

static const WmRopeCommandProgram program_sspr51_t = {
    "sspr51_t", 4u,
    {
        { 10u, &script_hash_sspr51 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad51 },
    }
};

static const WmRopeCommandProgram program_sspr52_t = {
    "sspr52_t", 4u,
    {
        { 10u, &script_hash_sspr52 },
        { 10u, &script_hash_sspr51 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad52 },
    }
};

static const WmRopeCommandProgram program_sspr53_t = {
    "sspr53_t", 4u,
    {
        { 10u, &script_hash_sspr53 },
        { 10u, &script_hash_sspr52 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad53 },
    }
};

static const WmRopeCommandProgram program_sspr54_t = {
    "sspr54_t", 4u,
    {
        { 10u, &script_hash_sspr54 },
        { 10u, &script_hash_sspr52 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad53 },
    }
};

static const WmRopeCommandProgram program_sspr55_t = {
    "sspr55_t", 4u,
    {
        { 10u, &script_hash_sspr55 },
        { 10u, &script_hash_sspr51 },
        { 10u, &script_hash_ssprXX },
        { 10u, &script_hash_sprshad52 },
    }
};

static const WmRopeCommandProgram program_sspr_trans_t = {
    "sspr_trans_t", 4u,
    {
        { 10u, &script_hash_sspr_trans_R },
        { 10u, &script_hash_sspr_trans_W },
        { 10u, &script_hash_sspr_trans_B },
        { 10u, &script_hash_sprshad_trans },
    }
};

static const WmRopeCommandProgram program_dspr11_t = {
    "dspr11_t", 4u,
    {
        { 9u, &script_hash_dspr15 },
        { 9u, &script_hash_dspr11 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr12_t = {
    "dspr12_t", 4u,
    {
        { 9u, &script_hash_dspr16 },
        { 9u, &script_hash_dspr12 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr13_t = {
    "dspr13_t", 4u,
    {
        { 9u, &script_hash_dspr17 },
        { 9u, &script_hash_dspr13 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr14_t = {
    "dspr14_t", 4u,
    {
        { 9u, &script_hash_dspr17 },
        { 9u, &script_hash_dspr14 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr15_t = {
    "dspr15_t", 4u,
    {
        { 9u, &script_hash_dspr16 },
        { 9u, &script_hash_dspr15 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr16_t = {
    "dspr16_t", 4u,
    {
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dspr15 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr21_t = {
    "dspr21_t", 4u,
    {
        { 9u, &script_hash_dspr25 },
        { 9u, &script_hash_dspr21 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr22_t = {
    "dspr22_t", 4u,
    {
        { 9u, &script_hash_dspr26 },
        { 9u, &script_hash_dspr22 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr23_t = {
    "dspr23_t", 4u,
    {
        { 9u, &script_hash_dspr27 },
        { 9u, &script_hash_dspr23 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr24_t = {
    "dspr24_t", 4u,
    {
        { 9u, &script_hash_dspr27 },
        { 9u, &script_hash_dspr24 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr25_t = {
    "dspr25_t", 4u,
    {
        { 9u, &script_hash_dspr26 },
        { 9u, &script_hash_dspr25 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr26_t = {
    "dspr26_t", 4u,
    {
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dspr25 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr31_t = {
    "dspr31_t", 4u,
    {
        { 9u, &script_hash_dspr35 },
        { 9u, &script_hash_dspr31 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr32_t = {
    "dspr32_t", 4u,
    {
        { 9u, &script_hash_dspr36 },
        { 9u, &script_hash_dspr32 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr33_t = {
    "dspr33_t", 4u,
    {
        { 9u, &script_hash_dspr37 },
        { 9u, &script_hash_dspr33 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr34_t = {
    "dspr34_t", 4u,
    {
        { 9u, &script_hash_dspr37 },
        { 9u, &script_hash_dspr34 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr35_t = {
    "dspr35_t", 4u,
    {
        { 9u, &script_hash_dspr36 },
        { 9u, &script_hash_dspr35 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr36_t = {
    "dspr36_t", 4u,
    {
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dspr35 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr41_t = {
    "dspr41_t", 4u,
    {
        { 9u, &script_hash_dspr45 },
        { 9u, &script_hash_dspr41 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr42_t = {
    "dspr42_t", 4u,
    {
        { 9u, &script_hash_dspr46 },
        { 9u, &script_hash_dspr42 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr43_t = {
    "dspr43_t", 4u,
    {
        { 9u, &script_hash_dspr47 },
        { 9u, &script_hash_dspr43 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr44_t = {
    "dspr44_t", 4u,
    {
        { 9u, &script_hash_dspr47 },
        { 9u, &script_hash_dspr44 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr45_t = {
    "dspr45_t", 4u,
    {
        { 9u, &script_hash_dspr46 },
        { 9u, &script_hash_dspr45 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr46_t = {
    "dspr46_t", 4u,
    {
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dspr45 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr51_t = {
    "dspr51_t", 4u,
    {
        { 9u, &script_hash_dspr55 },
        { 9u, &script_hash_dspr51 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr52_t = {
    "dspr52_t", 4u,
    {
        { 9u, &script_hash_dspr56 },
        { 9u, &script_hash_dspr52 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr53_t = {
    "dspr53_t", 4u,
    {
        { 9u, &script_hash_dspr57 },
        { 9u, &script_hash_dspr53 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr54_t = {
    "dspr54_t", 4u,
    {
        { 9u, &script_hash_dspr57 },
        { 9u, &script_hash_dspr54 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr55_t = {
    "dspr55_t", 4u,
    {
        { 9u, &script_hash_dspr56 },
        { 9u, &script_hash_dspr55 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr56_t = {
    "dspr56_t", 4u,
    {
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dspr55 },
        { 9u, &script_hash_dsprXX },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram program_dspr_trans_t = {
    "dspr_trans_t", 4u,
    {
        { 9u, &script_hash_dspr_trans_R },
        { 9u, &script_hash_dspr_trans_W },
        { 9u, &script_hash_dspr_trans_B },
        { 9u, &script_hash_dsprshad },
    }
};

static const WmRopeCommandProgram *const source_programs[] = {
    &program_front_bounceud4_t,
    &program_back_bounceud4_t,
    &program_side_bounceud4_t,
    &program_front_bounceud3_t,
    &program_back_bounceud3_t,
    &program_side_bounceud3_t,
    &program_front_bounceud2_t,
    &program_back_bounceud2_t,
    &program_side_bounceud2_t,
    &program_front_bounceud1_t,
    &program_back_bounceud1_t,
    &program_side_bounceud1_t,
    &program_side_bounceio_t,
    &program_sspr11_t,
    &program_sspr12_t,
    &program_sspr13_t,
    &program_sspr14_t,
    &program_sspr15_t,
    &program_sspr21_t,
    &program_sspr22_t,
    &program_sspr23_t,
    &program_sspr24_t,
    &program_sspr25_t,
    &program_sspr31_t,
    &program_sspr32_t,
    &program_sspr33_t,
    &program_sspr34_t,
    &program_sspr35_t,
    &program_sspr41_t,
    &program_sspr42_t,
    &program_sspr43_t,
    &program_sspr44_t,
    &program_sspr45_t,
    &program_sspr51_t,
    &program_sspr52_t,
    &program_sspr53_t,
    &program_sspr54_t,
    &program_sspr55_t,
    &program_sspr_trans_t,
    &program_dspr11_t,
    &program_dspr12_t,
    &program_dspr13_t,
    &program_dspr14_t,
    &program_dspr15_t,
    &program_dspr16_t,
    &program_dspr21_t,
    &program_dspr22_t,
    &program_dspr23_t,
    &program_dspr24_t,
    &program_dspr25_t,
    &program_dspr26_t,
    &program_dspr31_t,
    &program_dspr32_t,
    &program_dspr33_t,
    &program_dspr34_t,
    &program_dspr35_t,
    &program_dspr36_t,
    &program_dspr41_t,
    &program_dspr42_t,
    &program_dspr43_t,
    &program_dspr44_t,
    &program_dspr45_t,
    &program_dspr46_t,
    &program_dspr51_t,
    &program_dspr52_t,
    &program_dspr53_t,
    &program_dspr54_t,
    &program_dspr55_t,
    &program_dspr56_t,
    &program_dspr_trans_t,
};


static const WmRopeScript *const source_all_scripts[] = {
    &script_hash_ssprXX,
    &script_hash_dsprXX,
    &script_front_bounceud4_R,
    &script_front_bounceud3_R,
    &script_front_bounceud2_R,
    &script_front_bounceud1_R,
    &script_back_bounceud4_R,
    &script_back_bounceud3_R,
    &script_back_bounceud2_R,
    &script_back_bounceud1_R,
    &script_side_bounceud4_R,
    &script_side_bounceud3_R,
    &script_side_bounceud2_R,
    &script_side_bounceud1_R,
    &script_front_bounceud4_W,
    &script_front_bounceud3_W,
    &script_front_bounceud2_W,
    &script_front_bounceud1_W,
    &script_back_bounceud4_W,
    &script_back_bounceud3_W,
    &script_back_bounceud2_W,
    &script_back_bounceud1_W,
    &script_side_bounceud4_W,
    &script_side_bounceud3_W,
    &script_side_bounceud2_W,
    &script_side_bounceud1_W,
    &script_front_bounceud4_B,
    &script_front_bounceud3_B,
    &script_front_bounceud2_B,
    &script_front_bounceud1_B,
    &script_back_bounceud4_B,
    &script_back_bounceud3_B,
    &script_back_bounceud2_B,
    &script_back_bounceud1_B,
    &script_side_bounceud4_B,
    &script_side_bounceud3_B,
    &script_side_bounceud2_B,
    &script_side_bounceud1_B,
    &script_side_bounceud_S,
    &script_side_bounceio_R,
    &script_side_bounceio2_R,
    &script_side_bounceio_W,
    &script_side_bounceio2_W,
    &script_side_bounceio_B,
    &script_side_bounceio_S,
    &script_hash_sspr11,
    &script_hash_sspr12,
    &script_hash_sspr13,
    &script_hash_sspr14,
    &script_hash_sspr15,
    &script_hash_sspr16,
    &script_hash_sspr21,
    &script_hash_sspr22,
    &script_hash_sspr23,
    &script_hash_sspr24,
    &script_hash_sspr25,
    &script_hash_sspr26,
    &script_hash_sspr31,
    &script_hash_sspr32,
    &script_hash_sspr33,
    &script_hash_sspr34,
    &script_hash_sspr35,
    &script_hash_sspr36,
    &script_hash_sspr41,
    &script_hash_sspr42,
    &script_hash_sspr43,
    &script_hash_sspr44,
    &script_hash_sspr45,
    &script_hash_sspr46,
    &script_hash_sspr51,
    &script_hash_sspr52,
    &script_hash_sspr53,
    &script_hash_sspr54,
    &script_hash_sspr55,
    &script_hash_sspr56,
    &script_hash_sprshad11,
    &script_hash_sprshad12,
    &script_hash_sprshad13,
    &script_hash_sprshad14,
    &script_hash_sprshad15,
    &script_hash_sprshad21,
    &script_hash_sprshad22,
    &script_hash_sprshad23,
    &script_hash_sprshad24,
    &script_hash_sprshad25,
    &script_hash_sprshad31,
    &script_hash_sprshad32,
    &script_hash_sprshad33,
    &script_hash_sprshad34,
    &script_hash_sprshad35,
    &script_hash_sprshad41,
    &script_hash_sprshad42,
    &script_hash_sprshad43,
    &script_hash_sprshad44,
    &script_hash_sprshad45,
    &script_hash_sprshad51,
    &script_hash_sprshad52,
    &script_hash_sprshad53,
    &script_hash_sprshad54,
    &script_hash_sprshad55,
    &script_hash_sspr_trans_R,
    &script_hash_sspr_trans_W,
    &script_hash_sspr_trans_B,
    &script_hash_sprshad_trans,
    &script_hash_dsprshad,
    &script_hash_dspr11,
    &script_hash_dspr12,
    &script_hash_dspr13,
    &script_hash_dspr14,
    &script_hash_dspr15,
    &script_hash_dspr16,
    &script_hash_dspr17,
    &script_hash_dspr21,
    &script_hash_dspr22,
    &script_hash_dspr23,
    &script_hash_dspr24,
    &script_hash_dspr25,
    &script_hash_dspr26,
    &script_hash_dspr27,
    &script_hash_dspr31,
    &script_hash_dspr32,
    &script_hash_dspr33,
    &script_hash_dspr34,
    &script_hash_dspr35,
    &script_hash_dspr36,
    &script_hash_dspr37,
    &script_hash_dspr41,
    &script_hash_dspr42,
    &script_hash_dspr43,
    &script_hash_dspr44,
    &script_hash_dspr45,
    &script_hash_dspr46,
    &script_hash_dspr47,
    &script_hash_dspr51,
    &script_hash_dspr52,
    &script_hash_dspr53,
    &script_hash_dspr54,
    &script_hash_dspr55,
    &script_hash_dspr56,
    &script_hash_dspr57,
    &script_hash_dspr_trans_R,
    &script_hash_dspr_trans_W,
    &script_hash_dspr_trans_B,
};

size_t wm_rope_source_script_count(void)
{
    return sizeof(source_all_scripts)/sizeof(source_all_scripts[0]);
}

const WmRopeScript *wm_rope_source_script_at(size_t index)
{
    if (index >= wm_rope_source_script_count()) {
        return 0;
    }
    return source_all_scripts[index];
}

const WmRopeCommandProgram *wm_rope_source_program_resolver(
    void *user,
    const char *source_script_table)
{
    size_t i;
    (void)user;

    if (source_script_table == 0) {
        return 0;
    }

    for (i = 0u; i < sizeof(source_programs)/sizeof(source_programs[0]); ++i) {
        if (strcmp(source_programs[i]->source_label,
                   source_script_table) == 0) {
            return source_programs[i];
        }
    }

    return 0;
}

size_t wm_rope_source_program_count(void)
{
    return sizeof(source_programs)/sizeof(source_programs[0]);
}

const WmRopeCommandProgram *wm_rope_source_program_at(size_t index)
{
    if (index >= wm_rope_source_program_count()) {
        return 0;
    }
    return source_programs[index];
}

const WmRopeSourceImagePair *wm_rope_source_image_pair(
    const char *source_pair_label)
{
    size_t i;

    if (source_pair_label == 0) {
        return 0;
    }

    for (i = 0u; i < sizeof(source_pairs)/sizeof(source_pairs[0]); ++i) {
        if (strcmp(source_pairs[i].source_pair_label,
                   source_pair_label) == 0) {
            return &source_pairs[i];
        }
    }

    return 0;
}

size_t wm_rope_source_image_pair_count(void)
{
    return sizeof(source_pairs)/sizeof(source_pairs[0]);
}

const WmRopeSourceImagePair *wm_rope_source_image_pair_at(size_t index)
{
    if (index >= wm_rope_source_image_pair_count()) {
        return 0;
    }
    return &source_pairs[index];
}
