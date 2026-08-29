#ifndef WM_CHARACTER_ASSETS_H
#define WM_CHARACTER_ASSETS_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "wm/visual.h"
#include "wm/bret_sprites.h"
typedef enum wm_character_visual_slot { WM_CV_STAND2,WM_CV_STAND4,WM_CV_TORSO2,WM_CV_TORSO4,WM_CV_WALK2,WM_CV_WALK8,WM_CV_WALK4,WM_CV_WALK6,WM_CV_RUN,WM_CV_LP2,WM_CV_LP4,WM_CV_PP,WM_CV_LK2,WM_CV_LK4,WM_CV_PK,WM_CV_COUNT } wm_character_visual_slot;
const wm_visual_sequence *wm_character_visual(uint8_t roster_id, wm_character_visual_slot slot);
const wm_source_sprite *wm_character_sprite_find(uint8_t roster_id,const char *source_frame);
bool wm_character_wimp_tail_find(uint8_t roster_id,const char *source_frame,int16_t out_tail[WM_WIMP_TAIL_WORDS]);
const wm_source_sprite *wm_character_base_sprite(uint8_t roster_id);
size_t wm_character_sprite_count(uint8_t roster_id);
#endif
