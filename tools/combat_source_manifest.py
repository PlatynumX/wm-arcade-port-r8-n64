#!/usr/bin/env python3
"""Source-derived combat manifest for WWF WrestleMania arcade port.

This is a manifest of final-game LIVE source tables.  It is not a wishlist and
not a mod table.  Entries are taken from GAME.EQU NUM_*_FINISHES and the
*_smove_table blocks in BRET/RAZOR/TAKER/YOKO/SHAWN/BAM/DOINK/LEX ASM.
"""
from __future__ import annotations

GAME_EQU_FINISH_COUNTS = {
    "BRET": 0,
    "BAM": 0,
    "YOKO": 0,
    "DOINK": 0,
    "RAZOR": 0,
    "LEX": 0,
    "TAKER": 1,
    "SHAWN": 0,
}

# Roster file locations used by the current R11e/R13 port tree.
ROSTER_TABLES = {
    "BRET": ("src/fix39/wm_arcade_wrestler_port.c", "bret_smove"),
    "RAZOR": ("src/fix39/wm_arcade_wrestler_port.c", "razor_smove"),
    "TAKER": ("src/fix39/wm_arcade_taker.c", "special_processes"),
    "YOKO": ("src/fix39/wm_arcade_yoko.c", "special_processes"),
    "SHAWN": ("src/fix39/wm_arcade_shawn.c", "special_processes"),
    "BAM": ("src/fix39/wm_arcade_bam.c", "special_processes"),
    "DOINK": ("src/fix39/wm_arcade_doink.c", "special_processes"),
    "LEX": ("src/fix39/wm_arcade_lex.c", "special_processes"),
}

# Final assembled *_smove_table entries.  Conditional finisher entries are
# resolved using GAME_EQU_FINISH_COUNTS above.
ACTIVE_SMOVE_TABLES = {
    "BRET": [
        "hrt_charge_flying_kick",
        "hrt_charge_face_rake",
        "hrt_hdhold_pile",
        "hrt_hdhold_ddt",
        "hrt_hdhold_faceslam",
        "hrt_grab_toss_air",
        "hrt_roll_uppercut",
        "hrt_hdhold_combo1",
        "hrt_hdhold_combo2",
        "std_walk_fast",
        "std_taunt",
    ],
    "RAZOR": [
        "rzr_charge_slashes",
        "rzr_hdhold_pile",
        "rzr_hdhold_combo1",
        "rzr_hdhold_edge",
        "rzr_hdhold_rug",
        "rzr_grab_toss_air",
        "rzr_hdhold_combo2",
        "std_walk_fast",
        "std_taunt",
        "rzr_sliding_rug",
    ],
    "TAKER": [
        "und_hdhold_neckbrk",
        "und_hdhold_faceslam",
        "und_hdhold_pile",
        "und_spirit_pull",
        "und_spirit_push",
        "und_grab_toss_air",
        "und_hdhold_combo1",
        "und_hdhold_combo2",
        "und_choke_slide",
        "std_walk_fast",
        "std_taunt",
        "und_finish_move1",
    ],
    "YOKO": [
        "yok_hdhold_combo1",
        "yok_hdhold_scissor",
        "yok_hdhold_suplex",
        "yok_salt_throw",
        "yok_grab_toss_air",
        "yok_hdhold_combo2",
        "std_walk_fast",
        "std_taunt",
    ],
    "SHAWN": [
        "shn_hdhold_combo2",
        "shn_hdhold_combo1",
        "shn_charge_suplex",
        "shn_swirl_speedkick",
        "shn_sliding_kicktoss",
        "shn_hdhold_suplex",
        "shn_hdhold_frank",
        "shn_hdhold_kicktoss",
        "shn_hdhold_butts",
        "shn_flipslam",
        "shn_grab_toss_air",
        "std_walk_fast",
        "std_taunt",
    ],
    "BAM": [
        "bam_charge_neckbreaker",
        "bam_hdhold_combo1",
        "bam_hdhold_pile",
        "bam_hdhold_pogo",
        "bam_hdhold_combo2",
        "bam_grab_toss_air",
        "std_walk_fast",
        "std_taunt",
    ],
    "DOINK": [
        "dnk_charge_flykick",
        "dnk_hdhold_slam",
        "dnk_hdhold_combo1",
        "dnk_hdhold_pile",
        "dnk_hdhold_combo2",
        "dnk_hdhold_buzz",
        "dnk_grab_toss_air",
        "std_walk_fast",
        "std_taunt",
    ],
    "LEX": [
        "lex_hdhold_pile",
        "lex_hdhold_elbow_face",
        "lex_hdhold_graboh",
        "lex_grab_toss_air",
        "lex_hdhold_combo1",
        "lex_hdhold_combo2",
        "std_walk_fast",
        "std_taunt",
    ],
}

DISABLED_FINISH_LABELS = [
    "hrt_finish_move1", "hrt_finish_move2",
    "rzr_finish_move1", "rzr_finish_move2",
    "yok_finish_move1", "yok_finish_move2",
    "shn_finish_move1", "shn_finish_move2",
    "bam_finish_move1", "bam_finish_move2",
    "dnk_finish_move1", "dnk_finish_move2",
    "lex_finish_move1", "lex_finish_move2",
    "und_finish_move2",
]

# Secret move history/input tables.  These are tracked separately from the
# persistent SMOVE_PID monitor processes.
ACTIVE_SECRET_TABLES = {
    "BRET": ["charge_ddt", "neck_grab", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "face_rake", "jump_kick", "supercut"],
    "RAZOR": ["charge_flying_kick", "neck_grab", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "down_slash"],
    "TAKER": ["button_hold", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "neck_grab", "tomb_smash"],
    "YOKO": ["charge_salt", "neck_grab", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "scissors", "gut_push", "jabs"],
    "SHAWN": ["charge_flying_kick", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "neck_grab", "frankensteiner", "jump_kick"],
    # BAM source intentionally lists grab_fling2/hip_toss2 twice; preserve order.
    "BAM": ["firepnch", "neck_grab", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "jumpkick", "grab_fling2", "hip_toss2", "napalm"],
    "DOINK": ["charge_buzz", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "earslap", "hammer", "neck_grab", "boxing_pnch"],
    "LEX": ["charge_clobber", "neck_grab", "grab_fling", "hip_toss", "grab_fling2", "hip_toss2", "sliding_elbow", "hammer"],
}

SOURCE_NOTES = {
    "GAME.EQU": "NUM_*_FINISHES final config: only NUM_TAKER_FINISHES == 1; all other wrestler finisher counts == 0.",
    "MACROS.H": "WAITSWITCH_DWN uses (BUT_VAL_DOWN << 4) | STICK_REL_NEW, then andni MASK; it does not pre-mask buttons/stick.",
    "WRESTLE2.ASM": "init_smoves creates persistent SMOVE_PID monitor processes from each active smove table entry.",
}


def all_active_smove_labels() -> list[str]:
    out: list[str] = []
    for labels in ACTIVE_SMOVE_TABLES.values():
        out.extend(labels)
    return out
