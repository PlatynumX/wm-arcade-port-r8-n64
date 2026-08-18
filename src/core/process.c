#include "wm/process.h"
#include <string.h>

void wm_scheduler_init(wm_scheduler *s) {
    memset(s, 0, sizeof(*s));
    s->next_generation = 1;
}

wm_process *wm_process_create(wm_scheduler *s, uint16_t id, wm_process_fn fn, void *user) {
    if (!s || !fn) return NULL;
    for (size_t i = 0; i < WM_MAX_PROCESSES; ++i) {
        wm_process *p = &s->slots[i];
        if (!p->active) {
            *p = (wm_process){0};
            p->id = id;
            p->active = true;
            p->wake_tick = s->tick;
            p->generation = s->next_generation++;
            if (!s->next_generation) s->next_generation = 1;
            p->fn = fn;
            p->user = user;
            return p;
        }
    }
    return NULL;
}

wm_process *wm_process_create0(wm_scheduler *s, wm_process_fn fn, void *user) {
    return wm_process_create(s, 0, fn, user);
}

void wm_process_kill(wm_process *p) {
    if (p) p->active = false;
}

size_t wm_process_kill_id(wm_scheduler *s, uint16_t id) {
    if (!s) return 0;
    size_t n = 0;
    for (size_t i = 0; i < WM_MAX_PROCESSES; ++i) {
        if (s->slots[i].active && s->slots[i].id == id) {
            s->slots[i].active = false;
            ++n;
        }
    }
    return n;
}

size_t wm_process_kill_all(wm_scheduler *s, uint16_t id, uint16_t mask) {
    if (!s) return 0;
    size_t n = 0;
    for (size_t i = 0; i < WM_MAX_PROCESSES; ++i) {
        wm_process *p = &s->slots[i];
        if (p->active && ((p->id & mask) == (id & mask))) {
            p->active = false;
            ++n;
        }
    }
    return n;
}

wm_process *wm_process_find_id(wm_scheduler *s, uint16_t id) {
    if (!s) return NULL;
    for (size_t i = 0; i < WM_MAX_PROCESSES; ++i)
        if (s->slots[i].active && s->slots[i].id == id) return &s->slots[i];
    return NULL;
}

void wm_process_sleep(wm_scheduler *s, wm_process *p, uint32_t ticks) {
    if (!s || !p) return;
    p->wake_tick = s->tick + ticks;
}

void wm_scheduler_step(wm_scheduler *s) {
    if (!s) return;
    /* Snapshot generations so CREATE during a process doesn't execute the new
       process recursively in the same source tick. It becomes runnable on the
       next scheduler pass, matching cooperative source-process behavior. */
    uint32_t snapshot[WM_MAX_PROCESSES];
    for (size_t i = 0; i < WM_MAX_PROCESSES; ++i)
        snapshot[i] = s->slots[i].active ? s->slots[i].generation : 0;

    for (size_t i = 0; i < WM_MAX_PROCESSES; ++i) {
        wm_process *p = &s->slots[i];
        if (snapshot[i] && p->active && p->generation == snapshot[i] &&
            p->fn && p->wake_tick <= s->tick) {
            p->fn(p, p->user);
        }
    }
    ++s->tick;
}
