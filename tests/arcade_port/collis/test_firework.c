/*
 * FIREWORK.ASM: the camera that flies a figure of eight over the
 * arena, the flares, the explosions, and the text that goes up once.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wm_arcade_firework.h"

/* 16.16, written so a negative value does not shift into the sign bit. */
#define FIX(v) ((int32_t)((uint32_t)(int32_t)(v) << 16))

/* ---- the tables -------------------------------------------------- */

static void test_panning_points(void) {
    size_t i;
    int32_t minx = 1 << 30, maxx = -(1 << 30);
    int32_t miny = 1 << 30, maxy = -(1 << 30);

    assert(wm_fw_panning_point_count == 61);
    /* The first row is INIT_PAN_SPEED; every other is 3. */
    assert(wm_fw_panning_points[0].ticks == 53);
    for (i = 1; i < wm_fw_panning_point_count; ++i) {
        assert(wm_fw_panning_points[i].ticks == 3);
    }
    for (i = 0; i < wm_fw_panning_point_count; ++i) {
        int32_t x = wm_fw_panning_points[i].x;
        int32_t y = wm_fw_panning_points[i].y;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
        /* Every Y is written EXP_FWY plus an offset in [-48,48]. */
        assert(y - WM_FW_EXP_FWY >= -48 && y - WM_FW_EXP_FWY <= 48);
    }
    assert(minx == 700 && maxx == 1000);
    assert(miny == -308 && maxy == -212);

    /* A figure of eight closes: the last waypoint is the first. */
    assert(wm_fw_panning_points[wm_fw_panning_point_count - 1].x ==
           wm_fw_panning_points[0].x);
    assert(wm_fw_panning_points[wm_fw_panning_point_count - 1].y ==
           wm_fw_panning_points[0].y);
    /* And it crosses itself: x=850,y=EXP_FWY appears more than twice. */
    {
        int crossings = 0;
        for (i = 0; i < wm_fw_panning_point_count; ++i) {
            if (wm_fw_panning_points[i].x == 850 &&
                wm_fw_panning_points[i].y == WM_FW_EXP_FWY) crossings++;
        }
        assert(crossings == 3);
    }
}

static void test_flare_positions(void) {
    int i;
    /* Twelve across the back of the ring, 50 apart from 798. */
    for (i = 0; i < 12; ++i) {
        assert(wm_fw_flare_positions[i].x == 798 + 50 * i);
        assert(wm_fw_flare_positions[i].y == 128);
    }
    /* Then five down each side, stepping 20 in Y. */
    for (i = 12; i < 17; ++i) {
        assert(wm_fw_flare_positions[i].y == 148 + 20 * (i - 12));
    }
    for (i = 17; i < 22; ++i) {
        assert(wm_fw_flare_positions[i].y == 148 + 20 * (i - 17));
    }
    /* The left side goes left and the right side goes right. */
    assert(wm_fw_flare_positions[12].x == 770);
    assert(wm_fw_flare_positions[16].x == 702);
    assert(wm_fw_flare_positions[17].x == 1372);
    assert(wm_fw_flare_positions[21].x == 1440);
}

static void test_image_lists(void) {
    /* #flare_anim2 is INSIDE #flare_anim. */
    assert(wm_fw_flare_anim_count == 13);
    assert(WM_FW_FLARE_ANIM2 == 5);
    assert(strcmp(wm_fw_flare_anim[0], "FWFLAR01") == 0);
    assert(strcmp(wm_fw_flare_anim[WM_FW_FLARE_ANIM2], "FWFLAR06") == 0);
    assert(strcmp(wm_fw_flare_anim[12], "FWFLAR13") == 0);

    assert(wm_fw_fwexa_anim_count == 11);
    assert(wm_fw_fwexb_anim_count == 11);
    assert(strcmp(wm_fw_fwexa_anim[0], "FWEXPA01") == 0);
    assert(strcmp(wm_fw_fwexb_anim[10], "FWEXPB11") == 0);

    assert(strcmp(wm_fw_palettes[0], "FWWHT_P") == 0);
    assert(strcmp(wm_fw_palettes[4], "FWYEL_P") == 0);
}

