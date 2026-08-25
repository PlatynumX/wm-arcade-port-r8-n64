#include "wm_arcade_smove_runtime.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TOKEN_BASE ((uintptr_t)0x62000000u)

typedef struct SchedulerProbe {
    int resolve_count;
    int sound_count;
    int combo_value;
    int combo_checks;
    int find_kill_count;
    int reversal_count;
    int reversal_message_count;
    int bonus_count;
    int ck_ignore_count;
    int ck_ignore_value;
    const char *last_resolved;
    const char *last_sound;
} SchedulerProbe;

static uintptr_t resolve_cb(const char *source_label, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(source_label != 0);
    ++p->resolve_count;
    p->last_resolved = source_label;
    return TOKEN_BASE + (uintptr_t)p->resolve_count;
}

static void sound_cb(wm_arcade_actor_t *actor, const char *source_label, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(actor != 0);
    assert(source_label != 0);
    ++p->sound_count;
    p->last_sound = source_label;
}

static int combo_cb(wm_arcade_actor_t *actor, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(actor != 0);
    ++p->combo_checks;
    return p->combo_value;
}

static void find_kill_cb(wm_arcade_actor_t *actor, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(actor != 0);
    ++p->find_kill_count;
}

static void reversal_cb(wm_arcade_actor_t *actor, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(actor != 0);
    ++p->reversal_count;
}

static void reversal_message_cb(wm_arcade_actor_t *actor, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(actor != 0);
    ++p->reversal_message_count;
}

static void bonus_cb(wm_arcade_actor_t *actor, int bonus, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(actor != 0);
    (void)bonus;
    ++p->bonus_count;
}

static int ck_ignore_cb(wm_arcade_actor_t *actor, void *user)
{
    SchedulerProbe *p = (SchedulerProbe *)user;
    assert(actor != 0);
    ++p->ck_ignore_count;
    return p->ck_ignore_value;
}

static wm_arcade_smove_callbacks_t callbacks(SchedulerProbe *probe)
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

static void test_busy_special_move_addr_rewinds_without_fire(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_BRET, "hrt_roll_uppercut");
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    SchedulerProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    actors[0] = &a;
    actors[1] = &b;
    single_proc(&rt, entry);

    rt.proc[0].step_index = 2u;
    rt.proc[0].timeout = 33u;
    a.special_move_addr = (uintptr_t)0x12345678u;

    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);

    assert(rt.fire_count == 0u);
    assert(rt.reset_count == 1u);
    assert(rt.proc[0].step_index == 0u);
    assert(rt.proc[0].sleep_ticks == 1u);
    assert(rt.proc[0].timeout == 0u);
    assert(probe.resolve_count == 0);
    assert(probe.sound_count == 0);
}

static void test_combo_rejection_rewinds_without_queueing(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_BRET, "hrt_hdhold_combo1");
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    SchedulerProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = -1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    a.player_mode = WM_PMODE_HEADHOLD;
    a.who_i_hit = &b;
    a.smart_target = &b;

    drive_monitor(entry, &a, &b, &cb, &rt);

    assert(rt.fire_count == 0u);
    assert(rt.reset_count == 1u);
    assert(a.special_move_addr == (uintptr_t)0);
    assert(probe.combo_checks == 1);
    assert(probe.resolve_count == 0);
    assert(probe.find_kill_count == 0);
}

static void test_reset_and_kill_are_owner_scoped(void)
{
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t bret, razor;
    const wm_arcade_wrestler_profile_t *bret_profile;
    const wm_arcade_wrestler_profile_t *razor_profile;

    actor_init(&bret, WM_ROSTER_BRET, 0);
    actor_init(&razor, WM_ROSTER_RAZOR, 1);
    bret_profile = wm_arcade_roster_profile(WM_ROSTER_BRET);
    razor_profile = wm_arcade_roster_profile(WM_ROSTER_RAZOR);
    assert(bret_profile != 0);
    assert(razor_profile != 0);

    wm_arcade_smove_runtime_init(&rt);
    assert(wm_arcade_smove_init_for_wrestler(&rt, &bret, 0u, bret_profile) == 9u);
    assert(wm_arcade_smove_init_for_wrestler(&rt, &razor, 1u, razor_profile) == 8u);
    assert(rt.proc_count == 17u);

    for (size_t i = 0u; i < rt.proc_count; ++i) {
        rt.proc[i].step_index = 2u;
        rt.proc[i].timeout = 44u;
        rt.proc[i].sleep_ticks = 0u;
    }

    wm_arcade_smove_reset_for_wrestler(&rt, &bret);
    assert(rt.reset_count == 9u);
    for (size_t i = 0u; i < 9u; ++i) {
        assert(rt.proc[i].owner_slot == 0u);
        assert(rt.proc[i].step_index == 0u);
        assert(rt.proc[i].sleep_ticks == 1u);
        assert(rt.proc[i].timeout == 0u);
        assert(rt.proc[i].active == 1u);
    }
    for (size_t i = 9u; i < 17u; ++i) {
        assert(rt.proc[i].owner_slot == 1u);
        assert(rt.proc[i].step_index == 2u);
        assert(rt.proc[i].timeout == 44u);
        assert(rt.proc[i].active == 1u);
    }

    wm_arcade_smove_kill_for_wrestler(&rt, &razor);
    assert(rt.kill_count == 8u);
    for (size_t i = 0u; i < 9u; ++i)
        assert(rt.proc[i].active == 1u);
    for (size_t i = 9u; i < 17u; ++i)
        assert(rt.proc[i].active == 0u);
}

static void test_ck_ignore_blocks_charged_flying_kick_release(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_BRET, "hrt_charge_flying_kick");
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    SchedulerProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    probe.ck_ignore_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    a.smart_target = &b;
    actors[0] = &a;
    actors[1] = &b;
    single_proc(&rt, entry);

    a.but_val_cur = WM_BTN_SKICK;
    for (unsigned i = 0u; i < 100u; ++i)
        wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    a.but_val_cur = 0u;
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);

    assert(rt.fire_count == 0u);
    assert(rt.reset_count == 0u);
    assert(a.special_move_addr == (uintptr_t)0);
    assert(probe.ck_ignore_count == 1);
    assert(probe.resolve_count == 0);
    assert(probe.sound_count == 0);
}

int main(void)
{
    test_busy_special_move_addr_rewinds_without_fire();
    test_combo_rejection_rewinds_without_queueing();
    test_reset_and_kill_are_owner_scoped();
    test_ck_ignore_blocks_charged_flying_kick_release();
    puts("Combat2ES runtime scheduler regression: PASS");
    return 0;
}
