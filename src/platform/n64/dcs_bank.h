#ifndef WM_N64_DCS_BANK_H
#define WM_N64_DCS_BANK_H

#include <stdbool.h>
#include <stdint.h>

void wm_dcs_bank_init(void);
bool wm_dcs_bank_play(uint16_t command, uint32_t source_tick);
void wm_dcs_bank_service(void);
void wm_dcs_bank_stop(void);

#endif
