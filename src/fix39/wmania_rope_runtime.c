#include "wmania_rope_runtime.h"

#include <string.h>

#define WM_FIX39_ROPE_RUNTIME_GUARD 128u

static void clear_animation(WmRopeRuntimeChannel *c)
{
    /*
     * End-of-script in rope_update clears a long starting at PD_SQCNT.
     * On the arcade that clears sequence count + priority together.
     */
    c->sequence_hold_ticks = 0u;
    c->priority = 0u;
}

static bool load_script_entry(WmRopeRuntimeChannel *c)
{
    unsigned guard = 0u;

    while (c->script != 0 &&
           c->script_entry_index < c->script->entry_count &&
           guard++ < WM_FIX39_ROPE_RUNTIME_GUARD) {
        const WmRopeScriptEntry *entry =
            &c->script->entries[c->script_entry_index];

        switch (entry->kind) {
        case WM_FIX39_ROPE_SCRIPT_SEQUENCE:
            if (entry->repeat_count == 0u || entry->sequence == 0) {
                clear_animation(c);
                return false;
            }

            c->script_repeat_count = entry->repeat_count;
            c->sequence = entry->sequence;
            c->sequence_frame_index = 0u;

            /*
             * new_command_wake explicitly sets PD_SQCNT=1 so the next
             * source tick immediately falls through to the first frame.
             */
            c->sequence_hold_ticks = 1u;
            return true;

        case WM_FIX39_ROPE_SCRIPT_GOTO:
        {
            const WmRopeScript *target =
                entry->goto_script != 0 ? entry->goto_script : c->script;

            if (target == 0 ||
                entry->goto_entry_index >= target->entry_count) {
                clear_animation(c);
                return false;
            }

            c->script = target;
            c->script_entry_index = entry->goto_entry_index;
            break;
        }

        case WM_FIX39_ROPE_SCRIPT_END:
        default:
            clear_animation(c);
            return false;
        }
    }

    clear_animation(c);
    return false;
}

static bool repeat_or_advance_script(WmRopeRuntimeChannel *c)
{
    if (c->script_repeat_count > 0u) {
        --c->script_repeat_count;
    }

    if (c->script_repeat_count != 0u) {
        const WmRopeScriptEntry *entry =
            &c->script->entries[c->script_entry_index];

        c->sequence = entry->sequence;
        c->sequence_frame_index = 0u;
        return true;
    }

    ++c->script_entry_index;
    return load_script_entry(c);
}

static void send_image(
    const WmRopeRuntimeAdapter *adapter,
    WmRopeBank bank,
    WmRopeChannel channel,
    WmRopeHalf half,
    const char *symbol)
{
    if (adapter != 0 && adapter->set_image != 0 && symbol != 0) {
        adapter->set_image(adapter->user, bank, channel, half, symbol);
    }
}

static void render_frame(
    WmRopeRuntimeBank *runtime,
    WmRopeChannel channel_index,
    WmRopeRuntimeChannel *c,
    const WmRopeFrame *frame,
    const WmRopeRuntimeAdapter *adapter)
{
    const char *second;

    c->first_image_symbol = frame->first_image_symbol;

    /*
     * Front/back: direct image pointer is used by both halves.
     * Side: source frame points to a pair {top,bottom}; portable data puts
     * those two symbols directly in the frame.
     */
    if (runtime->horizontal_bank) {
        second = frame->first_image_symbol;
    } else {
        second = frame->second_image_symbol;
    }

    c->second_image_symbol = second;

    send_image(adapter, runtime->bank, channel_index,
               WM_FIX39_ROPE_HALF_FIRST, c->first_image_symbol);

    if (c->second_object_exists) {
        send_image(adapter, runtime->bank, channel_index,
                   WM_FIX39_ROPE_HALF_SECOND, c->second_image_symbol);
    }
}

static void tick_channel(
    WmRopeRuntimeBank *runtime,
    WmRopeChannel channel_index,
    const WmRopeRuntimeAdapter *adapter)
{
    WmRopeRuntimeChannel *c =
        &runtime->channel[(unsigned)channel_index];
    unsigned guard = 0u;

    if (c->sequence_hold_ticks == 0u || !c->first_object_exists) {
        return;
    }

    --c->sequence_hold_ticks;
    if (c->sequence_hold_ticks != 0u) {
        return;
    }

    /*
     * The source can chain GOTO/end transitions without sleeping.  Guard
     * against malformed translated data while retaining that behavior.
     */
    while (guard++ < WM_FIX39_ROPE_RUNTIME_GUARD) {
        const WmRopeFrame *frame;

        if (c->sequence == 0 ||
            c->sequence_frame_index >= c->sequence->frame_count) {
            if (!repeat_or_advance_script(c)) {
                return;
            }
            continue;
        }

        frame = &c->sequence->frames[c->sequence_frame_index++];

        switch (frame->kind) {
        case WM_FIX39_ROPE_FRAME_IMAGE:
            if (frame->hold_ticks == 0u) {
                /*
                 * Zero is the source end-of-sequence marker. Portable
                 * translated data should normally use FRAME_END, but treat
                 * a zero image hold identically for robustness.
                 */
                if (!repeat_or_advance_script(c)) {
                    return;
                }
                continue;
            }

            c->sequence_hold_ticks = frame->hold_ticks;
            render_frame(runtime, channel_index, c, frame, adapter);
            return;

        case WM_FIX39_ROPE_FRAME_GOTO:
            if (frame->goto_sequence == 0) {
                clear_animation(c);
                return;
            }
            c->sequence = frame->goto_sequence;
            c->sequence_frame_index = 0u;
            continue;

        case WM_FIX39_ROPE_FRAME_END:
        default:
            if (!repeat_or_advance_script(c)) {
                return;
            }
            continue;
        }
    }

    clear_animation(c);
}

