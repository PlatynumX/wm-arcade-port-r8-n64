#ifndef WM_ROSTER_H
#define WM_ROSTER_H
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    WM_WRESTLER_BRET = 0,
    WM_WRESTLER_BAM_BAM,
    WM_WRESTLER_YOKOZUNA,
    WM_WRESTLER_DOINK,
    WM_WRESTLER_RAZOR,
    WM_WRESTLER_LEX,
    WM_WRESTLER_UNDERTAKER,
    WM_WRESTLER_SHAWN,
    WM_WRESTLER_COUNT
} wm_wrestler_id;

typedef struct {
    const char *name;
    const char *source_module;
    const char *sequence_prefix;
    const char *finish_prefix;
    bool visual_backend_ready;
} wm_wrestler_def;

const wm_wrestler_def *wm_roster_get(wm_wrestler_id id);
size_t wm_roster_count(void);

#endif
