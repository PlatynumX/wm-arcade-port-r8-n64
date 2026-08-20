# Stage 19 source audit — Bam Bam Bigelow

Source: `BAM.ASM` (2937 lines)
`https://raw.githubusercontent.com/historicalsource/wwf-wrestlemania/main/BAM.ASM`

Translated in the dedicated `wm_arcade_bam.c/.h` module:

- The source 26-mode dispatcher and 32-action table are retained locally in this wrestler module; they are not executed through a generic cross-wrestler behavior engine.
- `bam_secret_moves` order retained exactly, including the duplicated `grab_fling2` / `hip_toss2` entries present in source.
- Fire-punch release threshold: 85 PUNCH ticks, source `bam_2_fpunch_anim` / `bam_4_fpunch_anim` labels.
- `jumpkick`: SKICK, away, away, max 32 -> `bam_4_jumpkick_anim`.
- `napalm`: PUNCH, down, down, max 50 -> source Napalm animation labels.
- 50x45 punch/headbutt routing, 160x140 grounded elbow-drop branch, and 90x55 normal super-punch close threshold.
- Bam-specific belly-flop/turnbuckle labels preserved.
- Full `bam_smove_table` label order exported.
