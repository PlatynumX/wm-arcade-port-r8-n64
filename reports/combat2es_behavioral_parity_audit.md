# Combat2ES behavioral parity audit

This audit starts after active SMOVE label closure. It is not a gameplay change.

## Strict manifest status

- strict_manifest_ran: True
- strict_manifest_rc: 0
- manifest_entries: 63
- source_exact_entries: 63

## Finding counts

- minor: 3
- info: 1

## Findings

### MINOR: ck_ignore_a8_direction_table_needs_source_proof

- file: `src/fix39/wm_fix39_runtime.c`
- evidence: src/fix39/wm_fix39_runtime.c:1796: urce message event for the lifebar/message renderer path. */ source_message_event(a, "BONUS_MESS", bonus, WM_FIX39_MESSAGE_PID_BONUS); } static int source_ck_ignore_a8_port(wm_arcade_actor_t *a, void *user) { uint16_t away = 0u; (void)user; if (!a) return 1; /* WRESTLE.ASM::ck_ignore_a8: index NEW_FACING_DIR throu
- why: ck_ignore/ck_ignore_a8 is direction-table logic in WRESTLE.ASM. The local translation needs exact mv_tbl coverage evidence.
- next: Add a small table-driven test against the WRESTLE.ASM mv_tbl mapping for all facing/move_dir cases.

### INFO: source_exact_body_flags_present

- file: `src/fix39/wm_arcade_smove_runtime.c`
- evidence: src/fix39/wm_arcade_smove_runtime.c:423: file; p->process_label = label; p->entry = wm_arcade_smove_lookup_entry(profile->id, label); p->unresolved = p->entry == 0 || p->entry->source_exact_body == 0; proc_rewind(0, p, 1); ++made; ++rt->created; if (p->unresolved) ++rt->unresolved_created; } (void)owner; retu
- why: The boolean exact-body flag is only a checklist marker; this audit tracks where deeper proof is still needed.
- next: Review each occurrence and either back it with exact source citations/tests or remove the approximation.

### MINOR: zero_step_charge_body_review

- file: `src/fix39/wm_arcade_smove_runtime.c`
- evidence: zero-step source_exact bodies: hrt_charge_face_rake, hrt_charge_flying_kick, rzr_charge_slashes, shn_charge_suplex, bam_charge_neckbreaker, dnk_charge_flykick
- why: Hold/release monitors do not go through WAITSWITCH. They need separate timing review for charge thresholds and reset behavior.
- next: Add focused tests for every zero-step body: hold duration below threshold, threshold hit, release reset, mode rejection, and sound/result side effects.

### MINOR: direct_special_move_addr_assignments

- file: `src/fix39/wm_arcade_smove_runtime.c`
- evidence: direct assignments found: 6
- why: Some source bodies pick FACE24/directional result labels manually. Direct writes must still use the label resolver and match source facing choices.
- next: For each direct assignment, add tests for both left/right or up/down facing choices and verify resolve_label_token is called.