static void test_congrats_text(void) {
    int s;
    /* Every set opens with the same line at the same place. */
    for (s = 0; s < WM_FW_CONGRATS_SETS; ++s) {
        assert(wm_fw_congrats[s].count >= 4);
        assert(strcmp(wm_fw_congrats[s].lines[0].text,
                      "CONGRATULATIONS!!!") == 0);
        assert(wm_fw_congrats[s].lines[0].y == 60);
        assert(wm_fw_congrats[s].lines[0].x == 200);
        /* Every line is centred on x=200 by print_string_C2. */
        {
            size_t i;
            for (i = 0; i < wm_fw_congrats[s].count; ++i) {
                assert(wm_fw_congrats[s].lines[i].x == 200);
            }
        }
    }

    /* The ladder ending says intercontinental, not world. */
    assert(strcmp(wm_fw_congrats[WM_FW_CONGRATS_1V3].lines[1].text,
                  "YOU ARE THE INTERCONTINENTAL CHAMPION!") == 0);
    assert(strcmp(wm_fw_congrats[WM_FW_CONGRATS_1V8].lines[1].text,
                  "YOU ARE THE NEW WWF WORLD CHAMPION!") == 0);
    assert(strcmp(wm_fw_congrats[WM_FW_CONGRATS_2V8].lines[1].text,
                  "YOU ARE THE NEW WORLD WRESTLING") == 0);

    /*
     * The 1v3 set has two JAM_STR rows commented out and one put
     * back, so it skips y=126 and jumps 113 -> 152. The 1v8 set
     * uses 126 and skips 152. That asymmetry is in the source.
     */
    {
        const wm_fw_congrats_t *a = &wm_fw_congrats[WM_FW_CONGRATS_1V3];
        const wm_fw_congrats_t *b = &wm_fw_congrats[WM_FW_CONGRATS_1V8];
        assert(a->count == 5 && b->count == 5);
        assert(a->lines[3].y == 152);
        assert(b->lines[3].y == 126);
        assert(a->lines[4].y == 178 && b->lines[4].y == 178);
    }
    /* The tag ending is one line shorter and ends differently. */
    assert(wm_fw_congrats[WM_FW_CONGRATS_2V8].count == 4);
    assert(strcmp(wm_fw_congrats[WM_FW_CONGRATS_2V8].lines[3].text,
                  "PREPARE TO BATTLE EACH OTHER!!!") == 0);
}

static void test_congrats_choice(void) {
    /* royal_rumble wins outright; is_8_on_1 is only consulted after. */
    assert(wm_fw_congrats_index(true, false) == WM_FW_CONGRATS_2V8);
    assert(wm_fw_congrats_index(true, true) == WM_FW_CONGRATS_2V8);
    assert(wm_fw_congrats_index(false, true) == WM_FW_CONGRATS_1V8);
    assert(wm_fw_congrats_index(false, false) == WM_FW_CONGRATS_1V3);
}

/* ---- the camera -------------------------------------------------- */

static void test_calc_dxdy(void) {
    wm_fw_camera_t c;
    memset(&c, 0, sizeof c);

    c.p1x = FIX(100);
    c.p2x = FIX(130);
    c.p1y = 0;
    c.p2y = FIX(-30);
    c.ticks = 3;
    wm_fw_calc_dxdy(&c);
    assert(c.dx == FIX(10));
    assert(c.dy == -FIX(10));

    /* SIGNED divide: a backwards move gives a negative delta. */
    c.p1x = FIX(130);
    c.p2x = FIX(100);
    wm_fw_calc_dxdy(&c);
    assert(c.dx == -FIX(10));
}

static void test_check_camera_position(void) {
    wm_fw_camera_t c;
    memset(&c, 0, sizeof c);
    c.dx = 1000;
    c.dy = 1000;

    /* Exactly three away counts as arrived -- `cmpi 3 / jrgt`. */
    c.worldtlx = FIX(100);
    c.targ_x = 103;
    c.worldtly = FIX(100);
    c.targ_y = 104;
    wm_fw_check_camera_position(&c);
    assert(c.dx == 0);          /* 3 away: stopped */
    assert(c.dy == 1000);       /* 4 away: still moving */

    /* The axes are independent, and a negative Y floors rather than
       truncating toward zero. */
    memset(&c, 0, sizeof c);
    c.dy = -500;
    c.worldtly = -FIX(100) - 0x8000;   /* -100.5 */
    c.targ_y = -101;
    wm_fw_check_camera_position(&c);
    assert(c.dy == 0);          /* sra gives -101, exactly on target */
}

static void test_segment_done(void) {
    wm_fw_camera_t c;
    memset(&c, 0, sizeof c);
    assert(wm_fw_segment_done(&c));
    c.dx = 1;
    assert(!wm_fw_segment_done(&c));   /* either being non-zero */
    c.dx = 0;
    c.dy = -1;
    assert(!wm_fw_segment_done(&c));
}

/* ---- pan_around -------------------------------------------------- */

static void test_pan_start_sleeps_first(void) {
    wm_fw_pan_t p;
    int i;
    wm_fw_pan_begin(&p, FIX(850), FIX(WM_FW_EXP_FWY));
    /* `SLEEP TSEC/2` -- 26 ticks with the camera untouched. */
    for (i = 0; i < 26; ++i) {
        assert(wm_fw_pan_tick(&p, false, false));
        assert(p.cam.worldtlx == FIX(850));
        assert(!p.update_background);
    }
    /* The first move is the next tick. */
    assert(wm_fw_pan_tick(&p, false, false));
    assert(p.update_background);
    assert(p.cheer);
}

