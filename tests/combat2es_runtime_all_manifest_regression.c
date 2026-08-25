#include "wm_arcade_smove_runtime.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TOKEN_BASE ((uintptr_t)0x73000000u)

typedef struct ManifestProbe {
    int resolve_count;
    int sound_count;
    int combo_value;
    int combo_checks;
    int find_kill_count;
    int reversal_count;
    int reversal_message_count;
    int bonus_count;
    int ck_ignore_count;
    const char *last_resolved;
    const char *last_sound;
} ManifestProbe;

static uintptr_t resolve_cb(const char *source_label, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(source_label != 0);
    ++p->resolve_count;
    p->last_resolved = source_label;
    return TOKEN_BASE + (uintptr_t)p->resolve_count;
}

static void sound_cb(wm_arcade_actor_t *actor, const char *source_label, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(actor != 0);
    assert(source_label != 0);
    ++p->sound_count;
    p->last_sound = source_label;
}

static int combo_cb(wm_arcade_actor_t *actor, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(actor != 0);
    ++p->combo_checks;
    return p->combo_value;
}

static void find_kill_cb(wm_arcade_actor_t *actor, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(actor != 0);
    ++p->find_kill_count;
}

static void reversal_cb(wm_arcade_actor_t *actor, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(actor != 0);
    ++p->reversal_count;
}

static void reversal_message_cb(wm_arcade_actor_t *actor, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(actor != 0);
    ++p->reversal_message_count;
}

static void bonus_cb(wm_arcade_actor_t *actor, int bonus, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(actor != 0);
    assert(bonus >= 0);
    ++p->bonus_count;
}

static int ck_ignore_cb(wm_arcade_actor_t *actor, void *user)
{
    ManifestProbe *p = (ManifestProbe *)user;
    assert(actor != 0);
    ++p->ck_ignore_count;
    return 0;
}

static wm_arcade_smove_callbacks_t callbacks(ManifestProbe *probe)
{
    wm_arcade_smove_callbacks_t cb;
    memset(&cb, 0, sizeof(cb));
    cb.resolve_label_token = resolve_cb;
    cb.sound_label = sound_cb;
    cb.check_combo_go = combo_cb;
    cb.find_and_kill_endless = find_kill_cb;
    cb.do_reversal = reversal_cb;
    cb.do_reversal_message = reversal_message_cb;
    cb.bonus_message = bonus_cb;
    cb.ck_ignore = ck_ignore_cb;
    cb.user = probe;
    return cb;
}

static void actor_init(wm_arcade_actor_t *a, wm_arcade_roster_id_t id, int slot)
{
    memset(a, 0, sizeof(*a));
    a->active = 1;
    a->wrestler_num = (int32_t)id;
    a->player_num = slot;
    a->player_side = slot;
    a->player_mode = WM_PMODE_NORMAL;
    a->facing_dir = WM_MOVE_RIGHT;
    a->new_facing_dir = WM_MOVE_RIGHT;
    a->life = 100;
    a->closest_dist = 0x20;
    a->closest_xdist = 0x20;
    a->closest_zdist = 0x20;
}

static void single_proc(wm_arcade_smove_runtime_t *rt,
                        const wm_arcade_smove_entry_t *entry)
{
    wm_arcade_smove_proc_t *p;
    memset(rt, 0, sizeof(*rt));
    assert(entry != 0);
    p = &rt->proc[0];
    rt->proc_count = 1u;
    p->active = 1u;
    p->owner_slot = 0u;
    p->entry = entry;
    p->process_label = entry->process_label;
    p->timeout_loaded_for = 0xffu;
}

typedef enum manifest_gate {
    MANIFEST_GATE_NORMAL = 0,
    MANIFEST_GATE_HEADHOLD,
    MANIFEST_GATE_GRAB_TOSS_AIR,
    MANIFEST_GATE_CHARGE_PUNCH,
    MANIFEST_GATE_CHARGE_SPUNCH,
    MANIFEST_GATE_CHARGE_SKICK
} manifest_gate_t;

typedef struct manifest_gate_rule {
    const char *process_label;
    manifest_gate_t gate;
} manifest_gate_rule_t;

