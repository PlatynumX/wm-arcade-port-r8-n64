/*
 * LIFEBAR.ASM's combo meter and the perfect-match check.
 *
 * Three small routines that were open for three different reasons:
 * clear_combo_meter and halve_combo_meter share a tail and so read as
 * one routine with two constants, and is_perfect answers through the
 * carry flag with the final battle as a special case that beats the
 * data.
 */
#include "wm/arcade/wm_arcade_combo.h"
#include "wm/arcade/wm_arcade_lifebar.h"
#include "wm/arcade/wm_arcade_life_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_the_two_meter_writers(void)
{
    wm_arcade_actor_t a;

    memset(&a, 0, sizeof(a));

    /* Fill it past the super threshold the way the game does. */
    while (a.combo_size < WM_COMBO_SUPER_SIZE)
        wm_arcade_add_to_combo_count(&a, 1);
    assert(a.combo_size == WM_COMBO_SUPER_SIZE);
    assert(a.combo_flash == 1);

    /* LIFEBAR.ASM:5291 halve_combo_meter's `movk 10,a2`. Ten is not
       half of sixteen; it is just the number the source writes, and it
       is below the threshold, so the flash goes out. */
    wm_arcade_halve_combo_meter(&a);
    assert(a.combo_size == WM_COMBO_HALF_SIZE);
    assert(a.combo_size == 10);
    assert(a.combo_size < WM_COMBO_SUPER_SIZE);
    assert(a.combo_flash == 0);

    /* LIFEBAR.ASM:5280 clear_combo_meter, through zero_combo_meter. */
    a.combo_flash = 1;
    wm_arcade_clear_combo_meter(&a);
    assert(a.combo_size == 0);
    assert(a.combo_flash == 0);

    wm_arcade_clear_combo_meter(NULL);
    wm_arcade_halve_combo_meter(NULL);
}

static void test_is_perfect(void)
{
    wm_arcade_actor_t me, mate, foe;
    const wm_arcade_actor_t *actors[4];

    memset(&me, 0, sizeof(me));
    memset(&mate, 0, sizeof(mate));
    memset(&foe, 0, sizeof(foe));
    me.player_side = 0;
    mate.player_side = 0;
    foe.player_side = 1;
    me.life = WM_LIFE_MAX;
    mate.life = WM_LIFE_MAX;
    foe.life = 0;

    actors[0] = &me;
    actors[1] = &mate;
    actors[2] = &foe;
    actors[3] = NULL;    /* an inactive process -- `jrz #nxt` */

    /* Untouched on my side, and the enemy's zero life is irrelevant. */
    assert(wm_arcade_is_perfect(&me, actors, 4, false));

    /* One scratch on a teammate and it is gone. */
    mate.life = WM_LIFE_MAX - 1;
    assert(!wm_arcade_is_perfect(&me, actors, 4, false));
    mate.life = WM_LIFE_MAX;
    assert(wm_arcade_is_perfect(&me, actors, 4, false));

    /* And on the winner himself. */
    me.life = WM_LIFE_MAX - 1;
    assert(!wm_arcade_is_perfect(&me, actors, 4, false));
    me.life = WM_LIFE_MAX;

    /*
     * `calla is_8_on_1 / jrc #final` reaches a bare `clrc`, so nobody
     * is ever perfect in the final battle however healthy they are.
     * The data says yes and the answer is still no.
     */
    assert(wm_arcade_is_perfect(&me, actors, 4, false));
    assert(!wm_arcade_is_perfect(&me, actors, 4, true));

    assert(!wm_arcade_is_perfect(NULL, actors, 4, false));
    assert(!wm_arcade_is_perfect(&me, NULL, 4, false));
}

int main(void)
{
    test_the_two_meter_writers();
    test_is_perfect();
    printf("lifebar combo meter and is_perfect: all checks passed\n");
    return 0;
}
