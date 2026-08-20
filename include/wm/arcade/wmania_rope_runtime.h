#ifndef WMANIA_ROPE_RUNTIME_H
#define WMANIA_ROPE_RUNTIME_H

#include "wm/arcade/wmania_rope_command.h"
#include "wm/arcade/wmania_rope_spawn.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Portable semantic representation of the two-level ROPES.ASM animation
 * format:
 *
 * command table -> per-channel script -> repeated sequence -> timed frames
 *
 * The original uses a high-bit word as its only remaining special command:
 * RANI_GOTO.  We expose it as explicit enum values instead of depending on
 * host pointer sizes or Midway's packed byte layout.
 */

typedef enum {
    WM_ROPE_FRAME_IMAGE = 0,
    WM_ROPE_FRAME_GOTO = 1,
    WM_ROPE_FRAME_END = 2
} WmRopeFrameKind;

struct WmRopeSequence;

typedef struct {
    WmRopeFrameKind kind;

    /* IMAGE */
    uint16_t hold_ticks;
    const char *first_image_symbol;
    const char *second_image_symbol;

    /* GOTO */
    const struct WmRopeSequence *goto_sequence;
} WmRopeFrame;

typedef struct WmRopeSequence {
    const char *source_label;
    const WmRopeFrame *frames;
    size_t frame_count;
} WmRopeSequence;

struct WmRopeScript;

typedef enum {
    WM_ROPE_SCRIPT_SEQUENCE = 0,
    WM_ROPE_SCRIPT_GOTO = 1,
    WM_ROPE_SCRIPT_END = 2
} WmRopeScriptEntryKind;

typedef struct {
    WmRopeScriptEntryKind kind;

    /* SEQUENCE */
    uint16_t repeat_count;
    const WmRopeSequence *sequence;

    /* GOTO */
    size_t goto_entry_index;

    /*
     * NULL = jump within the current script.
     * Non-NULL = source RANI_GOTO into another script label.
     */
    const struct WmRopeScript *goto_script;
} WmRopeScriptEntry;

typedef struct WmRopeScript {
    const char *source_label;
    const WmRopeScriptEntry *entries;
    size_t entry_count;
} WmRopeScript;

typedef struct {
    uint16_t priority;
    const WmRopeScript *script;
} WmRopeChannelOrder;

typedef struct {
    const char *source_label;
    size_t channel_count; /* source: 3 front/back, 4 left/right */
    WmRopeChannelOrder channel[WM_ROPE_CHANNEL_COUNT];
} WmRopeCommandProgram;

/*
 * Resolves a Chunk-1 command's source_script_table label (for example
 * "sspr32_t") to the direct-translated program data.
 *
 * The runtime deliberately keeps this data provider separate so the merge
 * can bind a generated/direct translation of ROPES.ASM script data without
 * changing the state machine.
 */
typedef const WmRopeCommandProgram *(*WmRopeProgramResolver)(
    void *user,
    const char *source_script_table);

typedef struct {
    bool first_object_exists;
    bool second_object_exists;

    uint16_t priority;

    const WmRopeScript *script;
    size_t script_entry_index;
    uint16_t script_repeat_count;

    const WmRopeSequence *sequence;
    size_t sequence_frame_index;
    uint16_t sequence_hold_ticks;

    const char *first_image_symbol;
    const char *second_image_symbol;
} WmRopeRuntimeChannel;

typedef struct {
    WmRopeBank bank;
    bool horizontal_bank;
    bool process_alive;
    WmRopeRuntimeChannel channel[WM_ROPE_CHANNEL_COUNT];
} WmRopeRuntimeBank;

typedef void (*WmRopeImageUpdateFn)(
    void *user,
    WmRopeBank bank,
    WmRopeChannel channel,
    WmRopeHalf half,
    const char *source_image_symbol);

typedef struct {
    WmRopeImageUpdateFn set_image;
    void *user;
} WmRopeRuntimeAdapter;

/*
 * Direct rope process initialization:
 * - object existence/current images come from the live position table
 * - every script/sequence counter and priority starts at zero
 * - reduce_bog kills only front/back processes after object creation
 */
void wm_rope_runtime_init_bank(
    WmRopeRuntimeBank *runtime,
    WmRopeBank bank,
    bool reduce_bog);

/*
 * Portable new_command_wake:
 * for each channel that exists, existing priority > incoming priority skips;
 * incoming >= existing installs the new script and primes SQCNT to 1.
 */
bool wm_rope_runtime_apply_program(
    WmRopeRuntimeBank *runtime,
    const WmRopeCommandProgram *program);

/*
 * Convenience bridge from Chunk 1 rope_command resolution to Chunk 2.
 */
bool wm_rope_runtime_apply_resolved_command(
    WmRopeRuntimeBank *runtime,
    const WmRopeCommand *command,
    WmRopeProgramResolver resolver,
    void *resolver_user);

/*
 * Direct main-loop semantic: one call = one source rope-process tick.
 * All four channel blocks are updated in red/white/blue/shadow order.
 */
void wm_rope_runtime_tick(
    WmRopeRuntimeBank *runtime,
    const WmRopeRuntimeAdapter *adapter);

#ifdef __cplusplus
}
#endif

#endif
