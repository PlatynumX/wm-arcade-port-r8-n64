#include "wmania_ring_geometry.h"
#include "wmania_rope_command.h"
#include "wmania_rope_runtime.h"
#include "wmania_rope_source_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned calls;
    char first[64];
    char second[64];
} Capture;

static void cap_image(
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
        snprintf(c->first, sizeof(c->first), "%s", symbol);
    } else {
        snprintf(c->second, sizeof(c->second), "%s", symbol);
    }
}

static const WmRopeScript *script_of(
    const char *program_label,
    unsigned channel)
{
    const WmRopeCommandProgram *p =
        wm_rope_source_program_resolver(0, program_label);
    assert(p != 0);
    assert(channel < p->channel_count);
    assert(p->channel[channel].script != 0);
    return p->channel[channel].script;
}

static void test_counts_and_all_programs(void)
{
    size_t i;
    assert(wm_rope_source_program_count() == WM_ROPE_SOURCE_PROGRAM_COUNT);
    assert(wm_rope_source_program_count() == 70u);
    assert(wm_rope_source_script_count() == WM_ROPE_SOURCE_SCRIPT_COUNT);
    assert(wm_rope_source_script_count() > 100u);
    assert(wm_rope_source_image_pair_count() ==
           WM_ROPE_SOURCE_IMAGE_PAIR_COUNT);
    assert(wm_rope_source_image_pair_count() == 134u);

    for (i = 0; i < wm_rope_source_program_count(); ++i) {
        const WmRopeCommandProgram *p = wm_rope_source_program_at(i);
        assert(p != 0);
        assert(p->source_label != 0);
        assert(wm_rope_source_program_resolver(0, p->source_label) == p);
        assert(p->channel_count == 3u || p->channel_count == 4u);
    }

    assert(wm_rope_source_program_at(70u) == 0);
    assert(wm_rope_source_image_pair_at(134u) == 0);
}

static void test_exact_program_mappings(void)
{
    const WmRopeCommandProgram *p;

    p = wm_rope_source_program_resolver(0, "sspr14_t");
    assert(p != 0 && p->channel_count == 4u);
    assert(strcmp(p->channel[0].script->source_label, "#sspr14") == 0);
    assert(strcmp(p->channel[1].script->source_label, "#sspr12") == 0);
    assert(strcmp(p->channel[2].script->source_label, "#ssprXX") == 0);
    assert(strcmp(p->channel[3].script->source_label, "#sprshad13") == 0);
    for (unsigned i=0; i<4; ++i) assert(p->channel[i].priority == 10u);

    p = wm_rope_source_program_resolver(0, "sspr55_t");
    assert(strcmp(p->channel[0].script->source_label, "#sspr55") == 0);
    assert(strcmp(p->channel[1].script->source_label, "#sspr51") == 0);
    assert(strcmp(p->channel[3].script->source_label, "#sprshad52") == 0);

    /* Source defines this program even though current lane lookup never picks it. */
    assert(wm_rope_source_program_resolver(0, "sspr51_t") != 0);

    p = wm_rope_source_program_resolver(0, "dspr13_t");
    assert(strcmp(p->channel[0].script->source_label, "#dspr17") == 0);
    assert(strcmp(p->channel[1].script->source_label, "#dspr13") == 0);
    assert(strcmp(p->channel[2].script->source_label, "#dsprXX") == 0);
    assert(strcmp(p->channel[3].script->source_label, "#dsprshad") == 0);
    for (unsigned i=0; i<4; ++i) assert(p->channel[i].priority == 9u);

    p = wm_rope_source_program_resolver(0, "dspr56_t");
    assert(strcmp(p->channel[0].script->source_label, "#dsprXX") == 0);
    assert(strcmp(p->channel[1].script->source_label, "#dspr55") == 0);

    /* Source also defines dspr51_t even though lane lookup jumps 41 -> 52. */
    assert(wm_rope_source_program_resolver(0, "dspr51_t") != 0);
}

