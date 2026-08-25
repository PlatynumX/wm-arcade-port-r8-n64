#include "wm/arcade_sound.h"
#include "wm/arcade_sound_tables.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { uint16_t cmd[512]; unsigned n; } sink;

static bool emit(void *u, uint16_t c, int8_t source_channel) {
    (void)source_channel;
    sink *s = (sink *)u;
    if (s->n >= 512u) return false;
    s->cmd[s->n++] = c;
    return true;
}

static uint32_t rng_zero(void *u, uint32_t max) {
    (void)u; (void)max; return 0u;
}

static uint32_t hcount_one(void *u) {
    (void)u;
    return 1u;
}

static uint16_t pd(unsigned pri, unsigned dur) {
    return (uint16_t)(((pri & 0xffu) << 8) | (dur & 0xffu));
}

static void test_bootstrap(void) {
    wm_arcade_sound s;
    sink out = {{0},0};
    wm_arcade_sound_init(&s, emit, &out, rng_zero, NULL);
    wm_arcade_sound_bind_default_tables(&s);

    wm_sound_result r = wm_sound_triple_sound(&s, 1);
    assert(r.played && r.channel == 1 && r.duration == 17);
    assert(out.n == 1 && out.cmd[0] == 0x80);

    /* DCS channel variants are source base + logical channel offset. */
    r = wm_sound_triple_sound(&s, 2);
    assert(r.played && r.channel == 2);
    assert(out.cmd[out.n - 1] == 0x85); /* 0x84 + channel-2 offset 1 */

    /* Fill all channels with higher priority, then reject lower call. */
    for (unsigned i = 0; i < WM_SOUND_CHANNELS; ++i) {
        s.channel[i].active = true;
        s.channel[i].duration = 100;
        s.channel[i].priority = WM_SOUND_PRI_SYSTEM3;
    }
    r = wm_sound_triple_sound(&s, 1);
    assert(!r.played && s.rejected_by_priority == 1);

    unsigned before = out.n;
    wm_sound_kill_all_channels(&s);
    assert(out.n == before + 4u);
    assert(out.cmd[before+0] == 994u && out.cmd[before+3] == 997u);
    for (unsigned i = 0; i < WM_SOUND_CHANNELS; ++i) assert(!s.channel[i].active);

    /* WALK_SOUND must use a bound HCOUNT source, not invented frame parity. */
    wm_arcade_sound_bind_hcount(&s, hcount_one, NULL);
    for (unsigned i = 0; i < WM_SOUND_CHANNELS; ++i) {
        s.channel[i].active = false;
        s.channel[i].priority = 0;
        s.channel[i].duration = 0;
    }
    before = out.n;
    r = wm_sound_walk(&s);
    assert(r.played && out.n == before + 1u && out.cmd[before] == 432u); /* source channel 1 uses base command unchanged */

    /* Source SNDSND command zero is valid and must reach the backend. */
    before = out.n;
    wm_sound_nosounds(&s);
    assert(out.n == before + 1u && out.cmd[before] == 0u);
}

static void test_announcer_queue(void) {
    wm_sound_entry table[0x303];
    memset(table, 0, sizeof(table));
    table[0xe0] = (wm_sound_entry){pd(WM_SOUND_PRI_ANNCER, 5), 3000, WM_SOUND_NO_SCRIPT, 0};
    table[0xe1] = (wm_sound_entry){pd(WM_SOUND_PRI_ANNCER, 7), 3100, WM_SOUND_NO_SCRIPT, 0};
    table[0x1fb] = (wm_sound_entry){pd(WM_SOUND_PRI_ANNCER, 8), 3200, WM_SOUND_NO_SCRIPT, 0};
    table[0x2b4] = (wm_sound_entry){pd(WM_SOUND_PRI_ANNCER, 9), 3300, WM_SOUND_NO_SCRIPT, 0};

    wm_arcade_sound s;
    sink out = {{0},0};
    wm_arcade_sound_init(&s, emit, &out, rng_zero, NULL);
    wm_arcade_sound_bind_tables(&s, table, 0x303, NULL, 0, NULL, 0,
        (wm_sound_wrestler_matrix){0}, NULL, 0, NULL, 0);

    assert(wm_sound_who_is_it(0xdf) == WM_SOUND_ANNOUNCER_NONE);
    assert(wm_sound_who_is_it(0xe0) == WM_SOUND_ANNOUNCER_VINCE);
    assert(wm_sound_who_is_it(0x1fb) == WM_SOUND_ANNOUNCER_HOWARD);
    assert(wm_sound_who_is_it(0x2b4) == WM_SOUND_ANNOUNCER_RANDY);
    assert(wm_sound_who_is_it(0x250) == WM_SOUND_ANNOUNCER_NONE);

    wm_sound_result r = wm_sound_announcer_sound(&s, 0xe0);
    assert(r.played && r.channel == 1 && s.vince_channel == 1 && s.vince_duration == 5);
    unsigned before = out.n;
    r = wm_sound_announcer_sound(&s, 0xe1);
    assert(r.played && r.channel == 1 && r.duration == 7);
    assert(out.n == before + 1u && out.cmd[before] == 3100u);

    wm_sound_reset_voice_queue(&s);
    s.vince_duration = 0;
    assert(wm_sound_add_voice(&s, 0xe0));
    assert(wm_sound_add_voice(&s, 0x1fb));
    assert(s.announce_count == 2);
    wm_sound_service_voice_queue(&s);
    assert(s.announce_count == 1 && s.voice_wait_ticks == 5);
    for (unsigned i = 0; i < 5; ++i) wm_arcade_sound_tick(&s);
    assert(s.announce_count == 0 && s.voice_wait_ticks == 8);
}

