#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ORIG="$ROOT/original/wwf-wrestlemania"
if [ ! -f "$ORIG/ANIM.EQU" ] || [ ! -f "$ORIG/FINISEQ.ASM" ] || \
   [ ! -f "$ORIG/HRTSEQ1.ASM" ] || [ ! -f "$ORIG/HRTSEQ2.ASM" ] || \
   [ ! -f "$ORIG/ATTRACT.ASM" ] || [ ! -f "$ORIG/SELECT.ASM" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi
python3 "$ROOT/tools/asmseq.py" \
    --equ "$ORIG/ANIM.EQU" \
    --source "$ORIG/FINISEQ.ASM" \
    --label hrt_finish1_move \
    --symbol wm_seq_hrt_finish1_move \
    --out "$ROOT/src/generated/finish_sequences.c"
python3 "$ROOT/tools/wlanim.py" \
    --source "$ORIG/HRTSEQ1.ASM" \
    --sequence hrt_stand2_anim wm_bret_stand2_anim bret_stand2_frames \
    --sequence hrt_stand4_anim wm_bret_stand4_anim bret_stand4_frames \
    --sequence hrt_torso2_anim wm_bret_torso2_anim bret_torso2_frames \
    --sequence hrt_torso4_anim wm_bret_torso4_anim bret_torso4_frames \
    --sequence hrt_torso6_anim wm_bret_torso6_anim bret_torso6_frames \
    --sequence hrt_torso8_anim wm_bret_torso8_anim bret_torso8_frames \
    --sequence hrt_walk1_f2_anim wm_bret_walk1_f2_anim bret_walk1_f2_frames \
    --sequence hrt_walk1_f4_anim wm_bret_walk1_f4_anim bret_walk1_f4_frames \
    --sequence hrt_walk2_f2_anim wm_bret_walk2_f2_anim bret_walk2_f2_frames \
    --sequence hrt_walk2_f4_anim wm_bret_walk2_f4_anim bret_walk2_f4_frames \
    --sequence hrt_walk4_f2_anim wm_bret_walk4_f2_anim bret_walk4_f2_frames \
    --sequence hrt_walk4_f4_anim wm_bret_walk4_f4_anim bret_walk4_f4_frames \
    --sequence hrt_walk5_f2_anim wm_bret_walk5_f2_anim bret_walk5_f2_frames \
    --sequence hrt_walk5_f4_anim wm_bret_walk5_f4_anim bret_walk5_f4_frames \
    --sequence hrt_walk6_f2_anim wm_bret_walk6_f2_anim bret_walk6_f2_frames \
    --sequence hrt_walk6_f4_anim wm_bret_walk6_f4_anim bret_walk6_f4_frames \
    --sequence hrt_walk8_f2_anim wm_bret_walk8_f2_anim bret_walk8_f2_frames \
    --sequence hrt_walk8_f4_anim wm_bret_walk8_f4_anim bret_walk8_f4_frames \
    --sequence hrt_2_to_4_turn_anim wm_bret_2_to_4_turn_anim bret_2_to_4_turn_frames \
    --sequence hrt_4_to_2_turn_anim wm_bret_4_to_2_turn_anim bret_4_to_2_turn_frames \
    --sequence hrt_4_to_6_turn_anim wm_bret_4_to_6_turn_anim bret_4_to_6_turn_frames \
    --sequence hrt_2_to_8_turn_anim wm_bret_2_to_8_turn_anim bret_2_to_8_turn_frames \
    --sequence hrt_4_to_8_turn_anim wm_bret_4_to_8_turn_anim bret_4_to_8_turn_frames \
    --sequence hrt_2_to_6_turn_anim wm_bret_2_to_6_turn_anim bret_2_to_6_turn_frames \
    --sequence hrt_2_to_4_turn2_anim wm_bret_2_to_4_turn2_anim bret_2_to_4_turn2_frames \
    --sequence hrt_4_to_2_turn2_anim wm_bret_4_to_2_turn2_anim bret_4_to_2_turn2_frames \
    --sequence hrt_4_to_6_turn2_anim wm_bret_4_to_6_turn2_anim bret_4_to_6_turn2_frames \
    --sequence hrt_2_to_8_turn2_anim wm_bret_2_to_8_turn2_anim bret_2_to_8_turn2_frames \
    --sequence hrt_4_to_8_turn2_anim wm_bret_4_to_8_turn2_anim bret_4_to_8_turn2_frames \
    --sequence hrt_2_to_6_turn2_anim wm_bret_2_to_6_turn2_anim bret_2_to_6_turn2_frames \
    --slice hrt_run_anim wm_bret_run_anim bret_run_frames true \
    --out "$ROOT/src/generated/bret_visuals.c"
python3 "$ROOT/tools/wlanim.py" \
    --source "$ORIG/HRTSEQ2.ASM" \
    --slice hrt_2_punch_anim wm_bret_light_punch2_anim bret_light_punch2_frames false \
    --slice hrt_4_punch_anim wm_bret_light_punch4_anim bret_light_punch4_frames false \
    --slice hrt_4_super_punch_anim wm_bret_power_punch_anim bret_power_punch_frames false \
    --slice hrt_2_super_punch2_anim wm_bret_super_punch2_2_anim bret_super_punch2_2_frames false \
    --slice hrt_4_super_punch2_anim wm_bret_super_punch2_4_anim bret_super_punch2_4_frames false \
    --slice hrt_2_kick_anim wm_bret_light_kick2_anim bret_light_kick2_frames false \
    --slice hrt_4_kick_anim wm_bret_light_kick4_anim bret_light_kick4_frames false \
    --slice hrt_2_super_kick_anim wm_bret_power_kick_anim bret_power_kick_frames false \
    --slice hrt_2_butt_anim wm_bret_butt2_anim bret_butt2_frames false \
    --slice hrt_4_butt_anim wm_bret_butt4_anim bret_butt4_frames false \
    --slice hrt_2_knee_anim wm_bret_knee2_anim bret_knee2_frames false \
    --slice hrt_4_knee_anim wm_bret_knee4_anim bret_knee4_frames false \
    --slice hrt_4_uppercut_anim wm_bret_uppercut4_anim bret_uppercut4_frames false \
    --slice hrt_2_stomp_anim wm_bret_stomp2_anim bret_stomp2_frames false \
    --slice hrt_4_stomp_anim wm_bret_stomp4_anim bret_stomp4_frames false \
    --slice hrt_2_ground_punch_anim wm_bret_ground_punch2_anim bret_ground_punch2_frames false \
    --slice hrt_4_ground_punch_anim wm_bret_ground_punch4_anim bret_ground_punch4_frames false \
    --slice hrt_4_push_anim wm_bret_push4_anim bret_push4_frames false \
    --slice hrt_4_jump_kick_anim wm_bret_jump_kick4_anim bret_jump_kick4_frames false \
    --slice hrt_4_knee_fall_anim wm_bret_knee_fall4_anim bret_knee_fall4_frames false \
    --slice hrt_kick_TB_anim wm_bret_kick_tb_anim bret_kick_tb_frames false \
    --slice hrt_4_knee_to_head_anim wm_bret_knee_to_head4_anim bret_knee_to_head4_frames false \
    --slice hrt_knees_to_head_anim wm_bret_knees_to_head_anim bret_knees_to_head_frames false \
    --slice hrt_2_pin_anim wm_bret_pin2_anim bret_pin2_frames false \
    --slice hrt_4_pin_anim wm_bret_pin4_anim bret_pin4_frames false \
    --slice hrt_2_butts_anim wm_bret_butts2_anim bret_butts2_frames false \
    --slice hrt_4_butts_anim wm_bret_butts4_anim bret_butts4_frames false \
    --slice hrt_flying_kick_anim wm_bret_flying_kick_anim bret_flying_kick_frames false \
    --slice hrt_tbukl_leap_anim wm_bret_tbukl_leap_anim bret_tbukl_leap_frames false \
    --slice hrt_running_ground_punch_anim wm_bret_running_ground_punch_anim bret_running_ground_punch_frames false \
    --slice hrt_combo_punch_anim wm_bret_combo_punch_anim bret_combo_punch_frames false \
    --slice hrt_combo_kick_anim wm_bret_combo_kick_anim bret_combo_kick_frames false \
    --out "$ROOT/src/generated/bret_attacks.c"
python3 "$ROOT/tools/wlanim.py" \
    --source "$ORIG/HRTSEQ4.ASM" \
    --slice hrt_4_block_anim wm_bret_block4_anim bret_block4_frames false \
    --slice hrt_fall_back_anim wm_bret_fall_back_anim bret_fall_back_frames false \
    --slice hrt_facedown_getup_anim wm_bret_facedown_getup_anim bret_facedown_getup_frames false \
    --slice hrt_faceup_getup_anim wm_bret_faceup_getup_anim bret_faceup_getup_frames false \
    --slice hrt_4_faceup_getup2_anim wm_bret_faceup_getup2_4_anim bret_faceup_getup2_4_frames false \
    --slice hrt_hitonground_facedown_anim wm_bret_hitonground_facedown_anim bret_hitonground_facedown_frames false \
    --out "$ROOT/src/generated/bret_defense.c"
python3 "$ROOT/tools/wlanim.py" \
    --source "$ORIG/HRTSEQ3.ASM" \
    --slice hrt_3_head_held_stand_anim wm_bret_head_held_stand3_anim bret_head_held_stand3_frames false \
    --slice hrt_3_fake_hold_anim wm_bret_fake_hold3_anim bret_fake_hold3_frames false \
    --out "$ROOT/src/generated/bret_grapple.c"
if [ ! -f "$ORIG/IMG/BRET.LOD" ]; then
    sh "$ROOT/scripts/fetch_original.sh"
fi
python3 "$ROOT/tools/wlcommands.py" \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_punch_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_punch_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_super_punch_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_super_punch2_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_super_punch2_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_kick_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_kick_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_super_kick_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_butt_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_butt_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_knee_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_knee_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_uppercut_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_stomp_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_stomp_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_ground_punch_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_ground_punch_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_push_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_jump_kick_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_knee_fall_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_kick_TB_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_knee_to_head_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_knees_to_head_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_pin_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_pin_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_2_butts_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_4_butts_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_flying_kick_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_tbukl_leap_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_running_ground_punch_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_combo_punch_anim \
    --animation "$ORIG/HRTSEQ2.ASM" hrt_combo_kick_anim \
    --animation "$ORIG/HRTSEQ3.ASM" hrt_3_head_held_stand_anim \
    --animation "$ORIG/HRTSEQ3.ASM" hrt_3_fake_hold_anim \
    --animation "$ORIG/HRTSEQ4.ASM" hrt_4_block_anim \
    --animation "$ORIG/HRTSEQ4.ASM" hrt_fall_back_anim \
    --animation "$ORIG/HRTSEQ4.ASM" hrt_facedown_getup_anim \
    --animation "$ORIG/HRTSEQ4.ASM" hrt_faceup_getup_anim \
    --animation "$ORIG/HRTSEQ4.ASM" hrt_4_faceup_getup2_anim \
    --animation "$ORIG/HRTSEQ4.ASM" hrt_hitonground_facedown_anim \
    --out "$ROOT/src/generated/bret_frame_commands.c"
python3 "$ROOT/tools/bret_geometry_bundle.py" \
    --lod "$ORIG/IMG/BRET.LOD" \
    --img-dir "$ORIG/IMG" \
    --visual-source "$ROOT/src/generated/bret_visuals.c" \
    --visual-source "$ROOT/src/generated/bret_attacks.c" \
    --visual-source "$ROOT/src/generated/bret_defense.c" \
    --visual-source "$ROOT/src/generated/bret_grapple.c" \
    --out "$ROOT/src/generated/bret_frame_geometry.c"
python3 "$ROOT/tools/select_source.py" \
    --source "$ORIG/SELECT.ASM" \
    --out "$ROOT/src/generated/select_tables.c"
python3 "$ROOT/tools/attract_sequence.py" \
    --source "$ORIG/ATTRACT.ASM" \
    --out "$ROOT/src/generated/attract_sequence.c"
python3 "$ROOT/tools/port_manifest.py" \
    --manifest "$ROOT/port/translation_manifest.json" \
    --out-c "$ROOT/src/generated/port_status.c" \
    --out-md "$ROOT/docs/PORT_COVERAGE.md" \
    --source-root "$ORIG"
