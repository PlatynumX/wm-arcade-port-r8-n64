/*
 * AWARD.ASM's powerup codes.
 */
#include "wm/arcade/wm_arcade_powerup.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const wm_powerup_code *code_named(const char *name)
{
    int i;
    for (i = 0; i < wm_powerup_code_count; ++i) {
        if (strcmp(wm_powerup_codes[i].name, name) == 0) {
            return &wm_powerup_codes[i];
        }
    }
    return NULL;
}

static void test_code_table(void)
{
    const wm_powerup_code *c;
    int i;

    assert(wm_powerup_code_count == 8);

    c = code_named("no_block");
    assert(c && c->steps == 3 && c->grants == WM_PU_BLOCKING_OFF);
    assert(c->blocked_in_royal_rumble && c->spawned);
    assert(c->step[0] == WM_PUP_BLOCK);

    c = code_named("buddy_mode");
    assert(c && c->steps == 5 && c->blocked_in_royal_rumble && c->spawned);

    /* no_block is BLOCK x3 and buddy_mode is BLOCK x5, and both listen
     * at once -- so five blocks turns on both. */
    assert(code_named("no_block")->step[0] ==
           code_named("buddy_mode")->step[0]);

    /* no_ring has a whole sequence and is never started: its CREATE is
     * commented out, "Disable until we get blimp module". */
    c = code_named("no_ring");
    assert(c && c->steps == 4 && !c->spawned);
    assert(c->step[0] == WM_PUP_UP && c->step[3] == WM_PUP_LEFT);

    /* drone_meters has no sequence at all -- its PUPWAITSWITCHes are
     * commented out -- so wherever it runs it just turns itself on. */
    c = code_named("drone_meters");
    assert(c && c->steps == 0 && c->spawned);

    /* Every code grants exactly one bit, and no two grant the same. */
    for (i = 0; i < wm_powerup_code_count; ++i) {
        uint32_t g = wm_powerup_codes[i].grants;
        int j;
        assert(g && (g & (g - 1u)) == 0u);
        for (j = i + 1; j < wm_powerup_code_count; ++j) {
            assert(wm_powerup_codes[j].grants != g);
        }
    }
}

static void test_the_first_press_has_no_deadline(void)
{
    wm_powerup_attempt att;
    int i;

    wm_powerup_attempt_start(&att, code_named("no_block"));
    for (i = 0; i < 10000; ++i) {
        assert(!wm_powerup_attempt_tick(&att, 0));
        assert(!att.dead);
    }
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_BLOCK));
    assert(att.at == 1 && att.timer == WM_PUP_WINDOW);
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_BLOCK));
    assert(wm_powerup_attempt_tick(&att, WM_PUP_BLOCK));
    assert(wm_powerup_attempt_done(&att));
}

static void test_a_wrong_press_does_not_restart_the_code(void)
{
    wm_powerup_attempt att;

    wm_powerup_attempt_start(&att, code_named("combos"));
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_PUNCH));   /* wrong */
    assert(att.at == 0 && !att.dead);
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_RIGHT));
    assert(att.at == 1);
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_KICK));    /* wrong */
    assert(att.at == 1);                                    /* not reset */
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_PUNCH));
    assert(wm_powerup_attempt_tick(&att, WM_PUP_SUPERP));

    /* A press must be exactly the value: two at once is neither. */
    wm_powerup_attempt_start(&att, code_named("combos"));
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_RIGHT | WM_PUP_PUNCH));
    assert(att.at == 0);
}

static void test_the_window_is_shared_not_per_press(void)
{
    wm_powerup_attempt att;
    int i;

    wm_powerup_attempt_start(&att, code_named("move_names"));
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_PUNCH));
    assert(att.timer == WM_PUP_WINDOW);

    for (i = 0; i < 50; ++i) {
        assert(!wm_powerup_attempt_tick(&att, 0));
    }
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_PUNCH));
    assert(att.at == 2);
    /* `movi TSEC*2,a11` ran once; the clock did NOT restart. */
    assert(att.timer == WM_PUP_WINDOW - 51);

    for (i = 0; i < WM_PUP_WINDOW; ++i) {
        if (wm_powerup_attempt_tick(&att, 0)) {
            break;
        }
    }
    assert(att.dead && !wm_powerup_attempt_done(&att));
    /* Once dead it stays dead. */
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_PUNCH));
    assert(att.at == 2);
}

