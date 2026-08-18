#ifndef WM_PROCESS_H
#define WM_PROCESS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WM_MAX_PROCESSES 96

typedef struct wm_process wm_process;
typedef void (*wm_process_fn)(wm_process *proc, void *user);

struct wm_process {
    uint16_t id;
    bool active;
    uint32_t wake_tick;
    uint32_t state;
    uint32_t generation;
    wm_process_fn fn;
    void *user;
};

typedef struct {
    uint32_t tick;
    uint32_t next_generation;
    wm_process slots[WM_MAX_PROCESSES];
} wm_scheduler;

void wm_scheduler_init(wm_scheduler *s);
wm_process *wm_process_create(wm_scheduler *s, uint16_t id, wm_process_fn fn, void *user);
wm_process *wm_process_create0(wm_scheduler *s, wm_process_fn fn, void *user);
void wm_process_kill(wm_process *p);
size_t wm_process_kill_id(wm_scheduler *s, uint16_t id);
size_t wm_process_kill_all(wm_scheduler *s, uint16_t id, uint16_t mask);
wm_process *wm_process_find_id(wm_scheduler *s, uint16_t id);
void wm_process_sleep(wm_scheduler *s, wm_process *p, uint32_t ticks);
void wm_scheduler_step(wm_scheduler *s);

#endif
