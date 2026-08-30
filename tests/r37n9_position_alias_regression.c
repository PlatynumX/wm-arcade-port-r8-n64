#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "wm_arcade_combat.h"
#include "wm_arcade_attach_anim.h"

static int32_t make_fixed(int16_t integer, uint16_t fraction)
{
    return (int32_t)(((uint32_t)(uint16_t)integer << 16) | fraction);
}

static int16_t fixed_integer(int32_t fixed)
{
    return (int16_t)((uint32_t)fixed >> 16);
}

static uint16_t fixed_fraction(int32_t fixed)
{
    return (uint16_t)((uint32_t)fixed & 0xffffu);
}

static void test_overlap_x_preserves_fraction(void)
{
    wm_arcade_actor_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.x_int = 100;
    a.z_int = 100;
    a.x_fixed = make_fixed(100, 0x1234u);
    a.z_fixed = make_fixed(100, 0x5678u);
    a.player_mode = WM_PMODE_NORMAL;
    b.player_mode = WM_PMODE_NORMAL;
    a.hurt_box.x1 = 90; a.hurt_box.x2 = 110;
    b.hurt_box.x1 = 105; b.hurt_box.x2 = 125;
    a.hurt_box.y1 = b.hurt_box.y1 = 0;
    a.hurt_box.y2 = b.hurt_box.y2 = 20;
    a.hurt_box.z1 = 0; a.hurt_box.z2 = 20;
    b.hurt_box.z1 = 5; b.hurt_box.z2 = 25;

    assert(wm_arcade_resolve_overlap(&a, &b) == 1);
    assert(a.x_int == 95);
    assert(fixed_integer(a.x_fixed) == 95);
    assert(fixed_fraction(a.x_fixed) == 0x1234u);
    assert(fixed_fraction(a.z_fixed) == 0x5678u);
}

static void test_overlap_z_preserves_fraction(void)
{
    wm_arcade_actor_t a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.x_int = 100;
    a.z_int = 100;
    a.x_fixed = make_fixed(100, 0x1357u);
    a.z_fixed = make_fixed(100, 0x2468u);
    a.player_mode = WM_PMODE_NORMAL;
    b.player_mode = WM_PMODE_NORMAL;
    a.hurt_box.x1 = 0; a.hurt_box.x2 = 30;
    b.hurt_box.x1 = 10; b.hurt_box.x2 = 40;
    a.hurt_box.y1 = b.hurt_box.y1 = 0;
    a.hurt_box.y2 = b.hurt_box.y2 = 20;
    a.hurt_box.z1 = 0; a.hurt_box.z2 = 10;
    b.hurt_box.z1 = 5; b.hurt_box.z2 = 15;

    assert(wm_arcade_resolve_overlap(&a, &b) == 1);
    assert(a.z_int == 95);
    assert(fixed_integer(a.z_fixed) == 95);
    assert(fixed_fraction(a.z_fixed) == 0x2468u);
    assert(fixed_fraction(a.x_fixed) == 0x1357u);
}

static void test_master_keep_attached_updates_integer_aliases(void)
{
    wm_arcade_actor_t master, slave;
    memset(&master, 0, sizeof(master));
    memset(&slave, 0, sizeof(slave));
    master.attach_proc = &slave;
    slave.attach_proc = &master;
    master.facing_dir = WM_MOVE_RIGHT;
    master.attach_xoff = 10;
    master.attach_yoff = -20;
    master.attach_zoff = 5;
    master.x_fixed = make_fixed(100, 0x1111u);
    master.y_fixed = make_fixed(200, 0x2222u);
    master.z_fixed = make_fixed(300, 0x3333u);
    slave.y_fixed = make_fixed(1, 0);
    slave.ground_y = 0;

    assert(wm_arcade_master_keep_attached(&master) == WM_ATTACH_OK);
    assert(slave.x_int == fixed_integer(slave.x_fixed));
    assert(slave.y_int == fixed_integer(slave.y_fixed));
    assert(slave.z_int == fixed_integer(slave.z_fixed));
    assert(slave.x_int == 110);
    assert(slave.y_int == 180);
    assert(slave.z_int == 305);
    assert(fixed_fraction(slave.x_fixed) == 0x1111u);
    assert(fixed_fraction(slave.y_fixed) == 0x2222u);
    assert(fixed_fraction(slave.z_fixed) == 0x3333u);
}

static void test_keep_attached_updates_integer_aliases(void)
{
    wm_arcade_actor_t master, slave;
    memset(&master, 0, sizeof(master));
    memset(&slave, 0, sizeof(slave));
    master.attach_proc = &slave;
    slave.attach_proc = &master;
    master.facing_dir = WM_MOVE_LEFT;
    master.attach_xoff = 12;
    master.attach_yoff = 7;
    master.attach_zoff = -3;
    master.x_fixed = make_fixed(200, 0xaaaau);
    master.y_fixed = make_fixed(80, 0xbbbbu);
    master.z_fixed = make_fixed(50, 0xccccu);

    assert(wm_arcade_keep_attached(&slave) == WM_ATTACH_OK);
    assert(slave.x_int == 188);
    assert(slave.y_int == 87);
    assert(slave.z_int == 47);
    assert(slave.x_int == fixed_integer(slave.x_fixed));
    assert(slave.y_int == fixed_integer(slave.y_fixed));
    assert(slave.z_int == fixed_integer(slave.z_fixed));
}

int main(void)
{
    test_overlap_x_preserves_fraction();
    test_overlap_z_preserves_fraction();
    test_master_keep_attached_updates_integer_aliases();
    test_keep_attached_updates_integer_aliases();
    return 0;
}