/*
 * Explicit source precondition map for every strict SMOVE manifest entry.
 *
 * R29B fixed yok_salt_throw by special-casing its label after the older test
 * guessed gate state from substrings.  R30 removes that heuristic completely:
 * every process label below is intentional, exact, and matched one-for-one
 * against the 63-entry source manifest.
 */
static const manifest_gate_rule_t manifest_gate_rules[] = {
    { "hrt_charge_face_rake", MANIFEST_GATE_CHARGE_PUNCH },
    { "hrt_charge_flying_kick", MANIFEST_GATE_CHARGE_SKICK },
    { "hrt_roll_uppercut", MANIFEST_GATE_NORMAL },
    { "hrt_hdhold_pile", MANIFEST_GATE_HEADHOLD },
    { "hrt_hdhold_ddt", MANIFEST_GATE_HEADHOLD },
    { "hrt_hdhold_faceslam", MANIFEST_GATE_HEADHOLD },
    { "hrt_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },
    { "hrt_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "hrt_hdhold_combo2", MANIFEST_GATE_HEADHOLD },

    { "rzr_charge_slashes", MANIFEST_GATE_CHARGE_PUNCH },
    { "rzr_hdhold_pile", MANIFEST_GATE_HEADHOLD },
    { "rzr_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "rzr_hdhold_edge", MANIFEST_GATE_HEADHOLD },
    { "rzr_hdhold_rug", MANIFEST_GATE_HEADHOLD },
    { "rzr_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },
    { "rzr_hdhold_combo2", MANIFEST_GATE_HEADHOLD },
    { "rzr_sliding_rug", MANIFEST_GATE_NORMAL },

    { "yok_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "yok_hdhold_scissor", MANIFEST_GATE_HEADHOLD },
    { "yok_hdhold_suplex", MANIFEST_GATE_HEADHOLD },
    { "yok_salt_throw", MANIFEST_GATE_HEADHOLD },
    { "yok_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },
    { "yok_hdhold_combo2", MANIFEST_GATE_HEADHOLD },

    { "shn_hdhold_combo2", MANIFEST_GATE_HEADHOLD },
    { "shn_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "shn_charge_suplex", MANIFEST_GATE_CHARGE_SPUNCH },
    { "shn_swirl_speedkick", MANIFEST_GATE_NORMAL },
    { "shn_sliding_kicktoss", MANIFEST_GATE_NORMAL },
    { "shn_hdhold_suplex", MANIFEST_GATE_HEADHOLD },
    { "shn_hdhold_frank", MANIFEST_GATE_HEADHOLD },
    { "shn_hdhold_kicktoss", MANIFEST_GATE_HEADHOLD },
    { "shn_hdhold_butts", MANIFEST_GATE_HEADHOLD },
    { "shn_flipslam", MANIFEST_GATE_NORMAL },
    { "shn_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },

    { "bam_charge_neckbreaker", MANIFEST_GATE_CHARGE_PUNCH },
    { "bam_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "bam_hdhold_pile", MANIFEST_GATE_HEADHOLD },
    { "bam_hdhold_pogo", MANIFEST_GATE_HEADHOLD },
    { "bam_hdhold_combo2", MANIFEST_GATE_HEADHOLD },
    { "bam_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },

    { "dnk_charge_flykick", MANIFEST_GATE_CHARGE_SKICK },
    { "dnk_hdhold_slam", MANIFEST_GATE_HEADHOLD },
    { "dnk_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "dnk_hdhold_pile", MANIFEST_GATE_HEADHOLD },
    { "dnk_hdhold_combo2", MANIFEST_GATE_HEADHOLD },
    { "dnk_hdhold_buzz", MANIFEST_GATE_HEADHOLD },
    { "dnk_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },

    { "lex_hdhold_pile", MANIFEST_GATE_HEADHOLD },
    { "lex_hdhold_elbow_face", MANIFEST_GATE_HEADHOLD },
    { "lex_hdhold_graboh", MANIFEST_GATE_HEADHOLD },
    { "lex_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },
    { "lex_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "lex_hdhold_combo2", MANIFEST_GATE_HEADHOLD },

    { "und_hdhold_neckbrk", MANIFEST_GATE_HEADHOLD },
    { "und_hdhold_faceslam", MANIFEST_GATE_HEADHOLD },
    { "und_hdhold_pile", MANIFEST_GATE_HEADHOLD },
    { "und_choke_slide", MANIFEST_GATE_NORMAL },
    { "und_spirit_push", MANIFEST_GATE_NORMAL },
    { "und_spirit_pull", MANIFEST_GATE_NORMAL },
    { "und_grab_toss_air", MANIFEST_GATE_GRAB_TOSS_AIR },
    { "und_hdhold_combo1", MANIFEST_GATE_HEADHOLD },
    { "und_hdhold_combo2", MANIFEST_GATE_HEADHOLD },
    { "und_finish_move1", MANIFEST_GATE_NORMAL }
};

static manifest_gate_t manifest_gate_for(const wm_arcade_smove_entry_t *e)
{
    assert(e != 0);
    assert(e->process_label != 0);
    assert(sizeof(manifest_gate_rules) / sizeof(manifest_gate_rules[0]) == 63u);
    for (size_t i = 0u; i < sizeof(manifest_gate_rules) / sizeof(manifest_gate_rules[0]); ++i) {
        if (strcmp(manifest_gate_rules[i].process_label, e->process_label) == 0)
            return manifest_gate_rules[i].gate;
    }
    fprintf(stderr, "missing explicit manifest gate rule for %s\n", e->process_label);
    assert(0 && "missing explicit manifest gate rule");
    return MANIFEST_GATE_NORMAL;
}

static int is_charge_entry(const wm_arcade_smove_entry_t *e)
{
    manifest_gate_t gate = manifest_gate_for(e);
    assert((gate == MANIFEST_GATE_CHARGE_PUNCH ||
            gate == MANIFEST_GATE_CHARGE_SPUNCH ||
            gate == MANIFEST_GATE_CHARGE_SKICK) == (e->step_count == 0u));
    return gate == MANIFEST_GATE_CHARGE_PUNCH ||
           gate == MANIFEST_GATE_CHARGE_SPUNCH ||
           gate == MANIFEST_GATE_CHARGE_SKICK;
}

static uint16_t charge_button_for(const wm_arcade_smove_entry_t *e)
{
    switch (manifest_gate_for(e)) {
    case MANIFEST_GATE_CHARGE_SPUNCH: return WM_BTN_SPUNCH;
    case MANIFEST_GATE_CHARGE_SKICK: return WM_BTN_SKICK;
    case MANIFEST_GATE_CHARGE_PUNCH: return WM_BTN_PUNCH;
    default: break;
    }
    assert(0 && "charge_button_for called for non-charge entry");
    return WM_BTN_PUNCH;
}

static void prepare_gate_state(const wm_arcade_smove_entry_t *e,
                               wm_arcade_actor_t *a,
                               wm_arcade_actor_t *b)
{
    switch (manifest_gate_for(e)) {
    case MANIFEST_GATE_HEADHOLD:
        a->player_mode = WM_PMODE_HEADHOLD;
        a->who_i_hit = b;
        a->smart_target = b;
        return;
    case MANIFEST_GATE_GRAB_TOSS_AIR:
        a->player_mode = WM_PMODE_NORMAL;
        a->smart_target = b;
        b->player_mode = WM_PMODE_INAIR;
        a->closest_dist = 0x20;
        return;
    case MANIFEST_GATE_CHARGE_PUNCH:
    case MANIFEST_GATE_CHARGE_SPUNCH:
    case MANIFEST_GATE_CHARGE_SKICK:
    case MANIFEST_GATE_NORMAL:
    default:
        a->player_mode = WM_PMODE_NORMAL;
        a->smart_target = b;
        return;
    }
}

static void drive_expected_step(wm_arcade_actor_t *a, uint16_t expected)
{
    a->but_val_down = (uint16_t)((expected >> 4) & WM_BTN_ATTACK_MASK);
    a->stick_rel_new = (uint16_t)(expected & 0x000fu);
}

static void drive_steps(const wm_arcade_smove_entry_t *entry,
                        wm_arcade_actor_t *a,
                        wm_arcade_actor_t *b,
                        const wm_arcade_smove_callbacks_t *cb,
                        wm_arcade_smove_runtime_t *rt)
{
    wm_arcade_actor_t *actors[2];
    actors[0] = a;
    actors[1] = b;
    single_proc(rt, entry);
    for (uint8_t i = 0u; i < entry->step_count; ++i) {
        drive_expected_step(a, entry->steps[i].expected);
        wm_arcade_smove_runtime_tick(rt, actors, 2u, cb);
    }
    a->but_val_down = 0u;
    a->stick_rel_new = 0u;
}

static void drive_charge(const wm_arcade_smove_entry_t *entry,
                         wm_arcade_actor_t *a,
                         wm_arcade_actor_t *b,
                         const wm_arcade_smove_callbacks_t *cb,
                         wm_arcade_smove_runtime_t *rt)
{
    wm_arcade_actor_t *actors[2];
    uint16_t button = charge_button_for(entry);
    actors[0] = a;
    actors[1] = b;
    single_proc(rt, entry);
    a->but_val_cur = button;
    for (unsigned i = 0u; i < 120u; ++i)
        wm_arcade_smove_runtime_tick(rt, actors, 2u, cb);
    a->but_val_cur = 0u;
    wm_arcade_smove_runtime_tick(rt, actors, 2u, cb);
}

static void assert_fired(const wm_arcade_smove_entry_t *entry,
                         const wm_arcade_smove_runtime_t *rt,
                         const wm_arcade_actor_t *a,
                         const ManifestProbe *probe)
{
    if (rt->fire_count != 1u || rt->proc[0].fires != 1u ||
        a->special_move_addr == (uintptr_t)0 || probe->resolve_count < 1) {
        fprintf(stderr,
                "all-manifest fire failed: wrestler=%d label=%s result=%s "
                "step_count=%u fire_count=%u proc_fires=%u special=0x%lx "
                "resolves=%d sounds=%d combos=%d find_kill=%d\n",
                (int)entry->wrestler,
                entry->process_label ? entry->process_label : "(null)",
                entry->result_label ? entry->result_label : "(null)",
                (unsigned)entry->step_count,
                (unsigned)rt->fire_count,
                (unsigned)rt->proc[0].fires,
                (unsigned long)a->special_move_addr,
                probe->resolve_count,
                probe->sound_count,
                probe->combo_checks,
                probe->find_kill_count);
        assert(rt->fire_count == 1u);
    }
}

static void fire_one_manifest_entry(const wm_arcade_smove_entry_t *entry)
{
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    ManifestProbe probe;
    wm_arcade_smove_callbacks_t cb;

    assert(entry != 0);
    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, entry->wrestler, 0);
    actor_init(&b, WM_ROSTER_BRET, 1);
    if (entry->wrestler == WM_ROSTER_BRET)
        b.wrestler_num = WM_ROSTER_RAZOR;
    prepare_gate_state(entry, &a, &b);

    if (is_charge_entry(entry))
        drive_charge(entry, &a, &b, &cb, &rt);
    else
        drive_steps(entry, &a, &b, &cb, &rt);

    assert_fired(entry, &rt, &a, &probe);
}

static void test_all_manifest_entries_are_live_fireable(void)
{
    size_t count = wm_arcade_smove_manifest_count();
    assert(count == 63u);
    for (size_t i = 0u; i < count; ++i)
        fire_one_manifest_entry(wm_arcade_smove_manifest_entry(i));
}

static void test_all_runtime_entries_remain_source_exact(void)
{
    size_t count = wm_arcade_smove_manifest_count();
    for (size_t i = 0u; i < count; ++i) {
        const wm_arcade_smove_entry_t *e = wm_arcade_smove_manifest_entry(i);
        assert(e != 0);
        assert(e->process_label != 0);
        assert(e->result_label != 0);
        assert(e->source_exact_body == 1u);
        if (e->step_count != 0u)
            assert(e->steps != 0);
    }
}

int main(void)
{
    test_all_runtime_entries_remain_source_exact();
    test_all_manifest_entries_are_live_fireable();
    puts("Combat2ES runtime all-manifest regression: PASS");
    return 0;
}
