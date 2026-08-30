# ANIM.ASM attachment/paired-state source-parity guard

- status: PASS
- scope: corrected attachment/paired-state opcode subset only
- source: historicalsource/wwf-wrestlemania/ANIM.ASM
- ANI_SUPERSLAVE2 (opcode 79): OPEN — requires exact get_mpart_offsets/get_mpart_xsize translation

## Guarded corrections

- ANI_ATTACH (9): X-only attachment offset write
- ANI_WAITHITGND (11): caller Y-velocity gate before paired/self ground probes
- ANI_ATTACHZ (18): X/Z argument mapping; no Y write
- ANI_ATTACHVEL (24): source non-null attachment-chain predicate
- ANI_OPP_GETUP (76): ATTACH_PROC else WHOIHIT only
- ANI_OPPOFFSET (82): source non-null attachment-chain predicate
- ANI_SETOPPFACING (85): source non-null attachment-chain predicate
- ANI_IMMOBILIZE (108): caller dizzy and victim block predicates
- ANI_WAITHITGND2 (111): caller Y-velocity gate before paired/self ground probes
- ANI_CLEAR_COMBO_COUNT (115): ATTACH_PROC else WHOIHIT only

## Open frontier

ANI_SUPERSLAVE2 still writes raw slave-table X/Y offsets in the current port. Midway additionally calls get_mpart_offsets and get_mpart_xsize for both images, then performs flip-sensitive multipart math. R37N5 does not guess that metadata mapping.
