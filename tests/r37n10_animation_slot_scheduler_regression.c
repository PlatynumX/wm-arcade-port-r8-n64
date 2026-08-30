#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "wm_arcade_source_animation_runtime.h"

static wm_source_anim_ins_t cmd(int opcode)
{
    wm_source_anim_ins_t i;
    memset(&i, 0, sizeof(i));
    i.kind = WM_SRC_INS_CMD;
    i.opcode = (int16_t)opcode;
    return i;
}

static wm_source_anim_ins_t cmd1(int opcode, int32_t value)
{
    wm_source_anim_ins_t i = cmd(opcode);
    i.argc = 1;
    i.a[0].kind = WM_SRC_ARG_NUM;
    i.a[0].value = value;
    return i;
}

static wm_source_anim_ins_t frame(const char *name, uint16_t ticks)
{
    wm_source_anim_ins_t i;
    memset(&i, 0, sizeof(i));
    i.kind = WM_SRC_INS_FRAME;
    i.name = name;
    i.ticks = ticks;
    return i;
}

static void test_secondary_primary_mode_ownership(void)
{
    wm_arcade_actor_t a;
    wm_source_anim_runtime_t s;
    wm_source_anim_ins_t ins[2];
    wm_source_anim_program_t p;

    memset(&a, 0, sizeof(a));
    memset(&p, 0, sizeof(p));
    a.ani_speed = 0x100u;
    a.anim_mode = (uint16_t)(WM_ARCADE_MODE_NOGRAVITY | WM_ARCADE_MODE_STATUS);
    a.ani_count = 77;

    ins[0] = cmd(15); /* _ani_gravity_on uses a13(ANIMODE), i.e. primary. */
    ins[1] = cmd(73); /* _ani_end uses a10(OANIMODE), i.e. current slot. */
    p.ins = ins;
    p.count = 2;

    wm_source_anim_runtime_init(&s);
    wm_source_anim_runtime_set_secondary(&s, true);
    s.program = &p;
    wm_source_anim_runtime_tick(&s, &a);

    assert((a.anim_mode & WM_ARCADE_MODE_NOGRAVITY) == 0u);
    assert((a.anim_mode & WM_ARCADE_MODE_STATUS) != 0u);
    assert((a.anim_mode & WM_ARCADE_MODE_END) == 0u);
    assert(a.ani_count == 77);
    assert((wm_source_anim_runtime_slot_mode(&s, &a) & WM_ARCADE_MODE_END) != 0u);
}

static void test_setmode_targets_secondary_slot(void)
{
    wm_arcade_actor_t a;
    wm_source_anim_runtime_t s;
    wm_source_anim_ins_t ins[2];
    wm_source_anim_program_t p;
    uint16_t primary;

    memset(&a, 0, sizeof(a));
    memset(&p, 0, sizeof(p));
    a.ani_speed = 0x100u;
    primary = (uint16_t)(WM_ARCADE_MODE_STATUS | WM_ARCADE_MODE_CHECKHIT);
    a.anim_mode = primary;
    a.ani_count = 31;

    ins[0] = cmd1(2, WM_ARCADE_MODE_UNINT);
    ins[1] = cmd(73);
    p.ins = ins;
    p.count = 2;

    wm_source_anim_runtime_init(&s);
    wm_source_anim_runtime_set_secondary(&s, true);
    s.program = &p;
    wm_source_anim_runtime_tick(&s, &a);

    assert(a.anim_mode == primary);
    assert(a.ani_count == 31);
    assert((wm_source_anim_runtime_slot_mode(&s, &a) & WM_ARCADE_MODE_UNINT) != 0u);
    assert((wm_source_anim_runtime_slot_mode(&s, &a) & WM_ARCADE_MODE_END) != 0u);
}

static void test_zero_tick_frame_is_exact_slot_state(void)
{
    wm_arcade_actor_t a;
    wm_source_anim_runtime_t s;
    wm_source_anim_ins_t ins[2];
    wm_source_anim_program_t p;

    memset(&a, 0, sizeof(a));
    memset(&p, 0, sizeof(p));
    a.ani_speed = 0x100u;
    a.anim_mode = WM_ARCADE_MODE_STATUS;
    a.ani_count = 55;

    ins[0] = frame("R37N10_ZERO_TICK_FRAME", 0);
    ins[1] = cmd(73);
    p.ins = ins;
    p.count = 2;

    wm_source_anim_runtime_init(&s);
    wm_source_anim_runtime_set_secondary(&s, true);
    s.program = &p;
    wm_source_anim_runtime_tick(&s, &a);

    assert(wm_source_anim_runtime_frame(&s) == ins[0].name);
    assert(wm_source_anim_runtime_slot_count(&s, &a) == 0);
    assert(a.ani_count == 55);
    assert((wm_source_anim_runtime_slot_mode(&s, &a) & WM_ARCADE_MODE_END) == 0u);

    wm_source_anim_runtime_tick(&s, &a);
    assert((wm_source_anim_runtime_slot_mode(&s, &a) & WM_ARCADE_MODE_END) != 0u);
    assert((a.anim_mode & WM_ARCADE_MODE_END) == 0u);
}

int main(void)
{
    test_secondary_primary_mode_ownership();
    test_setmode_targets_secondary_slot();
    test_zero_tick_frame_is_exact_slot_state();
    return 0;
}
