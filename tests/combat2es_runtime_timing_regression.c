#include "wm_arcade_smove_runtime.h"
#include "wm_arcade_roster.h"
#include "wm_arcade_combat_defs.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TOKEN_BASE ((uintptr_t)0x73000000u)

typedef struct TimingProbe {
    int resolve_count;
    int sound_count;
    int combo_value;
    int combo_checks;
    int find_kill_count;
    int ck_ignore_count;
    int ck_ignore_value;
    const char *last_resolved;
    const char *last_sound;
} TimingProbe;

static uintptr_t resolve_cb(const char *source_label, void *user)
{
    TimingProbe *p = (TimingProbe *)user;
    assert(source_label != 0);
    ++p->resolve_count;
    p->last_resolved = source_label;
    return TOKEN_BASE + (uintptr_t)p->resolve_count;
}

static void sound_cb(wm_arcade_actor_t *actor, const char *source_label, void *user)
{
    TimingProbe *p = (TimingProbe *)user;
    assert(actor != 0);
    assert(source_label != 0);
    ++p->sound_count;
    p->last_sound = source_label;
}

static int combo_cb(wm_arcade_actor_t *actor, void *user)
{
    TimingProbe *p = (TimingProbe *)user;
    assert(actor != 0);
    ++p->combo_checks;
    return p->combo_value;
}

static void find_kill_cb(wm_arcade_actor_t *actor, void *user)
{
    TimingProbe *p = (TimingProbe *)user;
    assert(actor != 0);
    ++p->find_kill_count;
}

static void reversal_cb(wm_arcade_actor_t *actor, void *user)
{
    (void)user;
    assert(actor != 0);
}

static void reversal_message_cb(wm_arcade_actor_t *actor, void *user)
{
    (void)user;
    assert(actor != 0);
}

static void bonus_cb(wm_arcade_actor_t *actor, int bonus, void *user)
{
    (void)user;
    (void)bonus;
    assert(actor != 0);
}

static int ck_ignore_cb(wm_arcade_actor_t *actor, void *user)
{
    TimingProbe *p = (TimingProbe *)user;
    assert(actor != 0);
    ++p->ck_ignore_count;
    return p->ck_ignore_value;
}

static wm_arcade_smove_callbacks_t callbacks(TimingProbe *probe)
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

static wm_arcade_smove_proc_t *find_proc(wm_arcade_smove_runtime_t *rt,
                                         const char *label)
{
    for (size_t i = 0u; i < rt->proc_count; ++i) {
        if (rt->proc[i].process_label && strcmp(rt->proc[i].process_label, label) == 0)
            return &rt->proc[i];
    }
    return 0;
}

static void drive_expected_step(wm_arcade_actor_t *a, uint16_t expected)
{
    a->but_val_down = (uint16_t)((expected >> 4) & WM_BTN_ATTACK_MASK);
    a->stick_rel_new = (uint16_t)(expected & 0x000fu);
}

static void test_created_processes_skip_first_tick_then_accept_input(void)
{
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    const wm_arcade_wrestler_profile_t *profile;
    wm_arcade_smove_proc_t *roll;
    TimingProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    actors[0] = &a;
    actors[1] = &b;
    profile = wm_arcade_roster_profile(WM_ROSTER_BRET);
    assert(profile != 0);

    wm_arcade_smove_runtime_init(&rt);
    assert(wm_arcade_smove_init_for_wrestler(&rt, &a, 0u, profile) == 9u);
    roll = find_proc(&rt, "hrt_roll_uppercut");
    assert(roll != 0);
    assert(roll->sleep_ticks == 1u);

    drive_expected_step(&a, WM_J_DOWN);
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(roll->sleep_ticks == 0u);
    assert(roll->step_index == 0u);
    assert(rt.fire_count == 0u);

    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(roll->step_index == 1u);
    assert(rt.fire_count == 0u);
}

static void test_wrong_input_and_timed_timeout_rewind(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_BRET, "hrt_roll_uppercut");
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    TimingProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    actors[0] = &a;
    actors[1] = &b;

    single_proc(&rt, entry);
    drive_expected_step(&a, entry->steps[0].expected);
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(rt.proc[0].step_index == 1u);

    a.but_val_down = 0u;
    a.stick_rel_new = WM_J_AWAY;
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(rt.reset_count == 1u);
    assert(rt.proc[0].step_index == 0u);
    assert(rt.proc[0].sleep_ticks == 1u);
    assert(rt.proc[0].timeout == 0u);
    assert(a.special_move_addr == (uintptr_t)0);

    single_proc(&rt, entry);
    rt.proc[0].step_index = 1u;
    rt.proc[0].timeout_loaded_for = 1u;
    rt.proc[0].timeout = 1u;
    a.but_val_down = 0u;
    a.stick_rel_new = 0u;
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(rt.reset_count == 1u);
    assert(rt.proc[0].step_index == 0u);
    assert(rt.proc[0].sleep_ticks == 1u);
}