static void test_image_pairs(void)
{
    const WmRopeSourceImagePair *p;

    p = wm_rope_source_image_pair("RPSBIN08");
    assert(p != 0);
    assert(strcmp(p->first_image_symbol, "RPSBIN08a") == 0);
    assert(strcmp(p->second_image_symbol, "RPSBIN08b") == 0);

    p = wm_rope_source_image_pair("RCSH5_05");
    assert(p != 0);
    assert(strcmp(p->first_image_symbol, "RCSH5_05A") == 0);
    assert(strcmp(p->second_image_symbol, "RCSH5_05B") == 0);

    p = wm_rope_source_image_pair("ROPSHAD");
    assert(p != 0);
    assert(strcmp(p->first_image_symbol, "ROPSHADA") == 0);
    assert(strcmp(p->second_image_symbol, "ROPSHADB") == 0);

    p = wm_rope_source_image_pair("RPDS5_08");
    assert(p != 0);
    assert(strcmp(p->first_image_symbol, "RPDS5_08a") == 0);
}

static void test_fallthrough_script_shapes(void)
{
    const WmRopeScript *s4 = script_of("front_bounceud4_t", 0u);
    const WmRopeScript *s3 = script_of("front_bounceud3_t", 0u);
    const WmRopeScript *s2 = script_of("front_bounceud2_t", 0u);
    const WmRopeScript *s1 = script_of("front_bounceud1_t", 0u);

    /* Source labels are suffix entry points of one fall-through script. */
    assert(s4->entry_count == 5u);
    assert(s3->entry_count == 4u);
    assert(s2->entry_count == 3u);
    assert(s1->entry_count == 2u);

    assert(s4->entries[0].repeat_count == 1u);
    assert(strcmp(s4->entries[0].sequence->source_label,
                  "#f_bncud1_R") == 0);
    assert(s4->entries[2].repeat_count == 2u);
    assert(s4->entries[3].repeat_count == 3u);

    assert(strcmp(s3->entries[0].sequence->source_label,
                  "#f_bncud2_R") == 0);
    assert(strcmp(s2->entries[0].sequence->source_label,
                  "#f_bncud3") == 0);
    assert(strcmp(s1->entries[0].sequence->source_label,
                  "#f_bncud4") == 0);
}

static void test_runtime_source_smoke(void)
{
    WmRopeRuntimeBank rt;
    WmRopeRuntimeAdapter adapter;
    Capture c = {0};
    WmRopeCommand cmd;

    adapter.set_image = cap_image;
    adapter.user = &c;

    wm_rope_runtime_init_bank(&rt, WM_ROPE_FRONT, false);
    assert(wm_rope_resolve_command(
        WM_ROPE_FRONT, WM_ROPE_BOUNCE_UD, 3u, 0, &cmd));
    assert(strcmp(cmd.source_script_table, "front_bounceud4_t") == 0);
    assert(wm_rope_runtime_apply_resolved_command(
        &rt, &cmd, wm_rope_source_program_resolver, 0));

    wm_rope_runtime_tick(&rt, &adapter);
    assert(strcmp(rt.channel[WM_ROPE_CHANNEL_RED].first_image_symbol,
                  "RPFBUP02") == 0);
    assert(strcmp(rt.channel[WM_ROPE_CHANNEL_RED].second_image_symbol,
                  "RPFBUP02") == 0);

    wm_rope_runtime_init_bank(&rt, WM_ROPE_LEFT, false);
    assert(wm_rope_resolve_command(
        WM_ROPE_LEFT, WM_ROPE_BOUNCE_UD, 3u, 0, &cmd));
    assert(wm_rope_runtime_apply_resolved_command(
        &rt, &cmd, wm_rope_source_program_resolver, 0));
    wm_rope_runtime_tick(&rt, &adapter);

    /* side bounceud4 red begins at #s_bncud1_R -> RPSBUP05 pair */
    assert(strcmp(rt.channel[WM_ROPE_CHANNEL_RED].first_image_symbol,
                  "RPSBUP05a") == 0);
    assert(strcmp(rt.channel[WM_ROPE_CHANNEL_RED].second_image_symbol,
                  "RPSBUP05b") == 0);
}