/*
 * The whole flight, with pan_down raised partway: the camera must
 * come home to where it started and then die.
 */
static void test_pan_flies_and_comes_home(void) {
    const int32_t x0 = FIX(900), y0 = FIX(-200);
    wm_fw_pan_t p;
    unsigned ticks = 0, prints = 0, deletes = 0;
    bool told_down = false;

    wm_fw_pan_begin(&p, x0, y0);
    while (wm_fw_pan_tick(&p, false, true)) {
        if (p.show_congrats) prints++;
        if (p.delete_text) deletes++;
        ticks++;
        /* Let it fly one lap, then call it home. */
        if (!told_down && p.laps >= 1) {
            wm_fw_pan_come_down(&p);
            told_down = true;
        }
        assert(ticks < 200000u);
    }

    assert(told_down);
    assert(p.phase == WM_FW_PAN_DONE);
    /* The text went up exactly once, and was deleted exactly once. */
    assert(prints == 1);
    assert(deletes == 1);
    assert(p.text_shown);
    assert(p.congrats_set == WM_FW_CONGRATS_1V8);

    /* Home, to within the three pixels check_camera_position allows. */
    {
        int32_t dx = (p.cam.worldtlx >> 16) - (x0 >> 16);
        int32_t dy = (p.cam.worldtly >> 16) - (y0 >> 16);
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        assert(dx <= 3);
        assert(dy <= 3);
    }
}

/* pan_down is only read at the end of a segment, so the camera never
   turns for home mid-segment. */
static void test_pan_down_finishes_the_segment(void) {
    wm_fw_pan_t p;
    unsigned ticks = 0;
    /* Start well away from waypoint 0 so the first segment is long. */
    wm_fw_pan_begin(&p, FIX(400), FIX(-100));
    /* Get past the initial sleep and into the first segment. */
    while (!p.update_background) {
        assert(wm_fw_pan_tick(&p, false, false));
        assert(++ticks < 1000u);
    }
    wm_fw_pan_come_down(&p);
    /* It is still flying the segment it was on, not the way home. */
    assert(p.phase == WM_FW_PAN_SEGMENT);
    assert(wm_fw_pan_tick(&p, false, false));
    assert(p.phase == WM_FW_PAN_SEGMENT || p.phase == WM_FW_PAN_TEXT_DELAY);
}

/* The text goes up once however many laps are flown. */
static void test_text_only_once(void) {
    wm_fw_pan_t p;
    unsigned ticks = 0, prints = 0;
    wm_fw_pan_begin(&p, FIX(850), FIX(WM_FW_EXP_FWY));
    while (wm_fw_pan_tick(&p, true, false)) {
        if (p.show_congrats) prints++;
        if (p.laps >= 3) { wm_fw_pan_come_down(&p); }
        assert(++ticks < 200000u);
    }
    assert(prints == 1);
    assert(p.congrats_set == WM_FW_CONGRATS_2V8);
    assert(p.laps >= 3);
}

/* ---- firework_flare ---------------------------------------------- */

static void test_flare_first_pass(void) {
    wm_fw_flare_t f;
    wm_fw_point_t pos;
    int i;
    int32_t seen[13];
    int n = 0;

    pos.x = 798;
    pos.y = 128;
    wm_fw_flare_begin(&f, pos, 1, 3);
    assert(f.x == 798 && f.y == 128);
    assert(f.palette == 3);

    /* PRCSLP(1) spends a whole tick; the object appears on the
       next one, not on it. */
    assert(wm_fw_flare_tick(&f, false));
    assert(!f.created);
    assert(f.phase == WM_FW_FLARE_DELAY);
    assert(wm_fw_flare_tick(&f, false));
    assert(f.created);
    assert(f.phase == WM_FW_FLARE_RUN);

    /* The object is already showing image zero. */
    assert(f.frame == 0);

    /* Then twelve more, two ticks apart, in order. */
    {
        int last_change = -1;
        for (i = 0; i < 200; ++i) {
            if (!wm_fw_flare_tick(&f, false)) break;
            /* The pass ends the moment the terminator sends it into
               the loop, and that tick already shows the next pass's
               first image -- so stop before recording it. */
            if (f.phase != WM_FW_FLARE_RUN) break;
            if (f.changed) {
                if (last_change >= 0) assert(i - last_change == 2);
                last_change = i;
                assert(n < 13);
                seen[n++] = f.frame;
            }
        }
    }
    assert(n == 12);
    for (i = 0; i < 12; ++i) assert(seen[i] == i + 1);
}

