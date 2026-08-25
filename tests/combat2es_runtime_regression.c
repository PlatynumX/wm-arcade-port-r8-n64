#include "wm_arcade_smove_runtime.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TOKEN_BASE ((uintptr_t)0x51000000u)

typedef struct RuntimeProbe {
    int resolve_count;
    int sound_count;
    int combo_value;
    int combo_checks;
    int find_kill_count;
    int reversal_count;
    int reversal_message_count;
    int bonus_count;
    int bonus_value;
    const char *last_resolved;
    const char *last_sound;
} RuntimeProbe;

static uintptr_t resolve_cb(const char *source_label, void *user)
{
    RuntimeProbe *p = (RuntimeProbe *)user;
    assert(source_label != 0);
    ++p->resolve_count;
    p->last_resolved = source_label;
    return TOKEN_BASE + (uintptr_t)p->resolve_count;
}

static void sound_cb(wm_arcade_actor_t *actor, const char *source_label, void *user)
{
    RuntimeProbe *p = (RuntimeProbe *)user;
    assert(actor != 0);
    assert(source_label != 0);
    ++p->sound_count;
    p->last_sound = source_label;
}

static int combo_cb(wm_arcade_actor_t *actor, void *user)
{
    RuntimeProbe *p = (RuntimeProbe *)user;
    assert(actor != 0);
    ++p->combo_checks;
    return p->combo_value;
}

static void find_kill_cb(wm_arcade_actor_t *actor, void *user)
{
    RuntimeProbe *p = (RuntimeProbe *)user;
    assert(actor != 0);
    ++p->find_kill_count;
}

static void reversal_cb(wm_arcade_actor_t *actor, void *user)
{
    RuntimeProbe *p = (RuntimeProbe *)user;
    assert(actor != 0);
    ++p->reversal_count;
}

static void reversal_message_cb(wm_arcade_actor_t *actor, void *user)
{
    RuntimeProbe *p = (RuntimeProbe *)user;
    assert(actor != 0);
    ++p->reversal_message_count;
}

static void bonus_cb(wm_arcade_actor_t *actor, int bonus, void *user)
{
    RuntimeProbe *p = (RuntimeProbe *)user;
    assert(actor != 0);
    ++p->bonus_count;
    p->bonus_value = bonus;
}

static int ck_ignore_never(wm_arcade_actor_t *actor, void *user)
{
    (void)user;
    assert(actor != 0);
    return 0;
}

static wm_arcade_smove_callbacks_t callbacks(RuntimeProbe *probe)
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
    cb.ck_ignore = ck_ignore_never;
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
    a->closest_dist = 0;
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

static void drive_expected_step(wm_arcade_actor_t *a, uint16_t expected)
{
    a->but_val_down = (uint16_t)((expected >> 4) & WM_BTN_ATTACK_MASK);
    a->stick_rel_new = (uint16_t)(expected & 0x000fu);
}

