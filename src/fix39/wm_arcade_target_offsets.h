#ifndef WM_ARCADE_TARGET_OFFSETS_H
#define WM_ARCADE_TARGET_OFFSETS_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool wm_source_target_offsets(uint16_t player_mode,uint8_t wrestler_num,uint16_t target_area,int16_t *x,int16_t *y,int16_t *z);
#ifdef __cplusplus
}
#endif
#endif