static void test_flare_loops_the_tail(void) {
    wm_fw_flare_t f;
    wm_fw_point_t pos;
    int i;
    int lowest = 99;

    pos.x = 0;
    pos.y = 0;
    wm_fw_flare_begin(&f, pos, 1, 0);
    /* Run a long time without ever asking it to fizzle. */
    for (i = 0; i < 500; ++i) {
        assert(wm_fw_flare_tick(&f, false));
        if (f.phase == WM_FW_FLARE_LOOP && f.changed && f.frame < lowest) {
            lowest = f.frame;
        }
    }
    /* Once looping it never goes below #flare_anim2. */
    assert(f.phase == WM_FW_FLARE_LOOP);
    assert(lowest == WM_FW_FLARE_ANIM2);
}

static void test_flare_fizzles_backwards(void) {
    wm_fw_flare_t f;
    wm_fw_point_t pos;
    int i;
    int32_t seen[8];
    int n = 0;

    pos.x = 0;
    pos.y = 0;
    wm_fw_flare_begin(&f, pos, 1, 0);
    /* Get it looping first. */
    for (i = 0; i < 200 && f.phase != WM_FW_FLARE_LOOP; ++i) {
        assert(wm_fw_flare_tick(&f, false));
    }
    assert(f.phase == WM_FW_FLARE_LOOP);

    /* Now tell it to fizzle and collect what it shows. */
    for (i = 0; i < 500; ++i) {
        if (!wm_fw_flare_tick(&f, true)) break;
        if (f.changed && f.phase == WM_FW_FLARE_FIZZLE) {
            assert(n < 8);
            seen[n++] = f.frame;
        }
    }
    assert(f.phase == WM_FW_FLARE_DEAD);
    /* Five images, index 4 down to 0 -- pre-decrement from
       #flare_anim2, stopping when it reaches #flare_anim. */
    assert(n == 5);
    for (i = 0; i < 5; ++i) assert(seen[i] == 4 - i);
}

/* ---- animate_fwexp ----------------------------------------------- */

static void test_explosion(void) {
    wm_fw_explosion_t e;
    int i;
    int32_t y0;

    /* The four draws at their extremes, with the source's offsets. */
    wm_fw_explosion_begin(&e, 0, 0, 0, 0, 0);
    assert(e.y == WM_FW_EXP_FWY - 48);
    assert(e.x == 850);
    assert(e.z == 0x700);

    wm_fw_explosion_begin(&e, 96, 350, 0x200, 1, 4);
    assert(e.y == WM_FW_EXP_FWY - 48 + 96);
    assert(e.x == 1200);
    assert(e.z == 0x900);
    assert(e.which == 1);

    /* Eleven images; ten changes after the one it was created with,
       and it drifts down four pixels each. */
    wm_fw_explosion_begin(&e, 0, 0, 0, 0, 0);
    y0 = e.y;
    for (i = 0; i < 10; ++i) {
        assert(wm_fw_explosion_tick(&e));
        assert(e.changed);
        assert(e.at == i + 1);
    }
    assert(e.y == y0 + 10 * WM_FW_EXPLOSION_DRIFT);
    /* The eleventh tick runs off the end and the object dies. */
    assert(!wm_fw_explosion_tick(&e));
    assert(e.dead);
    assert(!wm_fw_explosion_tick(&e));
}

/* ---- knockout_drones --------------------------------------------- */

static void test_knockout_drones(void) {
    wm_fw_process_t procs[5];
    size_t n;
    memset(procs, 0, sizeof procs);
    procs[0].plyr_type = 0;     /* a human */
    procs[1].plyr_type = 1;     /* a drone */
    procs[2].plyr_type = -1;    /* the referee is negative, and counts */
    procs[3].plyr_type = 0;
    procs[4].plyr_type = 2;

    n = wm_fw_knockout_drones(procs, 5);
    assert(n == 3);
    assert(procs[0].ptime == 0);
    assert(procs[1].ptime == WM_FW_DRONE_SLEEP);
    assert(procs[2].ptime == WM_FW_DRONE_SLEEP);
    assert(procs[3].ptime == 0);
    assert(procs[4].ptime == WM_FW_DRONE_SLEEP);
    /* Asleep, not deleted -- nothing here removes a process. */
    assert(WM_FW_DRONE_SLEEP == 0x7fff);
}

int main(void) {
    test_knockout_drones();
    test_panning_points();
    test_flare_positions();
    test_image_lists();
    test_congrats_text();
    test_congrats_choice();
    test_calc_dxdy();
    test_check_camera_position();
    test_segment_done();
    test_pan_start_sleeps_first();
    test_pan_flies_and_comes_home();
    test_pan_down_finishes_the_segment();
    test_text_only_once();
    test_flare_first_pass();
    test_flare_loops_the_tail();
    test_flare_fizzles_backwards();
    test_explosion();
    printf("firework ok\n");
    return 0;
}
