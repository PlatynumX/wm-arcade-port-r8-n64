#ifndef WM_SOURCE_DATA_H
#define WM_SOURCE_DATA_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *source_file;
    const char *source_label;
    const uint16_t *words;
    size_t word_count;
} wm_source_sequence;

extern const uint16_t wm_seq_hrt_finish1_move[];
extern const size_t wm_seq_hrt_finish1_move_count;
extern const wm_source_sequence wm_source_hrt_finish1_move;

#endif
