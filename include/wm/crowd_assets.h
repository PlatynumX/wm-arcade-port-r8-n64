#ifndef WM_CROWD_ASSETS_H
#define WM_CROWD_ASSETS_H
#include <stddef.h>
#include <stdint.h>
typedef enum { WM_CROWD_FRAME=0,WM_CROWD_GOTO,WM_CROWD_RNDWAIT,WM_CROWD_REPEAT,WM_CROWD_SHOULD_REPEAT } wm_crowd_op;
typedef struct { uint8_t op; uint16_t arg; uint16_t value; } wm_crowd_cmd;
typedef struct { const wm_crowd_cmd *cmd; uint16_t count; } wm_crowd_script;
typedef struct { const char *symbol,*path; uint16_t width,height; int16_t xani,yani; uint32_t pixel_bytes; uint16_t palette_offset,palette_colors; } wm_crowd_asset;
typedef struct { uint16_t normal_script,cheer1_script,cheer2_script; } wm_crowd_person;
size_t wm_crowd_person_count(void);
const wm_crowd_person *wm_crowd_person_at(size_t i);
const wm_crowd_script *wm_crowd_script_at(size_t i);
const wm_crowd_asset *wm_crowd_asset_at(size_t i);
#endif
