#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_wrestler_port.h"

static void assert_actor_equal(const wm_arcade_actor_t *a, const wm_arcade_actor_t *b)
{
    assert(memcmp(a, b, sizeof(*a)) == 0);
}

static const wm_arcade_input_pattern_t *find_pattern(
    const wm_arcade_wrestler_profile_t *p, const char *label)
{
    size_t i;
    for (i = 0; i < p->secret_count; ++i) {
        if (strcmp(p->secrets[i].source_label, label) == 0) return &p->secrets[i];
    }
    return NULL;
}

int main(void)
{
    const wm_arcade_wrestler_profile_t *p;
    wm_arcade_wrestler_port_bindings_t bindings;
    wm_arcade_roster_env_t env = {1000, 0, 0, 0, 0};
    wm_arcade_bret_env_t be = {1000, 0, 0, 0, 0};
    wm_arcade_razor_env_t re = {1000, 0, 0, 0, 0};
    wm_arcade_actor_t a1, a2, o1, o2;
    wm_arcade_bret_step_result_t br;
    wm_arcade_razor_step_result_t rr;
    wm_arcade_roster_step_result_t ur;
    wm_arcade_taker_step_result_t tr;
    const wm_arcade_input_pattern_t *pat;

    memset(&bindings, 0, sizeof(bindings));

    /* One unified roster surface now exposes all eight live arcade wrestlers. */
    assert(wm_arcade_roster_profile(WM_ROSTER_BRET) == &wm_arcade_profile_bret);
    assert(wm_arcade_roster_profile(WM_ROSTER_RAZOR) == &wm_arcade_profile_razor);
    assert(wm_arcade_roster_profile(WM_ROSTER_TAKER) == &wm_arcade_profile_taker);
    assert(wm_arcade_roster_profile(WM_ROSTER_YOKO) == &wm_arcade_profile_yoko);
    assert(wm_arcade_roster_profile(WM_ROSTER_SHAWN) == &wm_arcade_profile_shawn);
    assert(wm_arcade_roster_profile(WM_ROSTER_BAM) == &wm_arcade_profile_bam);
    assert(wm_arcade_roster_profile(WM_ROSTER_DOINK) == &wm_arcade_profile_doink);
    assert(wm_arcade_roster_profile(WM_ROSTER_LEX) == &wm_arcade_profile_lex);
    assert(wm_arcade_roster_profile((wm_arcade_roster_id_t)7) == NULL);

    p = &wm_arcade_profile_bret;
    assert(strcmp(p->source_file, "BRET.ASM") == 0);
    assert(p->source_lines == 2973);
    assert(p->charge_button == WM_BTN_SPUNCH && p->charge_ticks == 100);
    pat = find_pattern(p, "supercut");
    assert(pat != NULL && pat->step_count == 3 && pat->max_ticks == 16);
    assert(pat->steps[0].value == WM_B_PUNCH);

    p = &wm_arcade_profile_razor;
    assert(strcmp(p->source_file, "RAZOR.ASM") == 0);
    assert(p->source_lines == 2713);
    assert(p->charge_button == WM_BTN_SKICK && p->charge_ticks == 85);
    pat = find_pattern(p, "down_slash");
    assert(pat != NULL && pat->step_count == 4 && pat->max_ticks == 50);

    /* Bret: unified dispatch delegates to the direct BRET.ASM translation. */
    memset(&a1, 0, sizeof(a1)); memset(&o1, 0, sizeof(o1));
    a1.player_mode = WM_PMODE_RUNNING;
    a1.move_dir = WM_MOVE_RIGHT; a1.facing_dir = WM_MOVE_RIGHT;
    a1.new_facing_dir = WM_MOVE_RIGHT; a1.but_val_down = 0;
    a1.getup_time = 0; a1.run_time = 9; o1.player_mode = WM_PMODE_NORMAL;
    a2 = a1; o2 = o1;
    br = wm_arcade_move_bret(&a1, &o1, &be, NULL);
    ur = wm_arcade_move_ported_wrestler(&wm_arcade_profile_bret, &a2, &o2,
                                         &env, &bindings);
    assert(ur == (br == WM_BRET_STEP_ACTION ? WM_ROSTER_STEP_ACTION :
                  br == WM_BRET_STEP_EXTERNAL ? WM_ROSTER_STEP_EXTERNAL :
                  WM_ROSTER_STEP_IDLE));
    assert_actor_equal(&a1, &a2);
    assert_actor_equal(&o1, &o2);

    /* Razor: same proof against the direct RAZOR.ASM translation. */
    memset(&a1, 0, sizeof(a1)); memset(&o1, 0, sizeof(o1));
    a1.player_mode = WM_PMODE_RUNNING;
    a1.move_dir = WM_MOVE_RIGHT; a1.facing_dir = WM_MOVE_RIGHT;
    a1.new_facing_dir = WM_MOVE_RIGHT; a1.but_val_down = 0;
    a1.getup_time = 0; a1.run_time = 11; o1.player_mode = WM_PMODE_NORMAL;
    a2 = a1; o2 = o1;
    rr = wm_arcade_move_razor(&a1, &o1, &re, NULL);
    ur = wm_arcade_move_ported_wrestler(&wm_arcade_profile_razor, &a2, &o2,
                                         &env, &bindings);
    assert(ur == (rr == WM_RZR_STEP_ACTION ? WM_ROSTER_STEP_ACTION :
                  rr == WM_RZR_STEP_EXTERNAL ? WM_ROSTER_STEP_EXTERNAL :
                  WM_ROSTER_STEP_IDLE));
    assert_actor_equal(&a1, &a2);
    assert_actor_equal(&o1, &o2);

    /* Undertaker now has the same direct-module proof as Bret/Razor. */
    memset(&a1, 0, sizeof(a1)); memset(&o1, 0, sizeof(o1));
    a1.player_mode = WM_PMODE_RUNNING;
    a1.move_dir = WM_MOVE_RIGHT; a1.facing_dir = WM_MOVE_RIGHT;
    a1.new_facing_dir = WM_MOVE_RIGHT; a1.getup_time = 0;
    a2 = a1; o2 = o1;
    tr = wm_arcade_move_taker(&a1, &o1, &env, NULL);
    ur = wm_arcade_move_ported_wrestler(&wm_arcade_profile_taker, &a2, &o2,
                                         &env, &bindings);
    assert(ur == (tr == WM_TAKER_STEP_ACTION ? WM_ROSTER_STEP_ACTION :
                  tr == WM_TAKER_STEP_EXTERNAL ? WM_ROSTER_STEP_EXTERNAL :
                  WM_ROSTER_STEP_IDLE));
    assert_actor_equal(&a1, &a2);
    assert_actor_equal(&o1, &o2);

    /* Source-labelled charge entry points route into the exact direct bodies. */
    memset(&a1, 0, sizeof(a1)); memset(&o1, 0, sizeof(o1));
    a1.player_mode = WM_PMODE_NORMAL; o1.player_mode = WM_PMODE_NORMAL;
    assert(wm_arcade_port_release_charge(&wm_arcade_profile_bret, &a1, &o1,
                                          "hrt_charge_flying_kick", 99,
                                          &bindings) == 0);
    assert(wm_arcade_port_release_charge(&wm_arcade_profile_bret, &a1, &o1,
                                          "hrt_charge_flying_kick", 100,
                                          &bindings) == 1);
    assert(a1.player_mode == WM_PMODE_INAIR);
    assert(a1.special_move_addr == (uintptr_t)WM_BRET_ANIM_FLYING_KICK);

    memset(&a1, 0, sizeof(a1));
    a1.player_mode = WM_PMODE_NORMAL;
    assert(wm_arcade_port_release_charge(&wm_arcade_profile_razor, &a1, NULL,
                                          "rzr_charge_slashes", 99,
                                          &bindings) == 0);
    assert(wm_arcade_port_release_charge(&wm_arcade_profile_razor, &a1, NULL,
                                          "rzr_charge_slashes", 100,
                                          &bindings) == 1);
    assert(a1.special_move_addr == (uintptr_t)WM_RZR_ANIM_REPEAT_SLASH);

    /* Literal source labels are not silently accepted when they belong elsewhere. */
    assert(wm_arcade_port_fire_secret(&wm_arcade_profile_bret, &a1, NULL,
                                       "down_slash", 1000, &bindings) == 0);
    assert(wm_arcade_port_release_charge(&wm_arcade_profile_razor, &a1, NULL,
                                          "hrt_charge_face_rake", 100,
                                          &bindings) == 0);

    puts("Stage 22 unified direct-port roster/dispatch tests: PASS");
    return 0;
}
