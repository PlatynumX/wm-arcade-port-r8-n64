#include "wm_arcade_drone_source_tables.h"
#include "wm_arcade_drone_source_tables_generated.h"

static int clamp_skill(int skill)
{
    if (skill < 0) return 0;
    if (skill >= WM_FIX39_DRONE_SKILL_COUNT) return WM_FIX39_DRONE_SKILL_COUNT - 1;
    return skill;
}

static int clamp_miss(int missed)
{
    if (missed < 0) return 0;
    if (missed >= WM_FIX39_DRONE_BLOCK_MISS_COUNT) return WM_FIX39_DRONE_BLOCK_MISS_COUNT - 1;
    return missed;
}

bool wm_arcade_drone_source_tables_ready(void)
{
    return WM_FIX39_DRONE_SOURCE_GENERATED != 0;
}

int32_t wm_arcade_drone_source_getup_pct(int skill)
{
    if (!wm_arcade_drone_source_tables_ready()) return 0;
    return wm_fix39_drone_getup_t[clamp_skill(skill)];
}

int32_t wm_arcade_drone_source_block_base_pct(int skill, void *user)
{
    (void)user;
    if (!wm_arcade_drone_source_tables_ready()) return 0;
    return wm_fix39_drone_blkbase_t[clamp_skill(skill)];
}

int32_t wm_arcade_drone_source_block_attack_pct(int missed_count, void *user)
{
    (void)user;
    if (!wm_arcade_drone_source_tables_ready()) return 0;
    return wm_fix39_drone_blkatk_t[clamp_miss(missed_count)];
}

int32_t wm_arcade_drone_source_headhold_delay_max(int skill, void *user)
{
    (void)user;
    if (!wm_arcade_drone_source_tables_ready()) return 0;
    return wm_fix39_drone_sklhhdly_t[clamp_skill(skill)];
}

int32_t wm_arcade_drone_source_headheld_delay_max(int skill, void *user)
{
    (void)user;
    if (!wm_arcade_drone_source_tables_ready()) return 0;
    return wm_fix39_drone_sklhrdly_t[clamp_skill(skill)];
}
