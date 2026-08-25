#include "wm_arcade_smove_runtime.h"

#include <string.h>

#define STEP(e,i,l) {(uint16_t)(e),(uint16_t)(i),(uint16_t)(l)}

enum {
    G_NONE = 0,
    G_TAKER_HD_NECK,
    G_TAKER_HD_FACESLAM,
    G_TAKER_HD_PILE,
    G_TAKER_CHOKE_SLIDE,
    G_TAKER_SPIRIT_PUSH,
    G_TAKER_SPIRIT_PULL,
    G_TAKER_GRAB_TOSS_AIR,
    G_TAKER_COMBO1,
    G_TAKER_COMBO2,
    G_TAKER_FINISH1
};

static const wm_arcade_smove_wait_step_t und_hd_neck[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SPUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_hd_faceslam[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_hd_pile[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_choke_slide[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_UP | WM_J_DOWN, 60),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_spirit_push[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, WM_J_UP | WM_J_DOWN, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_spirit_pull[] = {
    STEP(WM_J_DOWN, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, WM_J_UP | WM_J_DOWN, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_grab_toss_air[] = {
    STEP(WM_J_AWAY, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_AWAY, 0, 40),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_combo1[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_SKICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_combo2[] = {
    STEP(WM_J_TOWARD, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_TOWARD, 0, 60),
    STEP(WM_B_KICK, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};
static const wm_arcade_smove_wait_step_t und_finish1[] = {
    STEP(WM_J_UP, 0, WM_ARCADE_SMOVE_TIMEOUT_KEEP),
    STEP(WM_J_DOWN, 0, 53),
    STEP(WM_B_PUNCH, WM_J_ALL, WM_ARCADE_SMOVE_TIMEOUT_KEEP)
};

static const wm_arcade_smove_entry_t manifest[] = {
    { WM_ROSTER_TAKER, "und_hdhold_neckbrk", "und_neckbreaker_anim", und_hd_neck, 3, G_TAKER_HD_NECK, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_faceslam", "und_choke_face_slam_anim", und_hd_faceslam, 3, G_TAKER_HD_FACESLAM, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_pile", "und_pile_anim", und_hd_pile, 3, G_TAKER_HD_PILE, 20, 1 },
    { WM_ROSTER_TAKER, "und_choke_slide", "und_sliding_choke_anim", und_choke_slide, 3, G_TAKER_CHOKE_SLIDE, 20, 1 },
    { WM_ROSTER_TAKER, "und_spirit_push", "und_spirit_push_anim", und_spirit_push, 3, G_TAKER_SPIRIT_PUSH, 180, 1 },
    { WM_ROSTER_TAKER, "und_spirit_pull", "und_spirit_pull_anim", und_spirit_pull, 3, G_TAKER_SPIRIT_PULL, 180, 1 },
    { WM_ROSTER_TAKER, "und_grab_toss_air", "und_2_snapmirror_anim", und_grab_toss_air, 3, G_TAKER_GRAB_TOSS_AIR, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_combo1", "und_combo_punch_anim", und_combo1, 3, G_TAKER_COMBO1, 20, 1 },
    { WM_ROSTER_TAKER, "und_hdhold_combo2", "und_combo_kick_anim", und_combo2, 3, G_TAKER_COMBO2, 20, 1 },
    { WM_ROSTER_TAKER, "und_finish_move1", "und_finish_move1", und_finish1, 3, G_TAKER_FINISH1, 1, 1 }
};

size_t wm_arcade_smove_manifest_count(void)
{
    return sizeof manifest / sizeof manifest[0];
}

const wm_arcade_smove_entry_t *wm_arcade_smove_manifest_entry(size_t index)
{
    return index < wm_arcade_smove_manifest_count() ? &manifest[index] : 0;
}

int wm_arcade_smove_label_source_enabled(wm_arcade_roster_id_t wrestler,
                                         const char *label)
{
    if (!label) return 0;
    if (strstr(label, "finish_move") != 0) {
        return wrestler == WM_ROSTER_TAKER && strcmp(label, "und_finish_move1") == 0;
    }
    return 1;
}

const wm_arcade_smove_entry_t *wm_arcade_smove_lookup_entry(
    wm_arcade_roster_id_t wrestler,
    const char *process_label)
{
    size_t i;
    if (!process_label) return 0;
    for (i = 0; i < wm_arcade_smove_manifest_count(); ++i) {
        if (manifest[i].wrestler == wrestler &&
            strcmp(manifest[i].process_label, process_label) == 0)
            return &manifest[i];
    }
    return 0;
}

void wm_arcade_smove_runtime_init(wm_arcade_smove_runtime_t *rt)
{
    if (rt) memset(rt, 0, sizeof(*rt));
}

static void proc_rewind(wm_arcade_smove_runtime_t *rt, wm_arcade_smove_proc_t *p,
                        uint8_t sleep_ticks)
{
    if (!p) return;
    p->step_index = 0;
    p->timeout = 0;
    p->timeout_loaded_for = 0xffu;
    p->sleep_ticks = sleep_ticks;
    if (rt) ++rt->reset_count;
}

size_t wm_arcade_smove_init_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    wm_arcade_actor_t *owner,
    uint8_t owner_slot,
    const wm_arcade_wrestler_profile_t *profile)
{
    size_t i, made = 0;
    if (!rt || !owner || !profile) return 0;
    for (i = 0; i < profile->special_process_count; ++i) {
        const char *label = profile->special_processes[i];
        wm_arcade_smove_proc_t *p;
        if (!wm_arcade_smove_label_source_enabled(profile->id, label))
            continue;
        if (rt->proc_count >= WM_ARCADE_SMOVE_MAX_PROCS)
            break;
        p = &rt->proc[rt->proc_count++];
        memset(p, 0, sizeof(*p));
        p->active = 1;
        p->owner_slot = owner_slot;
        p->profile = profile;
        p->process_label = label;
        p->entry = wm_arcade_smove_lookup_entry(profile->id, label);
        p->unresolved = p->entry == 0 || p->entry->source_exact_body == 0;
        proc_rewind(0, p, 1);
        ++made;
        ++rt->created;
        if (p->unresolved) ++rt->unresolved_created;
    }
    (void)owner;
    return made;
}

void wm_arcade_smove_reset_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    const wm_arcade_actor_t *owner)
{
    size_t i;
    if (!rt || !owner) return;
    for (i = 0; i < rt->proc_count; ++i) {
        wm_arcade_smove_proc_t *p = &rt->proc[i];
        if (!p->active || p->owner_slot != (uint8_t)owner->player_num) continue;
        proc_rewind(rt, p, 1);
    }
}

void wm_arcade_smove_kill_for_wrestler(
    wm_arcade_smove_runtime_t *rt,
    const wm_arcade_actor_t *owner)
{
    size_t i;
    if (!rt || !owner) return;
    for (i = 0; i < rt->proc_count; ++i) {
        wm_arcade_smove_proc_t *p = &rt->proc[i];
        if (!p->active || p->owner_slot != (uint8_t)owner->player_num) continue;
        p->active = 0;
        ++rt->kill_count;
    }
}

wm_arcade_smove_wait_result_t wm_arcade_smove_waitswitch_down(
    const wm_arcade_actor_t *actor,
    uint16_t expected,
    uint16_t ignore_mask,
    uint16_t *timeout_io)
{
    uint16_t timeout;
    uint16_t v;
    if (!actor || !timeout_io) return WM_SMOVE_WAIT_RESET;

    timeout = (uint16_t)(*timeout_io - 1u);
    *timeout_io = timeout;
    if (timeout == 0u) return WM_SMOVE_WAIT_RESET;
    if (actor->special_move_addr != (uintptr_t)0) return WM_SMOVE_WAIT_RESET;

    /* MACROS.H::WAITSWITCH_DWN uses raw (BUT_VAL_DOWN << 4) |
       STICK_REL_NEW, then andni MASK.  Do not pre-mask here. */
    v = (uint16_t)((actor->but_val_down << 4) | actor->stick_rel_new);
    v = (uint16_t)(v & (uint16_t)~ignore_mask);
    if (v == 0u) return WM_SMOVE_WAIT_STILL_WAITING;
    return v == expected ? WM_SMOVE_WAIT_ADVANCED : WM_SMOVE_WAIT_RESET;
}

static wm_arcade_actor_t *opponent_for(wm_arcade_actor_t **actors, size_t n,
                                       wm_arcade_actor_t *a)
{
    size_t i;
    if (!actors || !a) return 0;
    if (a->smart_target) return a->smart_target;
    for (i = 0; i < n; ++i) {
        if (actors[i] && actors[i] != a && actors[i]->active &&
            actors[i]->player_side != a->player_side)
            return actors[i];
    }
    return 0;
}

static int mode_is_headhold(uint16_t mode) { return mode == WM_PMODE_HEADHOLD; }
static int mode_is_headheld(uint16_t mode) { return mode == WM_PMODE_HEADHELD; }
static int mode_is_chokehold(uint16_t mode) { return mode == WM_PMODE_CHOKEHOLD; }
static int mode_is_ground_dead(uint16_t mode) { return mode == WM_PMODE_ONGROUND || mode == WM_PMODE_DEAD; }
static int mode_is_inair(uint16_t mode) { return mode == WM_PMODE_INAIR || mode == WM_PMODE_INAIR2; }

static void queue_result(wm_arcade_actor_t *a, const wm_arcade_smove_entry_t *e,
                         const wm_arcade_smove_callbacks_t *cb)
{
    uintptr_t tok = (uintptr_t)e->result_label;
    if (cb && cb->resolve_label_token)
        tok = cb->resolve_label_token(e->result_label, cb->user);
    a->special_move_addr = tok;
}

static int fire_taker_headhold(wm_arcade_actor_t *a, wm_arcade_actor_t *opp,
                               const wm_arcade_smove_entry_t *e,
                               const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *target = 0;
    if (!a) return 0;
    if (mode_is_headheld(a->player_mode)) {
        if (a->i_will_die || a->immobilize_time != 0) return 0;
        target = a->who_hit_me ? a->who_hit_me : opp;
    } else if (mode_is_headhold(a->player_mode) ||
               (e->gate_kind == G_TAKER_HD_FACESLAM && mode_is_chokehold(a->player_mode))) {
        if (a->immobilize_time != 0) return 0;
        target = a->who_i_hit ? a->who_i_hit : opp;
    } else {
        return 0;
    }
    if (!target) return 0;
    target->immobilize_time = 15;
    a->smart_target = target;
    if (cb && cb->find_and_kill_endless)
        cb->find_and_kill_endless(a, cb->user);
    queue_result(a, e, cb);
    if (cb && cb->sound_label) cb->sound_label(a, "GRABHOLD_T1/GRABHOLD_T2", cb->user);
    return 1;
}

static int fire_entry(wm_arcade_actor_t **actors, size_t n,
                      wm_arcade_actor_t *a,
                      const wm_arcade_smove_entry_t *e,
                      const wm_arcade_smove_callbacks_t *cb)
{
    wm_arcade_actor_t *opp = opponent_for(actors, n, a);
    if (!a || !e) return 0;
    switch (e->gate_kind) {
    case G_TAKER_HD_NECK:
    case G_TAKER_HD_FACESLAM:
    case G_TAKER_HD_PILE:
        return fire_taker_headhold(a, opp, e, cb);
    case G_TAKER_CHOKE_SLIDE:
        if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
        if (a->player_mode == WM_PMODE_ONTURNBKL || a->player_mode == WM_PMODE_RUNNING) return 0;
        if (a->i_will_die) return 0;
        if (opp && (mode_is_ground_dead(opp->player_mode) ||
                    opp->player_mode == WM_PMODE_HEADHELD ||
                    opp->player_mode == WM_PMODE_CHOKING)) return 0;
        queue_result(a, e, cb);
        return 1;
    case G_TAKER_SPIRIT_PUSH:
    case G_TAKER_SPIRIT_PULL:
        if (a->player_mode == WM_PMODE_HEADHOLD || a->player_mode == WM_PMODE_HEADHELD) return 0;
        if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
        if (a->immobilize_time != 0) return 0;
        if (opp && (opp->player_mode == WM_PMODE_CHOKING || opp->player_mode == WM_PMODE_HEADHELD)) return 0;
        a->run_time = 0;
        if (a->player_mode != WM_PMODE_DEAD) a->player_mode = WM_PMODE_NORMAL;
        queue_result(a, e, cb);
        if (cb && cb->sound_label) cb->sound_label(a, "SPIRIT", cb->user);
        return 1;
    case G_TAKER_GRAB_TOSS_AIR:
        if ((a->anim_mode & WM_ARCADE_MODE_UNINT) != 0u) return 0;
        if (a->player_mode == WM_PMODE_HEADHOLD) return 0;
        if (opp && mode_is_ground_dead(opp->player_mode)) return 0;
        if (opp && mode_is_inair(opp->player_mode)) {
            /* Source FACE24 snapmirror2: pick side using facing. */
            a->special_move_addr = (uintptr_t)(((a->facing_dir & WM_MOVE_RIGHT) != 0u) ?
                "und_2_snapmirror2_anim" : "und_4_snapmirror2_anim");
        } else {
            if (a->closest_dist > 0x68) return 0;
            a->special_move_addr = (uintptr_t)(((a->facing_dir & WM_MOVE_RIGHT) != 0u) ?
                "und_2_snapmirror_anim" : "und_4_snapmirror_anim");
        }
        if (cb && cb->resolve_label_token)
            a->special_move_addr = cb->resolve_label_token((const char *)a->special_move_addr, cb->user);
        if (cb && cb->sound_label) cb->sound_label(a, "HIPTOSS_T1/HIPTOSS_T2", cb->user);
        a->attach_proc = 0;
        if (a->player_mode != WM_PMODE_DEAD) a->player_mode = WM_PMODE_NORMAL;
        return 1;
    case G_TAKER_COMBO1:
    case G_TAKER_COMBO2:
        if (a->player_mode != WM_PMODE_HEADHOLD) return 0;
        if (cb && cb->check_combo_go && cb->check_combo_go(a, cb->user) < 0) return 0;
        if (a->immobilize_time != 0) return 0;
        a->smart_target = a->who_i_hit ? a->who_i_hit : opp;
        if (cb && cb->find_and_kill_endless)
            cb->find_and_kill_endless(a, cb->user);
        queue_result(a, e, cb);
        return 1;
    case G_TAKER_FINISH1:
        queue_result(a, e, cb);
        return 1;
    default:
        return 0;
    }
}

void wm_arcade_smove_runtime_tick(
    wm_arcade_smove_runtime_t *rt,
    wm_arcade_actor_t **actors,
    size_t actor_count,
    const wm_arcade_smove_callbacks_t *callbacks)
{
    size_t i;
    if (!rt || !actors) return;
    for (i = 0; i < rt->proc_count; ++i) {
        wm_arcade_smove_proc_t *p = &rt->proc[i];
        wm_arcade_actor_t *a;
        const wm_arcade_smove_wait_step_t *s;
        wm_arcade_smove_wait_result_t wr;
        if (!p->active || p->unresolved || !p->entry) continue;
        if (p->owner_slot >= actor_count) continue;
        a = actors[p->owner_slot];
        if (!a || !a->active) continue;
        if (p->sleep_ticks != 0u) { --p->sleep_ticks; continue; }
        if (a->special_move_addr != (uintptr_t)0) { proc_rewind(rt, p, 1); continue; }
        if (p->step_index >= p->entry->step_count) { proc_rewind(rt, p, 1); continue; }
        s = &p->entry->steps[p->step_index];
        if (s->load_timeout != WM_ARCADE_SMOVE_TIMEOUT_KEEP &&
            p->timeout_loaded_for != p->step_index) {
            p->timeout = s->load_timeout;
            p->timeout_loaded_for = p->step_index;
        }
        wr = wm_arcade_smove_waitswitch_down(a, s->expected, s->ignore_mask, &p->timeout);
        if (wr == WM_SMOVE_WAIT_STILL_WAITING) continue;
        if (wr == WM_SMOVE_WAIT_RESET) { proc_rewind(rt, p, 1); continue; }
        ++p->step_index;
        p->timeout_loaded_for = 0xffu;
        if (p->step_index < p->entry->step_count) continue;
        if (fire_entry(actors, actor_count, a, p->entry, callbacks)) {
            ++p->fires;
            ++rt->fire_count;
            proc_rewind(0, p, p->entry->post_fire_sleep);
        } else {
            proc_rewind(rt, p, 1);
        }
    }
}
