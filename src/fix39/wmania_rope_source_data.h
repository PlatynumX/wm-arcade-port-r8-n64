#ifndef WMANIA_ROPE_SOURCE_DATA_H
#define WMANIA_ROPE_SOURCE_DATA_H

#include "wmania_rope_runtime.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Complete direct C translation of the static command/script/sequence data
 * in ROPES.ASM.
 *
 * 70 source script-table entry points:
 *   4 front bounce-UD
 *   4 back bounce-UD
 *   4 side bounce-UD
 *   1 side bounce-IO
 *   25 sideways-spring tables
 *   30 down-spring tables
 *   1 sideways-spring release
 *   1 down-spring release
 */
#define WM_ROPE_SOURCE_PROGRAM_COUNT 70u
#define WM_ROPE_SOURCE_IMAGE_PAIR_COUNT 134u
#define WM_ROPE_SOURCE_SCRIPT_COUNT 143u

typedef struct {
    const char *source_pair_label;
    const char *first_image_symbol;
    const char *second_image_symbol;
} WmRopeSourceImagePair;

const WmRopeCommandProgram *wm_rope_source_program_resolver(
    void *user,
    const char *source_script_table);

size_t wm_rope_source_program_count(void);
const WmRopeCommandProgram *wm_rope_source_program_at(size_t index);

size_t wm_rope_source_script_count(void);
const WmRopeScript *wm_rope_source_script_at(size_t index);

const WmRopeSourceImagePair *wm_rope_source_image_pair(
    const char *source_pair_label);
size_t wm_rope_source_image_pair_count(void);
const WmRopeSourceImagePair *wm_rope_source_image_pair_at(size_t index);

#ifdef __cplusplus
}
#endif

#endif
