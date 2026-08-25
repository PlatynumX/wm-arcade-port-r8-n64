# Combat2ES behavioral parity audit

This audit starts after active SMOVE label closure. It is not a gameplay change.

## Strict manifest status

- strict_manifest_ran: True
- strict_manifest_rc: 0
- manifest_entries: 63
- source_exact_entries: 63

## Finding counts

- major: 3
- minor: 3
- info: 1

## Findings

### MAJOR: reversal_message_not_text_object_verified

- file: `src/fix39/wm_fix39_runtime.c`
- evidence: src/fix39/wm_fix39_runtime.c:1750: ource uses DO_REVERSAL before WHOHITME-targeted counter throws. Preserve actor-owned reversal state here; presentation messaging is the companion DO_REVERSAL_MESS hook below. */ (void)user; if (!a) return; a->status_flags |= WM_STATUS_COMBO_BROKEN; a->anti_combo_time = g.status.pcnt; } static void source
- why: DO_REVERSAL_MESS is a source message effect. Routing it through trace/sound metadata is not proof that the arcade message object/timing exists.
- next: Port or connect the real message renderer path and assert timing/object spawn semantics.

### MAJOR: bonus_message_synthesized_label

- file: `src/fix39/wm_fix39_runtime.c`
- evidence: src/fix39/wm_fix39_runtime.c:1774: object/spawn backend can consume the same hook later without changing SMOVE bodies. */ a->damage_given += bonus; snprintf(label, sizeof(label), "BONUS_MESS_%03d", bonus); common_sound_label(a, label, 0); } static int source_ck_ignore_a8_port(wm_arcade_actor_t *a, void *user) { uint16_t away = 0u; (void
- why: BONUS_MESS was converted into a synthesized label/accounting hook. That may not match source scoring, text lifetime, message PID, or display timing.
- next: Translate BONUS_MESS as a real source message process and compare bonus values/timing per wrestler.

### MAJOR: bonus_message_changes_damage_counter

- file: `src/fix39/wm_fix39_runtime.c`
- evidence: src/fix39/wm_fix39_runtime.c:1773: /* Runtime-visible accounting for BONUS_MESS(A10). The text object/spawn backend can consume the same hook later without changing SMOVE bodies. */ a->damage_given += bonus; snprintf(label, sizeof(label), "BONUS_MESS_%03d", bonus); common_sound_label(a, label, 0); } static int source_ck_ignore_a8_port(wm_arcade_actor_t *a
- why: Damage accounting and bonus-message scoring are not necessarily the same source side effect.
- next: Verify the original accounting path before modifying damage_given as a stand-in.

### MINOR: ck_ignore_a8_direction_table_needs_source_proof

- file: `src/fix39/wm_fix39_runtime.c`
- evidence: src/fix39/wm_fix39_runtime.c:1778: SMOVE bodies. */ a->damage_given += bonus; snprintf(label, sizeof(label), "BONUS_MESS_%03d", bonus); common_sound_label(a, label, 0); } static int source_ck_ignore_a8_port(wm_arcade_actor_t *a, void *user) { uint16_t away = 0u; (void)user; if (!a) return 1; /* WRESTLE.ASM::ck_ignore_a8: index NEW_FACING_DIR throu
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