static void drive_monitor(const wm_arcade_smove_entry_t *entry,
                          wm_arcade_actor_t *a,
                          wm_arcade_actor_t *b,
                          const wm_arcade_smove_callbacks_t *cb,
                          wm_arcade_smove_runtime_t *rt)
{
    wm_arcade_actor_t *actors[2];
    assert(entry != 0);
    assert(entry->step_count > 0u);
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

static void test_manifest_runtime_creation_counts(void)
{
    static const struct {
        wm_arcade_roster_id_t id;
        size_t count;
    } expected[] = {
        { WM_ROSTER_BRET, 9u },
        { WM_ROSTER_RAZOR, 8u },
        { WM_ROSTER_TAKER, 10u },
        { WM_ROSTER_YOKO, 6u },
        { WM_ROSTER_SHAWN, 11u },
        { WM_ROSTER_BAM, 6u },
        { WM_ROSTER_DOINK, 7u },
        { WM_ROSTER_LEX, 6u },
    };
    size_t total = 0u;

    assert(wm_arcade_smove_manifest_count() == 63u);
    for (size_t i = 0u; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        wm_arcade_smove_runtime_t rt;
        wm_arcade_actor_t actor;
        const wm_arcade_wrestler_profile_t *profile = wm_arcade_roster_profile(expected[i].id);
        assert(profile != 0);
        actor_init(&actor, expected[i].id, 0);
        wm_arcade_smove_runtime_init(&rt);
        assert(wm_arcade_smove_init_for_wrestler(&rt, &actor, 0u, profile) == expected[i].count);
        assert(rt.unresolved_created == 0u);
        assert(rt.proc_count == expected[i].count);
        for (size_t j = 0u; j < rt.proc_count; ++j) {
            assert(rt.proc[j].entry != 0);
            assert(rt.proc[j].entry->source_exact_body == 1u);
        }
        total += rt.proc_count;
    }
    assert(total == 63u);
}

static void test_waitswitch_runtime_edges(void)
{
    wm_arcade_actor_t a;
    uint16_t timeout;
    actor_init(&a, WM_ROSTER_BRET, 0);

    timeout = 0u;
    a.stick_rel_new = 0u;
    a.but_val_down = 0u;
    assert(wm_arcade_smove_waitswitch_down(&a, WM_J_TOWARD, 0u, &timeout) ==
           WM_SMOVE_WAIT_STILL_WAITING);
    assert(timeout == 0xffffu);

    timeout = 60u;
    a.stick_rel_new = WM_J_AWAY;
    a.but_val_down = 0u;
    assert(wm_arcade_smove_waitswitch_down(&a, WM_J_TOWARD, 0u, &timeout) ==
           WM_SMOVE_WAIT_RESET);
    assert(timeout == 59u);

    timeout = 60u;
    a.stick_rel_new = WM_J_DOWN;
    a.but_val_down = WM_BTN_SPUNCH;
    assert(wm_arcade_smove_waitswitch_down(&a, WM_B_SPUNCH, WM_J_ALL, &timeout) ==
           WM_SMOVE_WAIT_ADVANCED);
}

static void test_headhold_bonus_and_reversal_callbacks(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_BRET, "hrt_hdhold_pile");
    wm_arcade_smove_callbacks_t cb;
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    RuntimeProbe probe;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    a.player_mode = WM_PMODE_HEADHOLD;
    a.who_i_hit = &b;
    a.smart_target = &b;
    drive_monitor(entry, &a, &b, &cb, &rt);

    assert(rt.fire_count == 1u);
    assert(a.special_move_addr == TOKEN_BASE + 1u);
    assert(strcmp(probe.last_resolved, "hrt_3_pile_driver_anim") == 0);
    assert(b.immobilize_time == 15);
    assert(probe.bonus_count == 1);
    assert(probe.bonus_value == 35);
    assert(probe.find_kill_count == 1);
    assert(probe.reversal_count == 0);
    assert(probe.reversal_message_count == 0);
    assert(probe.sound_count == 1);

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    a.player_mode = WM_PMODE_HEADHELD;
    a.who_hit_me = &b;
    a.smart_target = &b;
    drive_monitor(entry, &a, &b, &cb, &rt);

    assert(rt.fire_count == 1u);
    assert(a.special_move_addr == TOKEN_BASE + 1u);
    assert(b.immobilize_time == 15);
    assert(probe.bonus_count == 0);
    assert(probe.reversal_count == 1);
    assert(probe.reversal_message_count == 1);
    assert(probe.find_kill_count == 1);
}

static void test_charge_release_runtime_threshold(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_RAZOR, "rzr_charge_slashes");
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    RuntimeProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_RAZOR, 0);
    actor_init(&b, WM_ROSTER_BRET, 1);
    actors[0] = &a;
    actors[1] = &b;
    single_proc(&rt, entry);

    a.but_val_cur = WM_BTN_PUNCH;
    for (unsigned i = 0u; i < 99u; ++i)
        wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    a.but_val_cur = 0u;
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(rt.fire_count == 0u);
    assert(a.special_move_addr == (uintptr_t)0);

    single_proc(&rt, entry);
    a.but_val_cur = WM_BTN_PUNCH;
    for (unsigned i = 0u; i < 100u; ++i)
        wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    a.but_val_cur = 0u;
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(rt.fire_count == 1u);
    assert(a.special_move_addr == TOKEN_BASE + 1u);
    assert(strcmp(probe.last_resolved, "rzr_repeat_slash_anim") == 0);
    assert(probe.sound_count == 1);
    assert(strcmp(probe.last_sound, "KICK_T2") == 0);
}

static void test_grab_toss_air_runtime_direct_resolver(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_LEX, "lex_grab_toss_air");
    wm_arcade_smove_callbacks_t cb;
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    RuntimeProbe probe;

    memset(&probe, 0, sizeof(probe));
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_LEX, 0);
    actor_init(&b, WM_ROSTER_BRET, 1);
    a.smart_target = &b;
    a.closest_dist = 0x20;
    b.player_mode = WM_PMODE_INAIR;

    drive_monitor(entry, &a, &b, &cb, &rt);
    assert(rt.fire_count == 1u);
    assert(a.special_move_addr == TOKEN_BASE + 1u);
    assert(strcmp(probe.last_resolved, "lex_hiptoss2_anim") == 0);
    assert(probe.sound_count == 1);
    assert(strcmp(probe.last_sound, "GRABFLING_T1/PUNCH_T2") == 0);
}

int main(void)
{
    test_manifest_runtime_creation_counts();
    test_waitswitch_runtime_edges();
    test_headhold_bonus_and_reversal_callbacks();
    test_charge_release_runtime_threshold();
    test_grab_toss_air_runtime_direct_resolver();
    puts("Combat2ES runtime regression: PASS");
    return 0;
}