static void test_script_and_random(void) {
    static const uint16_t script_words[] = {
        (9u << 8) | WM_SOUND_SCRIPT_SETPRI,
        (0x44u << 8) | WM_SOUND_SCRIPT_SEND,
        (3u << 8) | WM_SOUND_SCRIPT_SLEEP,
        WM_SOUND_SCRIPT_END
    };
    static const wm_sound_script scripts[] = {{script_words, 4}};
    static const uint16_t random_vals[] = {2,3};
    static const wm_sound_random_table random_tables[] = {{random_vals, 2}};

    wm_sound_entry table[8];
    memset(table, 0, sizeof(table));
    table[1] = (wm_sound_entry){pd(4,1), 100, 0, 0};
    table[2] = (wm_sound_entry){pd(5,4), 200, WM_SOUND_NO_SCRIPT, 0};
    table[3] = (wm_sound_entry){pd(5,4), 300, WM_SOUND_NO_SCRIPT, 0};

    wm_arcade_sound s;
    sink out = {{0},0};
    wm_arcade_sound_init(&s, emit, &out, rng_zero, NULL);
    wm_arcade_sound_bind_tables(&s, table, 8, scripts, 1, random_tables, 1,
        (wm_sound_wrestler_matrix){0}, NULL, 0, NULL, 0);

    wm_sound_result r = wm_sound_triple_sound(&s, 1);
    assert(r.played && s.channel[0].priority == 9 && s.channel[0].duration == 3);
    assert(out.n == 2 && out.cmd[0] == 100 && out.cmd[1] == 0x44);
    wm_arcade_sound_tick(&s);
    wm_arcade_sound_tick(&s);
    wm_arcade_sound_tick(&s);
    assert(!s.channel[0].active);

    r = wm_sound_table_sound(&s, 0x1000);
    assert(r.played && r.duration == 4);
    assert(out.cmd[out.n - 1] == 200);
}

static void test_volume_and_repeat(void) {
    wm_arcade_sound s;
    sink out = {{0},0};
    wm_arcade_sound_init(&s, emit, &out, rng_zero, NULL);
    wm_arcade_sound_bind_default_tables(&s);

    wm_sound_set_lower_vol(&s, 2, 0x7f);
    assert(out.n == 2);
    assert(out.cmd[0] == 0x55adu);
    assert(out.cmd[1] == 0x7f80u);

    wm_sound_set_volume(&s, 0xff);
    assert(out.cmd[out.n - 2] == 0x55aau);
    assert(out.cmd[out.n - 1] == 0xff00u);

    wm_sound_begin_fade(&s, -1, 2);
    wm_arcade_sound_tick(&s);
    assert(s.master_volume < 0xffu);
    wm_arcade_sound_tick(&s);
    assert(s.master_volume == 0u && !s.fade_active);

    s.last_voice[0] = 0x123;
    s.repeat_state = 3;
    wm_sound_clear_speech_repeat(&s);
    assert(s.repeat_state == 0 && s.which_last_voice == 0 && s.last_voice[0] == 0);
}


static void test_source_process_wrappers(void) {
    static const int16_t one_voice[] = {1};
    wm_sound_speech_table speech[WM_SOUND_SPEECH_COUNT];
    memset(speech, 0, sizeof(speech));
    speech[WM_SOUND_SPEECH_MISSES] = (wm_sound_speech_table){0, -1, 1, 1, one_voice};
    speech[WM_SOUND_SPEECH_SPECIAL_MOVE] = (wm_sound_speech_table){0, -1, 1, 1, one_voice};

    wm_sound_entry table[2];
    memset(table, 0, sizeof(table));
    table[1] = (wm_sound_entry){pd(WM_SOUND_PRI_ANNCER, 2), 123, WM_SOUND_NO_SCRIPT, 0};

    wm_arcade_sound s;
    sink out = {{0},0};
    wm_arcade_sound_init(&s, emit, &out, rng_zero, NULL);
    wm_arcade_sound_bind_tables(&s, table, 2, NULL, 0, NULL, 0,
        (wm_sound_wrestler_matrix){0}, speech, WM_SOUND_SPEECH_COUNT, NULL, 0);

    assert(wm_sound_call_misses(&s));
    for (unsigned i = 0; i < 4; ++i) wm_arcade_sound_tick(&s);
    assert(out.n == 0);
    wm_arcade_sound_tick(&s);
    assert(out.n == 1 && out.cmd[0] == 123u);

    wm_sound_reset_voice_queue(&s);
    wm_sound_clear_speech_repeat(&s);
    for (unsigned i = 0; i < WM_SOUND_CHANNELS; ++i) {
        s.channel[i].active = false;
        s.channel[i].priority = 0;
        s.channel[i].duration = 0;
    }
    out.n = 0;
    assert(wm_sound_call_special_move(&s, 0));
    for (unsigned i = 0; i < 9; ++i) wm_arcade_sound_tick(&s);
    assert(out.n == 0);
    wm_arcade_sound_tick(&s);
    assert(out.n == 1 && out.cmd[0] == 123u);
}

int main(void) {
    test_bootstrap();
    test_announcer_queue();
    test_script_and_random();
    test_volume_and_repeat();
    test_source_process_wrappers();
    puts("arcade_sound: PASS");
    return 0;
}