static void test_fire_rewinds_with_source_post_fire_sleep(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_BRET, "hrt_roll_uppercut");
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    TimingProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    a.smart_target = &b;
    actors[0] = &a;
    actors[1] = &b;
    single_proc(&rt, entry);

    for (uint8_t i = 0u; i < entry->step_count; ++i) {
        drive_expected_step(&a, entry->steps[i].expected);
        wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    }

    assert(rt.fire_count == 1u);
    assert(rt.reset_count == 0u);
    assert(rt.proc[0].step_index == 0u);
    assert(rt.proc[0].sleep_ticks == entry->post_fire_sleep);
    assert(rt.proc[0].fires == 1u);
    assert(a.special_move_addr == TOKEN_BASE + 1u);
    assert(strcmp(probe.last_resolved, "hrt_roll_uppercut_anim") == 0);
    assert(probe.sound_count == 1);
}

static void test_manifest_special_sleeps_and_finish_filters(void)
{
    const wm_arcade_smove_entry_t *yoko_salt =
        wm_arcade_smove_lookup_entry(WM_ROSTER_YOKO, "yok_salt_throw");
    const wm_arcade_smove_entry_t *spirit_push =
        wm_arcade_smove_lookup_entry(WM_ROSTER_TAKER, "und_spirit_push");
    const wm_arcade_smove_entry_t *taker_finish =
        wm_arcade_smove_lookup_entry(WM_ROSTER_TAKER, "und_finish_move1");

    assert(yoko_salt != 0);
    assert(yoko_salt->post_fire_sleep == 120u);
    assert(spirit_push != 0);
    assert(spirit_push->post_fire_sleep == 180u);
    assert(taker_finish != 0);
    assert(taker_finish->post_fire_sleep == 1u);

    assert(wm_arcade_smove_label_source_enabled(WM_ROSTER_TAKER, "und_finish_move1"));
    assert(!wm_arcade_smove_label_source_enabled(WM_ROSTER_TAKER, "und_finish_move2"));
    assert(!wm_arcade_smove_label_source_enabled(WM_ROSTER_BRET, "hrt_finish_move1"));
    assert(!wm_arcade_smove_label_source_enabled(WM_ROSTER_RAZOR, "rzr_finish_move1"));
}

static void test_all_wrestlers_fit_one_source_manifest_runtime(void)
{
    static const wm_arcade_roster_id_t ids[8] = {
        WM_ROSTER_BRET, WM_ROSTER_RAZOR, WM_ROSTER_TAKER, WM_ROSTER_YOKO,
        WM_ROSTER_SHAWN, WM_ROSTER_BAM, WM_ROSTER_DOINK, WM_ROSTER_LEX
    };
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t actors[8];
    size_t total = 0u;

    wm_arcade_smove_runtime_init(&rt);
    for (size_t i = 0u; i < 8u; ++i) {
        const wm_arcade_wrestler_profile_t *profile = wm_arcade_roster_profile(ids[i]);
        assert(profile != 0);
        actor_init(&actors[i], ids[i], (int)i);
        total += wm_arcade_smove_init_for_wrestler(&rt, &actors[i], (uint8_t)i, profile);
    }

    assert(total == 63u);
    assert(rt.proc_count == 63u);
    assert(rt.proc_count < WM_ARCADE_SMOVE_MAX_PROCS);
    assert(rt.unresolved_created == 0u);
    for (size_t i = 0u; i < rt.proc_count; ++i) {
        assert(rt.proc[i].entry != 0);
        assert(rt.proc[i].entry->source_exact_body == 1u);
        assert(rt.proc[i].active == 1u);
    }
}

static void test_inactive_owner_slot_is_skipped(void)
{
    const wm_arcade_smove_entry_t *entry =
        wm_arcade_smove_lookup_entry(WM_ROSTER_BRET, "hrt_roll_uppercut");
    wm_arcade_smove_runtime_t rt;
    wm_arcade_actor_t a, b;
    wm_arcade_actor_t *actors[2];
    TimingProbe probe;
    wm_arcade_smove_callbacks_t cb;

    memset(&probe, 0, sizeof(probe));
    probe.combo_value = 1;
    cb = callbacks(&probe);
    actor_init(&a, WM_ROSTER_BRET, 0);
    actor_init(&b, WM_ROSTER_RAZOR, 1);
    actors[0] = &a;
    actors[1] = &b;
    single_proc(&rt, entry);
    rt.proc[0].owner_slot = 1u;
    b.active = 0;

    drive_expected_step(&b, entry->steps[0].expected);
    wm_arcade_smove_runtime_tick(&rt, actors, 2u, &cb);
    assert(rt.proc[0].step_index == 0u);
    assert(rt.fire_count == 0u);
    assert(rt.reset_count == 0u);
    assert(probe.resolve_count == 0);
}

int main(void)
{
    test_created_processes_skip_first_tick_then_accept_input();
    test_wrong_input_and_timed_timeout_rewind();
    test_fire_rewinds_with_source_post_fire_sleep();
    test_manifest_special_sleeps_and_finish_filters();
    test_all_wrestlers_fit_one_source_manifest_runtime();
    test_inactive_owner_slot_is_skipped();
    puts("Combat2ES runtime timing regression: PASS");
    return 0;
}
