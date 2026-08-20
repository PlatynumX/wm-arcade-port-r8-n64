#include "wmania_rope_command.h"
#include "wmania_rope_runtime.h"
#include "wmania_rope_spawn.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned calls;
    char last_first[32];
    char last_second[32];
} Capture;

static void capture_image(
    void *user,
    WmRopeBank bank,
    WmRopeChannel channel,
    WmRopeHalf half,
    const char *symbol)
{
    Capture *c = (Capture *)user;
    (void)bank;
    (void)channel;

    ++c->calls;
    if (half == WM_ROPE_HALF_FIRST) {
        snprintf(c->last_first, sizeof(c->last_first), "%s", symbol);
    } else {
        snprintf(c->last_second, sizeof(c->last_second), "%s", symbol);
    }
}

static void test_spawn_tables(void)
{
    const WmRopeObjectSeed *s;

    s = wm_rope_object_seed(
        WM_ROPE_FRONT, WM_ROPE_CHANNEL_RED, WM_ROPE_HALF_FIRST);
    assert(s != 0 && s->exists);
    assert(strcmp(s->source_image_symbol, "ROPE_F_R") == 0);
    assert(s->raw_x == 674);
    assert(s->raw_y == 400);
    assert(s->raw_z == 0x15aa);
    assert(wm_rope_spawn_x_fp16(s) == ((674 + 104) << 16));
    assert(wm_rope_spawn_y_fp16(s) == ((400 - 258) << 16));

    s = wm_rope_object_seed(
        WM_ROPE_RIGHT, WM_ROPE_CHANNEL_SHADOW, WM_ROPE_HALF_SECOND);
    assert(s != 0 && s->exists);
    assert(strcmp(s->source_image_symbol, "ROPSHADB") == 0);
    assert(s->flip_horizontal);
    assert(s->raw_x == 0x469 + 100);

    s = wm_rope_object_seed(
        WM_ROPE_BACK, WM_ROPE_CHANNEL_SHADOW, WM_ROPE_HALF_FIRST);
    assert(s != 0 && !s->exists);

    assert(wm_rope_process_survives_reduce_bog(WM_ROPE_FRONT, false));
    assert(!wm_rope_process_survives_reduce_bog(WM_ROPE_FRONT, true));
    assert(!wm_rope_process_survives_reduce_bog(WM_ROPE_BACK, true));
    assert(wm_rope_process_survives_reduce_bog(WM_ROPE_LEFT, true));
    assert(wm_rope_process_survives_reduce_bog(WM_ROPE_RIGHT, true));
}

static void test_horizontal_runtime(void)
{
    static const WmRopeFrame seq_frames[] = {
        { WM_ROPE_FRAME_IMAGE, 2u, "IMG_A", 0, 0 },
        { WM_ROPE_FRAME_IMAGE, 1u, "IMG_B", 0, 0 },
        { WM_ROPE_FRAME_END, 0u, 0, 0, 0 }
    };
    static const WmRopeSequence seq = {
        "test_seq", seq_frames, 3u
    };
    static const WmRopeScriptEntry script_entries[] = {
        { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq, 0u , 0 },
        { WM_ROPE_SCRIPT_END, 0u, 0, 0u , 0 }
    };
    static const WmRopeScript script = {
        "test_script", script_entries, 2u
    };
    static const WmRopeCommandProgram program = {
        "test_program", 3u,
        {
            { 5u, &script },
            { 5u, &script },
            { 5u, &script },
            { 0u, 0 }
        }
    };

    WmRopeRuntimeBank rt;
    WmRopeRuntimeAdapter adapter;
    Capture cap = {0};

    adapter.set_image = capture_image;
    adapter.user = &cap;

    wm_rope_runtime_init_bank(&rt, WM_ROPE_FRONT, false);
    assert(rt.process_alive);
    assert(rt.horizontal_bank);
    assert(!rt.channel[WM_ROPE_CHANNEL_SHADOW].first_object_exists);

    assert(wm_rope_runtime_apply_program(&rt, &program));
    assert(rt.channel[0].priority == 5u);
    assert(rt.channel[0].sequence_hold_ticks == 1u);

    /* First source process tick advances immediately to IMG_A. */
    wm_rope_runtime_tick(&rt, &adapter);
    assert(strcmp(rt.channel[0].first_image_symbol, "IMG_A") == 0);
    assert(strcmp(rt.channel[0].second_image_symbol, "IMG_A") == 0);

    /* Hold=2 means one complete holding tick, then advance on next. */
    wm_rope_runtime_tick(&rt, &adapter);
    assert(strcmp(rt.channel[0].first_image_symbol, "IMG_A") == 0);

    wm_rope_runtime_tick(&rt, &adapter);
    assert(strcmp(rt.channel[0].first_image_symbol, "IMG_B") == 0);

    /* IMG_B hold=1; next tick reaches script end and clears priority. */
    wm_rope_runtime_tick(&rt, &adapter);
    assert(rt.channel[0].sequence_hold_ticks == 0u);
    assert(rt.channel[0].priority == 0u);
    assert(cap.calls > 0u);
}

