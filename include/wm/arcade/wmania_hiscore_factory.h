#ifndef WMANIA_HISCORE_FACTORY_H
#define WMANIA_HISCORE_FACTORY_H

#include "wm/arcade/wmania_hiscore_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WM_HS_STREAK_LAST_ENTRY 18u
#define WM_HS_PIN_SPEED_LAST_ENTRY 9u
#define WM_HS_BEATEN_LAST_ENTRY 30u
#define WM_HS_INTER_LAST_ENTRY 30u
#define WM_HS_TAG_LAST_ENTRY 18u

extern const WmHsTableTemplate wm_hs_streak_template;
extern const WmHsTableTemplate wm_hs_pin_speed_template;
extern const WmHsTableTemplate wm_hs_beaten_template;
extern const WmHsTableTemplate wm_hs_inter_template;
extern const WmHsTableTemplate wm_hs_tag_template;

#ifdef __cplusplus
}
#endif

#endif