static void test_a_code_entered_inside_the_window(void)
{
    /* buddy_mode is the longest: five presses, four of them inside one
     * 106-tick budget. */
    wm_powerup_attempt att;
    int i;
    int step;

    wm_powerup_attempt_start(&att, code_named("buddy_mode"));
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_BLOCK));
    for (step = 1; step < 4; ++step) {
        for (i = 0; i < 25; ++i) {
            assert(!wm_powerup_attempt_tick(&att, 0));
        }
        assert(!wm_powerup_attempt_tick(&att, WM_PUP_BLOCK));
    }
    assert(att.at == 4 && !att.dead);
    assert(wm_powerup_attempt_tick(&att, WM_PUP_BLOCK));
    assert(wm_powerup_attempt_done(&att));

    /* Twenty-seven ticks apart instead of twenty-six and the same
     * five presses run out of budget. */
    wm_powerup_attempt_start(&att, code_named("buddy_mode"));
    assert(!wm_powerup_attempt_tick(&att, WM_PUP_BLOCK));
    for (step = 1; step < 5 && !att.dead; ++step) {
        for (i = 0; i < 27 && !att.dead; ++i) {
            (void)wm_powerup_attempt_tick(&att, 0);
        }
        if (!att.dead) {
            (void)wm_powerup_attempt_tick(&att, WM_PUP_BLOCK);
        }
    }
    assert(att.dead && !wm_powerup_attempt_done(&att));
}

static void test_drone_meters_needs_no_input(void)
{
    wm_powerup_attempt att;
    wm_powerup_attempt_start(&att, code_named("drone_meters"));
    assert(wm_powerup_attempt_done(&att));
    assert(!wm_powerup_attempt_tick(&att, 0));
    assert(wm_powerup_attempt_done(&att));
}

static void test_get_powerups(void)
{
    wm_powerup_flags f;

    /* Anything in BOTH_P_MASK needs both players. */
    wm_powerup_reset(&f);
    f.p_request[0] = WM_PU_BLOCKING_OFF;
    wm_get_powerups(&f);
    assert(f.blocking_off == 0);
    assert(f.p_request[0] == 0);

    wm_powerup_reset(&f);
    f.p_request[0] = WM_PU_BLOCKING_OFF;
    f.p_request[1] = WM_PU_BLOCKING_OFF;
    wm_get_powerups(&f);
    /* Not 1 -- `movi 2020h,a8`. */
    assert(f.blocking_off == WM_BLOCKING_OFF_VALUE);
    assert(f.p_request[0] == WM_PU_BLOCKING_OFF);
    assert(f.p_request[1] == WM_PU_BLOCKING_OFF);

    /* D_METERS_ON is outside the mask: either player is enough, and it
     * stays on only the player who asked. */
    wm_powerup_reset(&f);
    f.p_request[1] = WM_PU_D_METERS_ON;
    wm_get_powerups(&f);
    assert(f.drone_meters_on == WM_PU_D_METERS_ON);
    assert(f.p_request[0] == 0);
    assert(f.p_request[1] == WM_PU_D_METERS_ON);

    /* MOVE_NAMES_ON is also outside the mask and has no output flag:
     * it stays on the requesting player's word, which is what
     * LIFEBAR.ASM reads per player. */
    wm_powerup_reset(&f);
    f.p_request[0] = WM_PU_MOVE_NAMES_ON;
    wm_get_powerups(&f);
    assert(f.p_request[0] == WM_PU_MOVE_NAMES_ON);
    assert(f.p_request[1] == 0);

    /* Three flags keep the bit; hyper_speed_on is normalised to 1. */
    wm_powerup_reset(&f);
    f.p_request[0] = WM_PU_COMBOS_ON | WM_PU_RING_OUTS_ON | WM_PU_NO_RING |
                     WM_PU_HYPER_MATCH_ON;
    f.p_request[1] = f.p_request[0];
    wm_get_powerups(&f);
    assert(f.instant_combos_on == WM_PU_COMBOS_ON);
    assert(f.ring_out_on == WM_PU_RING_OUTS_ON);
    assert(f.no_ring_on == WM_PU_NO_RING);
    assert(f.hyper_speed_on == 1);

    /* One player asking for a both-players option loses it, and the
     * one-player options either side asked for survive. */
    wm_powerup_reset(&f);
    f.p_request[0] = WM_PU_BUDDY_MODE | WM_PU_MOVE_NAMES_ON;
    f.p_request[1] = WM_PU_D_METERS_ON;
    wm_get_powerups(&f);
    assert(f.p_request[0] == WM_PU_MOVE_NAMES_ON);
    assert(f.p_request[1] == WM_PU_D_METERS_ON);
    assert(f.drone_meters_on == WM_PU_D_METERS_ON);
}

int main(void)
{
    test_code_table();
    test_the_first_press_has_no_deadline();
    test_a_wrong_press_does_not_restart_the_code();
    test_the_window_is_shared_not_per_press();
    test_a_code_entered_inside_the_window();
    test_drone_meters_needs_no_input();
    test_get_powerups();
    printf("AWARD.ASM powerup codes: all checks passed\n");
    return 0;
}