static void test_side_pair_and_priority(void)
{
    static const WmRopeFrame seq_frames[] = {
        { WM_ROPE_FRAME_IMAGE, 1u, "TOP_A", "BOT_A", 0 },
        { WM_ROPE_FRAME_END, 0u, 0, 0, 0 }
    };
    static const WmRopeSequence seq = {
        "side_seq", seq_frames, 2u
    };
    static const WmRopeScriptEntry entries[] = {
        { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq, 0u , 0 },
        { WM_ROPE_SCRIPT_END, 0u, 0, 0u , 0 }
    };
    static const WmRopeScript script = {
        "side_script", entries, 2u
    };
    static const WmRopeCommandProgram low = {
        "low", 4u,
        {
            { 5u, &script }, { 5u, &script },
            { 5u, &script }, { 5u, &script }
        }
    };
    static const WmRopeCommandProgram high = {
        "high", 4u,
        {
            { 10u, &script }, { 10u, &script },
            { 10u, &script }, { 10u, &script }
        }
    };

    WmRopeRuntimeBank rt;
    WmRopeRuntimeAdapter adapter;
    Capture cap = {0};

    adapter.set_image = capture_image;
    adapter.user = &cap;

    wm_rope_runtime_init_bank(&rt, WM_ROPE_LEFT, false);
    assert(!rt.horizontal_bank);
    assert(rt.channel[WM_ROPE_CHANNEL_SHADOW].first_object_exists);

    assert(wm_rope_runtime_apply_program(&rt, &high));
    assert(rt.channel[0].priority == 10u);

    /* Lower-priority wake is rejected on every already-high channel. */
    assert(!wm_rope_runtime_apply_program(&rt, &low));
    assert(rt.channel[0].priority == 10u);

    /* Equal priority is accepted by the source rule. */
    assert(wm_rope_runtime_apply_program(&rt, &high));

    wm_rope_runtime_tick(&rt, &adapter);
    assert(strcmp(rt.channel[0].first_image_symbol, "TOP_A") == 0);
    assert(strcmp(rt.channel[0].second_image_symbol, "BOT_A") == 0);
}

static void test_reduce_bog_runtime(void)
{
    static const WmRopeFrame frames[] = {
        { WM_ROPE_FRAME_IMAGE, 1u, "X", 0, 0 },
        { WM_ROPE_FRAME_END, 0u, 0, 0, 0 }
    };
    static const WmRopeSequence seq = { "s", frames, 2u };
    static const WmRopeScriptEntry entries[] = {
        { WM_ROPE_SCRIPT_SEQUENCE, 1u, &seq, 0u , 0 },
        { WM_ROPE_SCRIPT_END, 0u, 0, 0u , 0 }
    };
    static const WmRopeScript script = { "sc", entries, 2u };
    static const WmRopeCommandProgram p = {
        "p", 3u, {{5u,&script},{5u,&script},{5u,&script},{0u,0}}
    };
    WmRopeRuntimeBank rt;

    wm_rope_runtime_init_bank(&rt, WM_ROPE_FRONT, true);

    /* Objects were still created before the source process dies. */
    assert(rt.channel[0].first_object_exists);
    assert(rt.channel[0].second_object_exists);
    assert(!rt.process_alive);

    /* No live process => rope_command would see a null process pointer. */
    assert(!wm_rope_runtime_apply_program(&rt, &p));
}

int main(void)
{
    test_spawn_tables();
    test_horizontal_runtime();
    test_side_pair_and_priority();
    test_reduce_bog_runtime();

    puts("wmania_ring_chunk2 tests: PASS");
    return 0;
}
