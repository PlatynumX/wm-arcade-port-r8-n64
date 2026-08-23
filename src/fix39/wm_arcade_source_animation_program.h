#ifndef WM_ARCADE_SOURCE_ANIMATION_PROGRAM_H
#define WM_ARCADE_SOURCE_ANIMATION_PROGRAM_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { WM_SRC_ARG_TEXT=0, WM_SRC_ARG_NUM=1, WM_SRC_ARG_LOCAL_PC=2, WM_SRC_ARG_TABLE=3 };
typedef struct { int32_t value; const char *text; uint8_t kind; } wm_source_anim_arg_t;
typedef enum { WM_SRC_INS_CMD=0, WM_SRC_INS_FRAME=1 } wm_source_anim_ins_kind_t;
typedef struct { uint8_t kind; int16_t opcode; uint8_t argc; uint16_t ticks; const char *name; wm_source_anim_arg_t a[9]; } wm_source_anim_ins_t;
typedef struct { uint8_t roster_id; const char *label; const char *source_file; const wm_source_anim_ins_t *ins; uint16_t count; } wm_source_anim_program_t;
typedef struct { const char *source_file; const char *label; const wm_source_anim_arg_t *entries; uint16_t count; } wm_source_anim_table_t;
const wm_source_anim_program_t *wm_source_anim_program_find(uint8_t roster_id,const char *label);
const wm_source_anim_table_t *wm_source_anim_table_by_id(uint16_t id);
const wm_source_anim_table_t *wm_source_anim_table_find(const char *source_file,const char *label);
size_t wm_source_anim_program_count(void);
size_t wm_source_anim_table_count(void);
#ifdef __cplusplus
}
#endif
#endif
