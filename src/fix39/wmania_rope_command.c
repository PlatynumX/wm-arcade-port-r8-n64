#include "wmania_rope_command.h"
#include "wmania_ring_geometry.h"

#include <string.h>

static const char *const front_ud[4] = {
    "front_bounceud1_t",
    "front_bounceud2_t",
    "front_bounceud3_t",
    "front_bounceud4_t"
};

static const char *const back_ud[4] = {
    "back_bounceud1_t",
    "back_bounceud2_t",
    "back_bounceud3_t",
    "back_bounceud4_t"
};

static const char *const side_ud[4] = {
    "side_bounceud1_t",
    "side_bounceud2_t",
    "side_bounceud3_t",
    "side_bounceud4_t"
};

/*
 * Exact #sspring table.  The sixth position in every row is NULL in source.
 */
static const char *const sspring[5][6] = {
    { "sspr11_t","sspr12_t","sspr13_t","sspr14_t","sspr15_t",0 },
    { "sspr11_t","sspr22_t","sspr23_t","sspr24_t","sspr25_t",0 },
    { "sspr21_t","sspr32_t","sspr33_t","sspr34_t","sspr35_t",0 },
    { "sspr31_t","sspr42_t","sspr43_t","sspr44_t","sspr45_t",0 },
    { "sspr41_t","sspr52_t","sspr53_t","sspr54_t","sspr55_t",0 }
};

/* Exact #dspring table. */
static const char *const dspring[5][6] = {
    { "dspr11_t","dspr12_t","dspr13_t","dspr14_t","dspr15_t","dspr16_t" },
    { "dspr11_t","dspr22_t","dspr23_t","dspr24_t","dspr25_t","dspr26_t" },
    { "dspr21_t","dspr32_t","dspr33_t","dspr34_t","dspr35_t","dspr36_t" },
    { "dspr31_t","dspr42_t","dspr43_t","dspr44_t","dspr45_t","dspr46_t" },
    { "dspr41_t","dspr52_t","dspr53_t","dspr54_t","dspr55_t","dspr56_t" }
};

uint8_t wm_rope_side_lane_from_z_fp16(int32_t z)
{
    const int32_t t1 =
        (int32_t)(WM_RING_TOP + 1 * WM_ROPE_LANE_WIDTH) << 16;
    const int32_t t2 =
        (int32_t)(WM_RING_TOP + 2 * WM_ROPE_LANE_WIDTH) << 16;
    const int32_t t3 =
        (int32_t)(WM_RING_TOP + 3 * WM_ROPE_LANE_WIDTH) << 16;
    const int32_t t4 =
        (int32_t)(WM_RING_TOP + 4 * WM_ROPE_LANE_WIDTH) << 16;

    if (z < t1) return 0u;
    if (z < t2) return 1u;
    if (z < t3) return 2u;
    if (z < t4) return 3u;
    return 4u;
}

bool wm_rope_priority_accepts(
    uint16_t existing_priority,
    uint16_t incoming_priority)
{
    return existing_priority <= incoming_priority;
}

uint16_t wm_rope_second_half_z(
    uint16_t first_half_current_z,
    WmRopeZAction action)
{
    if (action == WM_ROPE_Z_HIGH) {
        return WM_ROPE_HIGH_SECOND_HALF_Z;
    }
    return first_half_current_z;
}

bool wm_rope_resolve_command(
    WmRopeBank bank,
    WmRopeAction action,
    uint8_t selector,
    int32_t wrestler_z_fp16,
    WmRopeCommand *out)
{
    const char *label = 0;
    uint16_t priority = 0u;
    uint8_t lane = 0xffu;

    if (out == 0 ||
        (unsigned)bank > WM_ROPE_RIGHT ||
        (unsigned)action >= WM_ROPE_COMMAND_COUNT) {
        return false;
    }

    /*
     * Exact command_table:
     * front: frontud_table,0,0,0,0,0
     * back : backud_table,0,0,0,0,0
     * sides: sideud, side_bounceio_t, sspring, dspring,
     *        sspr_trans_t, dspr_trans_t
     */
    if (bank == WM_ROPE_FRONT || bank == WM_ROPE_BACK) {
        if (action != WM_ROPE_BOUNCE_UD || selector >= 4u) {
            return false;
        }

        label = bank == WM_ROPE_FRONT ?
            front_ud[selector] : back_ud[selector];
        priority = WM_ROPE_SHAKE_PRIORITY;
    } else {
        switch (action) {
        case WM_ROPE_BOUNCE_UD:
            if (selector >= 4u) return false;
            label = side_ud[selector];
            priority = WM_ROPE_SHAKE_PRIORITY;
            break;

        case WM_ROPE_BOUNCE_IO:
            label = "side_bounceio_t";
            priority = WM_ROPE_SHAKE_PRIORITY;
            break;

        case WM_ROPE_SIDE_SPRING:
            if (selector >= 6u) return false;
            lane = wm_rope_side_lane_from_z_fp16(wrestler_z_fp16);
            label = sspring[lane][selector];
            if (label == 0) return false;
            priority = WM_ROPE_SIDE_SPRING_PRIORITY;
            break;

        case WM_ROPE_DOWN_SPRING:
            if (selector >= 6u) return false;
            lane = wm_rope_side_lane_from_z_fp16(wrestler_z_fp16);
            label = dspring[lane][selector];
            priority = WM_ROPE_DOWN_SPRING_PRIORITY;
            break;

        case WM_ROPE_SIDE_SPRING_RELEASE:
            label = "sspr_trans_t";
            priority = WM_ROPE_SIDE_SPRING_PRIORITY;
            break;

        case WM_ROPE_DOWN_SPRING_RELEASE:
            label = "dspr_trans_t";
            priority = WM_ROPE_DOWN_SPRING_PRIORITY;
            break;

        default:
            return false;
        }
    }

    memset(out, 0, sizeof(*out));
    out->bank = bank;
    out->action = action;
    out->source_script_table = label;
    out->priority = priority;
    out->lane = lane;
    out->selector = selector;
    return true;
}
