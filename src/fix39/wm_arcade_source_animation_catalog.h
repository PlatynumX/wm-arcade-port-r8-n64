#ifndef WM_ARCADE_SOURCE_ANIMATION_CATALOG_H
#define WM_ARCADE_SOURCE_ANIMATION_CATALOG_H
#include <stddef.h>
#include <stdint.h>
typedef struct { const char *source_frame; uint16_t ticks; } wm_source_anim_frame_t;
enum { WM_SRC_ANIM_INIT_ANIM_MODE=1u<<0, WM_SRC_ANIM_INIT_PLAYER_MODE=1u<<1, WM_SRC_ANIM_INIT_FRICTION=1u<<2, WM_SRC_ANIM_INIT_SPEED=1u<<3, WM_SRC_ANIM_INIT_XVEL=1u<<4, WM_SRC_ANIM_INIT_YVEL=1u<<5, WM_SRC_ANIM_INIT_ZVEL=1u<<6, WM_SRC_ANIM_INIT_GRAVITY=1u<<7 };
typedef struct { uint16_t mask,anim_mode,player_mode,speed; int32_t friction,xvel,yvel,zvel,gravity; } wm_source_anim_init_t;
typedef struct { uint8_t roster_id; const char *label; const char *source_file; const wm_source_anim_frame_t *frames; uint16_t frame_count; uint8_t repeat; wm_source_anim_init_t init; } wm_source_anim_def_t;
const wm_source_anim_def_t *wm_source_anim_find(uint8_t roster_id,const char *label);
const char *wm_source_bret_anim_label(int id);
const char *wm_source_razor_anim_label(int id);
const char *wm_source_reaction_anim_label(uint8_t roster_id,int group,int facing_dir);
#endif
