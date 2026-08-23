#ifndef WM_ARCADE_DRONE_SOURCE_TABLES_H
#define WM_ARCADE_DRONE_SOURCE_TABLES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact DRONE.ASM scalar tables, generated from the historical source. */
bool wm_arcade_drone_source_tables_ready(void);
int32_t wm_arcade_drone_source_getup_pct(int skill);
int32_t wm_arcade_drone_source_block_base_pct(int skill, void *user);
int32_t wm_arcade_drone_source_block_attack_pct(int missed_count, void *user);
int32_t wm_arcade_drone_source_headhold_delay_max(int skill, void *user);
int32_t wm_arcade_drone_source_headheld_delay_max(int skill, void *user);

#ifdef __cplusplus
}
#endif
#endif
