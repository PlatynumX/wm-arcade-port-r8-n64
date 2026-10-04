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

/*
 * FINISEQ.ASM's hrt_finish1_move -- CUT CONTENT, and nothing reads it.
 *
 * GAME.EQU:580 sets NUM_BRET_FINISHES to 0, and the routine's own
 * definition sits inside `.if NUM_BRET_FINISHES`, so the assembler
 * never emitted it: Bret has no finishing move in the shipped game.
 * Undertaker's und_finish_move1 is the only one that exists.
 *
 * It stays extracted because it is real source text and because this
 * was the port's first extractor -- but tools/asmseq.py now stamps the
 * generated file with a banner saying so, decided from the `.if`
 * conditions rather than written by hand, so the banner tracks the
 * switch if it ever changes.
 */
extern const uint16_t wm_seq_hrt_finish1_move[];
extern const size_t wm_seq_hrt_finish1_move_count;
extern const wm_source_sequence wm_source_hrt_finish1_move;

#endif
