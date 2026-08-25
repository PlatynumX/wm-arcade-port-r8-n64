#ifndef WM_N64_DCS_BANK_H
#define WM_N64_DCS_BANK_H
#include <stdbool.h>
#include <stdint.h>
void wm_dcs_bank_init(void);
bool wm_dcs_bank_play(uint16_t command,uint32_t source_tick);
bool wm_dcs_bank_play_source(uint16_t command,uint32_t source_tick,int8_t source_channel);
void wm_dcs_bank_stop_source(int8_t source_channel);
void wm_dcs_bank_set_source_volume(int8_t source_channel,uint8_t volume);
void wm_dcs_bank_set_master_volume(uint8_t volume);
void wm_dcs_bank_service(void);
void wm_dcs_bank_stop(void);
#endif
