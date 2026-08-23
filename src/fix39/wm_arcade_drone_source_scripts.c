#include "wm_arcade_drone_source_scripts.h"

typedef struct WmFix39DroneSkillTable {
    const char *source_label;
    const int16_t *values;
} WmFix39DroneSkillTable;

#include "wm_arcade_drone_source_scripts_generated.h"

#if WM_FIX39_DRONE_SCRIPTS_GENERATED
static int wm_fix39_source_symbol_equal(const char *a, const char *b)
{
    unsigned char ca, cb;
    if (!a || !b) return 0;
    while (*a && *b) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= (unsigned char)'A' && ca <= (unsigned char)'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= (unsigned char)'A' && cb <= (unsigned char)'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return 0;
    }
    return *a == *b;
}

#endif

bool wm_arcade_drone_source_scripts_ready(void)
{
    return WM_FIX39_DRONE_SCRIPTS_GENERATED != 0 && WM_FIX39_DRONE_SCRIPT_COUNT > 0;
}

const wm_arcade_drone_script_t *wm_arcade_drone_source_resolve_script(const char *source_label, void *user)
{
    size_t i;
    (void)user;
    if (!source_label || !wm_arcade_drone_source_scripts_ready()) return 0;
#if WM_FIX39_DRONE_SCRIPTS_GENERATED
    for (i = 0; i < (size_t)WM_FIX39_DRONE_SCRIPT_COUNT; ++i)
        if (wm_fix39_source_symbol_equal(wm_fix39_drone_scripts[i].source_label, source_label))
            return &wm_fix39_drone_scripts[i];
#else
    (void)i;
#endif
    return 0;
}

int32_t wm_arcade_drone_source_script_skill_pct(const char *source_table_label, int skill, void *user)
{
    size_t i;
    (void)user;
    if (!source_table_label || !wm_arcade_drone_source_scripts_ready()) return 0;
    if (skill < 0) skill = 0;
    if (skill > 29) skill = 29;
#if WM_FIX39_DRONE_SCRIPTS_GENERATED
    for (i = 0; i < (size_t)WM_FIX39_DRONE_SKILL_TABLE_COUNT; ++i)
        if (wm_fix39_source_symbol_equal(wm_fix39_drone_skill_tables[i].source_label, source_table_label))
            return wm_fix39_drone_skill_tables[i].values[skill];
#else
    (void)i;
#endif
    return 0;
}

int wm_arcade_drone_source_c4_seam_count(void)
{
    return WM_FIX39_DRONE_C4_SEAM_COUNT;
}

const char *wm_arcade_drone_source_c4_seam_label(int index)
{
#if WM_FIX39_DRONE_SCRIPTS_GENERATED
    if (index < 0 || index >= WM_FIX39_DRONE_C4_SEAM_COUNT) return 0;
    return wm_fix39_drone_c4_seam_labels[index];
#else
    (void)index; return 0;
#endif
}