void wm_rope_runtime_init_bank(
    WmRopeRuntimeBank *runtime,
    WmRopeBank bank,
    bool reduce_bog)
{
    const WmRopeBankSeed *seed;
    unsigned i;

    if (runtime == 0) {
        return;
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->bank = bank;

    seed = wm_rope_bank_seed(bank);
    if (seed == 0) {
        return;
    }

    runtime->horizontal_bank = seed->horizontal_bank;
    runtime->process_alive =
        wm_rope_process_survives_reduce_bog(bank, reduce_bog);

    for (i = 0u; i < WM_FIX39_ROPE_CHANNEL_COUNT; ++i) {
        WmRopeRuntimeChannel *c = &runtime->channel[i];

        c->first_object_exists =
            seed->object[i][WM_FIX39_ROPE_HALF_FIRST].exists;
        c->second_object_exists =
            seed->object[i][WM_FIX39_ROPE_HALF_SECOND].exists;

        c->first_image_symbol =
            seed->object[i][WM_FIX39_ROPE_HALF_FIRST].source_image_symbol;
        c->second_image_symbol =
            seed->object[i][WM_FIX39_ROPE_HALF_SECOND].source_image_symbol;

        /* Source rope() clears all runtime pointers/counters/priorities. */
        c->priority = 0u;
        c->script = 0;
        c->script_entry_index = 0u;
        c->script_repeat_count = 0u;
        c->sequence = 0;
        c->sequence_frame_index = 0u;
        c->sequence_hold_ticks = 0u;
    }
}

bool wm_rope_runtime_apply_program(
    WmRopeRuntimeBank *runtime,
    const WmRopeCommandProgram *program)
{
    size_t i;
    bool changed = false;

    if (runtime == 0 || program == 0 || !runtime->process_alive) {
        return false;
    }

    for (i = 0u;
         i < program->channel_count && i < WM_FIX39_ROPE_CHANNEL_COUNT;
         ++i) {
        WmRopeRuntimeChannel *c = &runtime->channel[i];
        const WmRopeChannelOrder *order = &program->channel[i];
        const WmRopeScriptEntry *first;

        if (!c->first_object_exists || order->script == 0 ||
            order->script->entry_count == 0u) {
            continue;
        }

        if (!wm_rope_priority_accepts(c->priority, order->priority)) {
            continue;
        }

        first = &order->script->entries[0];
        if (first->kind != WM_FIX39_ROPE_SCRIPT_SEQUENCE ||
            first->repeat_count == 0u || first->sequence == 0) {
            /*
             * The live command tables supplied to new_command_wake begin
             * with a normal repeated sequence. Refuse malformed translation
             * instead of inventing an interpretation.
             */
            continue;
        }

        c->priority = order->priority;
        c->script = order->script;
        c->script_entry_index = 0u;
        c->script_repeat_count = first->repeat_count;
        c->sequence = first->sequence;
        c->sequence_frame_index = 0u;
        c->sequence_hold_ticks = 1u;
        changed = true;
    }

    return changed;
}

bool wm_rope_runtime_apply_resolved_command(
    WmRopeRuntimeBank *runtime,
    const WmRopeCommand *command,
    WmRopeProgramResolver resolver,
    void *resolver_user)
{
    const WmRopeCommandProgram *program;

    if (runtime == 0 || command == 0 || resolver == 0 ||
        runtime->bank != command->bank) {
        return false;
    }

    program = resolver(resolver_user, command->source_script_table);
    if (program == 0) {
        return false;
    }

    return wm_rope_runtime_apply_program(runtime, program);
}

void wm_rope_runtime_tick(
    WmRopeRuntimeBank *runtime,
    const WmRopeRuntimeAdapter *adapter)
{
    unsigned i;

    if (runtime == 0 || !runtime->process_alive) {
        return;
    }

    for (i = 0u; i < WM_FIX39_ROPE_CHANNEL_COUNT; ++i) {
        tick_channel(runtime, (WmRopeChannel)i, adapter);
    }
}