static void tick_until_script(
    WmRopeRuntimeBank *rt,
    WmRopeChannel ch,
    const char *source_label,
    unsigned max_ticks)
{
    unsigned i;
    for (i=0; i<max_ticks; ++i) {
        if (rt->channel[ch].script != 0 &&
            strcmp(rt->channel[ch].script->source_label, source_label) == 0) {
            return;
        }
        wm_rope_runtime_tick(rt, 0);
    }
    assert(!"script transition timeout");
}

static void test_cross_script_gotos(void)
{
    WmRopeRuntimeBank rt;
    WmRopeCommand cmd;

    wm_rope_runtime_init_bank(&rt, WM_ROPE_LEFT, false);
    assert(wm_rope_resolve_command(
        WM_ROPE_LEFT, WM_ROPE_SIDE_SPRING_RELEASE, 0u, 0, &cmd));
    assert(strcmp(cmd.source_script_table, "sspr_trans_t") == 0);
    assert(wm_rope_runtime_apply_resolved_command(
        &rt, &cmd, wm_rope_source_program_resolver, 0));

    tick_until_script(&rt, WM_ROPE_CHANNEL_RED, "side_bounceio2_R", 64u);
    assert(rt.channel[WM_ROPE_CHANNEL_RED].priority == 10u);

    wm_rope_runtime_init_bank(&rt, WM_ROPE_LEFT, false);
    assert(wm_rope_resolve_command(
        WM_ROPE_LEFT, WM_ROPE_DOWN_SPRING_RELEASE, 0u, 0, &cmd));
    assert(strcmp(cmd.source_script_table, "dspr_trans_t") == 0);
    assert(wm_rope_runtime_apply_resolved_command(
        &rt, &cmd, wm_rope_source_program_resolver, 0));

    tick_until_script(&rt, WM_ROPE_CHANNEL_RED, "side_bounceud2_R", 64u);
    assert(rt.channel[WM_ROPE_CHANNEL_RED].priority == 9u);
}

static void test_all_command_resolutions_have_source_programs(void)
{
    WmRopeCommand c;

    for (unsigned bank=0; bank<4; ++bank) {
        for (unsigned sel=0; sel<4; ++sel) {
            if (wm_rope_resolve_command(
                    (WmRopeBank)bank, WM_ROPE_BOUNCE_UD,
                    (uint8_t)sel, 0, &c)) {
                assert(wm_rope_source_program_resolver(
                    0, c.source_script_table) != 0);
            }
        }
    }

    for (unsigned bank=WM_ROPE_LEFT; bank<=WM_ROPE_RIGHT; ++bank) {
        assert(wm_rope_resolve_command(
            (WmRopeBank)bank, WM_ROPE_BOUNCE_IO, 0u, 0, &c));
        assert(wm_rope_source_program_resolver(0,c.source_script_table));

        for (unsigned lane=0; lane<5; ++lane) {
            int32_t z = (int32_t)(WM_RING_TOP + lane*WM_ROPE_LANE_WIDTH + 1) << 16;
            for (unsigned sel=0; sel<6; ++sel) {
                if (wm_rope_resolve_command(
                        (WmRopeBank)bank, WM_ROPE_SIDE_SPRING,
                        (uint8_t)sel, z, &c)) {
                    assert(wm_rope_source_program_resolver(
                        0,c.source_script_table));
                }
                if (wm_rope_resolve_command(
                        (WmRopeBank)bank, WM_ROPE_DOWN_SPRING,
                        (uint8_t)sel, z, &c)) {
                    assert(wm_rope_source_program_resolver(
                        0,c.source_script_table));
                }
            }
        }
    }
}

int main(void)
{
    test_counts_and_all_programs();
    test_exact_program_mappings();
    test_image_pairs();
    test_fallthrough_script_shapes();
    test_runtime_source_smoke();
    test_cross_script_gotos();
    test_all_command_resolutions_have_source_programs();

    puts("wmania_ring_chunk4 tests: PASS");
    return 0;
}
