#include "wm/arcade_sound.h"

#include <string.h>

#define WM_SOUND_STOP_CH1 994u
#define WM_SOUND_STOP_CH2 995u
#define WM_SOUND_STOP_CH3 996u
#define WM_SOUND_STOP_CH4 997u
#define WM_SOUND_VOL_MASTER 0x55AAu
#define WM_SOUND_VOL_CH0    0x55ABu

/* Source announcer table boundaries. */
#define WM_SOUND_ANNOUNCER_START 0x00E0u
#define WM_SOUND_VINCE_END       0x01D0u
#define WM_SOUND_RANDY_END       0x01FBu
#define WM_SOUND_HOWARD_END      0x0200u
#define WM_SOUND_MORE_JERRY      0x02B0u
#define WM_SOUND_TRIPLE_END      0x0303u

/* SOUND.EQU special speech tokens. */
#define WM_SPEECH_GIVE_CREDIT          (-1)
#define WM_SPEECH_VERY_IMPRESSIVE      (-2)
#define WM_SPEECH_END_GAME_STUFF       (-3)
#define WM_SPEECH_IT_DOESNT_LOOK_GOOD  (-4)
#define WM_SPEECH_R_IMPRESSIVE_MOVE    (-5)
#define WM_SPEECH_GIDDUP_MODE          (-6)
#define WM_SPEECH_REPEAT_MODE          (-7)

static const uint16_t give_credit_to[9] = {
    0x16d, 0x169, 0x16b, 0x16e, 0x168, 0x16a, 0x167, 0, 0x16c
};
static const uint16_t very_impressive_move[9] = {
    0x10f, 0x10b, 0x10c, 0x110, 0x10a, 0x10d, 0x109, 0, 0x10e
};
static const uint16_t very_impressive_move_r[9] = {
    0x1e1, 0x1dd, 0x1de, 0x1e2, 0x1db, 0x1df, 0x1dc, 0, 0x1e0
};
static const uint16_t doesnt_look_good_for[9] = {
    0x2bb, 0x2b7, 0x2b8, 0x2be, 0x2b6, 0x2b9, 0x2b4, 0, 0x2ba
};
static const uint16_t giddup_all[9] = {
    0x2cd, 0x2c9, 0x2ca, 0x2ce, 0x2c8, 0x2cb, 0x2c7, 0, 0x2cc
};
static const uint16_t ascending_table[9][4] = {
    {0x102, 0x101, 0x100, 0x0ff},
    {0x0ec, 0x0eb, 0x0ea, 0x0e9},
    {0x0f6, 0x0f5, 0x0f4, 0x0f3},
    {0x106, 0x105, 0x104, 0x103},
    {0x0e8, 0x0e7, 0x0e6, 0x0e5},
    {0x0fa, 0x0f9, 0x0f8, 0x0f7},
    {0x0e4, 0x0e3, 0x0e2, 0x0e1},
    {0, 0, 0, 0},
    {0x0fe, 0x0fd, 0x0fc, 0x0fb}
};
static const uint16_t special_last_stuff[] = {
    0x15e, 0x1d9, 0x171, 0x17a, 0x17b, 0x1e6, 0x2f9
};

static bool snd_rng_value(wm_arcade_sound *s, uint32_t inclusive_max,
                          uint32_t *value_out) {
    if (!s || !value_out) return false;
    if (!s->rng) {
        /* Source random calls are stateful.  Never substitute libc/frame RNG;
         * fail closed until the exact translated RNDRNG0 service is bound. */
        ++s->invalid_calls;
        return false;
    }
    *value_out = s->rng(s->rng_user, inclusive_max);
    return true;
}

static bool chance_passes(wm_arcade_sound *s, uint16_t chance_per_1000) {
    uint32_t sample = 0u;
    /* RNDPER always advances the source random state, even for 0/1000. */
    if (!snd_rng_value(s, 999u, &sample)) return false;
    return chance_per_1000 > sample;
}

static void channel_clear(wm_arcade_sound *s, uint8_t ch) {
    if (!s || ch >= WM_SOUND_CHANNELS) return;
    memset(&s->channel[ch], 0, sizeof(s->channel[ch]));
    s->channel[ch].script_index = WM_SOUND_NO_SCRIPT;
}

static uint8_t pick_channel(const wm_arcade_sound *s, uint8_t priority) {
    uint8_t weakest = 0;
    uint8_t weakest_pri = 0xffu;

    /* Exact source order: first zero-priority channel wins. */
    for (uint8_t ch = 0; ch < WM_SOUND_CHANNELS; ++ch) {
        if (!s->channel[ch].active || s->channel[ch].priority == 0u)
            return ch;
    }

    /* All occupied: choose the lowest priority, first one on ties. */
    for (uint8_t ch = 0; ch < WM_SOUND_CHANNELS; ++ch) {
        if (s->channel[ch].priority < weakest_pri) {
            weakest_pri = s->channel[ch].priority;
            weakest = ch;
        }
    }
    return priority >= weakest_pri ? weakest : 0xffu;
}

static bool emit_word(wm_arcade_sound *s, uint16_t dcs_command, int8_t source_channel) {
    if (!s || !s->sound_enabled) return false;
    return s->emit ? s->emit(s->emit_user, dcs_command, source_channel) : true;
}

bool wm_sound_sndsnd(wm_arcade_sound *s, uint16_t dcs_command) {
    /* Raw/control SNDSND path.  The DCS word itself owns routing. */
    return emit_word(s, dcs_command, -2);
}

static bool run_script(wm_arcade_sound *s, uint8_t ch) {
    if (!s || ch >= WM_SOUND_CHANNELS) return false;
    wm_sound_channel *c = &s->channel[ch];
    if (!s->scripts || c->script_index == WM_SOUND_NO_SCRIPT ||
        c->script_index >= s->script_count) return false;

    const wm_sound_script *sc = &s->scripts[c->script_index];
    while (c->script_pc < sc->word_count) {
        uint16_t packed = sc->words[c->script_pc++];
        uint8_t command = (uint8_t)(packed & 0xffu);
        uint8_t parameter = (uint8_t)(packed >> 8);

        switch (command) {
            case WM_SOUND_SCRIPT_SEND:
                (void)emit_word(s, parameter, (int8_t)ch);
                break;

            case WM_SOUND_SCRIPT_SETPRI:
                c->priority = parameter;
                break;

            case WM_SOUND_SCRIPT_END:
                channel_clear(s, ch);
                return false;

            case WM_SOUND_SCRIPT_SLEEP:
                c->duration = parameter;
                /* A zero sleep reaches the update boundary again next tick in
                 * the portable scheduler; keep it from underflowing. */
                if (c->duration == 0u) c->duration = 1u;
                return true;

            case WM_SOUND_SCRIPT_SEND2: {
                /* Source SEND2 stores P1 in this command's parameter byte and
                 * P2 in the following byte.  Logical channels 1/2 are P1,
                 * channels 3/4 are P2. */
                uint8_t code = parameter;
                if (c->script_pc >= sc->word_count) {
                    channel_clear(s, ch);
                    return false;
                }
                uint16_t next = sc->words[c->script_pc++];
                if (ch >= 2u) code = (uint8_t)(next & 0xffu);
                (void)emit_word(s, code, (int8_t)ch);
                break;
            }

            default:
                ++s->invalid_calls;
                channel_clear(s, ch);
                return false;
        }
    }

    channel_clear(s, ch);
    return false;
}

void wm_arcade_sound_init(wm_arcade_sound *s,
                          wm_sound_emit_fn emit, void *emit_user,
                          wm_sound_rng_fn rng, void *rng_user) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->sound_enabled = true;
    s->master_volume = 255u;
    for (uint8_t i = 0; i < WM_SOUND_CHANNELS; ++i) {
        s->lower_volume[i] = 255u;
        s->channel[i].script_index = WM_SOUND_NO_SCRIPT;
    }
    s->emit = emit;
    s->emit_user = emit_user;
    s->rng = rng;
    s->rng_user = rng_user;
}

void wm_arcade_sound_bind_tables(wm_arcade_sound *s,
                                 const wm_sound_entry *table, uint16_t table_count,
                                 const wm_sound_script *scripts, uint16_t script_count,
                                 const wm_sound_random_table *random_tables,
                                 uint16_t random_table_count,
                                 wm_sound_wrestler_matrix wrestler_matrix,
                                 const wm_sound_speech_table *speech_tables,
                                 uint16_t speech_table_count,
                                 const wm_sound_crowd_table *crowd_tables,
                                 uint16_t crowd_table_count) {
    if (!s) return;
    s->sound_table = table;
    s->sound_table_count = table_count;
    s->scripts = scripts;
    s->script_count = script_count;
    s->random_tables = random_tables;
    s->random_table_count = random_table_count;
    s->wrestler_matrix = wrestler_matrix;
    s->speech_tables = speech_tables;
    s->speech_table_count = speech_table_count;
    s->crowd_tables = crowd_tables;
    s->crowd_table_count = crowd_table_count;
}

void wm_arcade_sound_bind_hcount(wm_arcade_sound *s,
                                  wm_sound_hcount_fn hcount,
                                  void *hcount_user) {
    if (!s) return;
    s->hcount = hcount;
    s->hcount_user = hcount_user;
}

void wm_arcade_sound_bind_game_callbacks(wm_arcade_sound *s,
                                         wm_sound_health_fn health,
                                         void *health_user,
                                         uint8_t wrestler_count,
                                         wm_sound_crowd_fn crowd,
                                         void *crowd_user) {
    if (!s) return;
    s->health = health;
    s->health_user = health_user;
    s->health_wrestler_count = wrestler_count;
    s->crowd = crowd;
    s->crowd_user = crowd_user;
}

void wm_arcade_sound_reset(wm_arcade_sound *s) {
    if (!s) return;

    wm_sound_emit_fn emit = s->emit;
    void *emit_user = s->emit_user;
    wm_sound_rng_fn rng = s->rng;
    void *rng_user = s->rng_user;
    wm_sound_hcount_fn hcount = s->hcount;
    void *hcount_user = s->hcount_user;
    wm_sound_health_fn health = s->health;
    void *health_user = s->health_user;
    uint8_t wrestler_count = s->health_wrestler_count;
    wm_sound_crowd_fn crowd = s->crowd;
    void *crowd_user = s->crowd_user;

    const wm_sound_entry *table = s->sound_table;
    uint16_t table_count = s->sound_table_count;
    const wm_sound_script *scripts = s->scripts;
    uint16_t script_count = s->script_count;
    const wm_sound_random_table *random_tables = s->random_tables;
    uint16_t random_table_count = s->random_table_count;
    wm_sound_wrestler_matrix matrix = s->wrestler_matrix;
    const wm_sound_speech_table *speech = s->speech_tables;
    uint16_t speech_count = s->speech_table_count;
    const wm_sound_crowd_table *crowd_tables = s->crowd_tables;
    uint16_t crowd_table_count = s->crowd_table_count;

    wm_arcade_sound_init(s, emit, emit_user, rng, rng_user);
    wm_arcade_sound_bind_tables(s, table, table_count, scripts, script_count,
                                random_tables, random_table_count, matrix,
                                speech, speech_count,
                                crowd_tables, crowd_table_count);
    wm_arcade_sound_bind_hcount(s, hcount, hcount_user);
    wm_arcade_sound_bind_game_callbacks(s, health, health_user, wrestler_count,
                                        crowd, crowd_user);
}

void wm_arcade_sound_set_enabled(wm_arcade_sound *s, bool enabled) {
    if (!s) return;
    if (s->sound_enabled == enabled) return;
    if (!enabled) {
        wm_sound_nosounds(s);
        s->sound_enabled = false;
    } else {
        s->sound_enabled = true;
    }
}

wm_sound_result wm_sound_triple_sound(wm_arcade_sound *s, uint16_t sound_index) {
    wm_sound_result out = {0};
    if (!s || !s->sound_enabled || !s->sound_table ||
        sound_index >= s->sound_table_count) {
        if (s) ++s->invalid_calls;
        return out;
    }

    const wm_sound_entry *e = &s->sound_table[sound_index];
    if (e->priority_duration == 0u) return out;

    uint8_t priority = (uint8_t)(e->priority_duration >> 8);
    uint8_t duration = (uint8_t)(e->priority_duration & 0xffu);

    /* A nonzero table word with a zero DCS call is a source-success/no-send. */
    if (e->dcs_command == 0u) return out;

    uint8_t ch = pick_channel(s, priority);
    if (ch == 0xffu) {
        ++s->rejected_by_priority;
        return out;
    }

    wm_sound_channel *c = &s->channel[ch];
    c->active = true;
    c->priority = priority;
    c->duration = duration;
    c->current_sound = sound_index;
    c->script_index = e->script_index;
    c->script_pc = 0u;

    (void)emit_word(s, (uint16_t)(e->dcs_command + ch), (int8_t)ch);
    if (e->script_index != WM_SOUND_NO_SCRIPT)
        (void)run_script(s, ch);

    out.played = true;
    out.channel = (uint8_t)(ch + 1u);
    out.duration = c->duration;
    return out;
}

wm_sound_result wm_sound_channel_sound(wm_arcade_sound *s,
                                       uint16_t sound_index,
                                       uint8_t source_channel) {
    wm_sound_result out = {0};
    if (!s || !s->sound_enabled || !s->sound_table ||
        sound_index >= s->sound_table_count ||
        source_channel < 1u || source_channel > WM_SOUND_CHANNELS) {
        if (s) ++s->invalid_calls;
        return out;
    }

    const wm_sound_entry *e = &s->sound_table[sound_index];
    uint8_t ch = (uint8_t)(source_channel - 1u);
    wm_sound_channel *c = &s->channel[ch];
    c->active = e->priority_duration != 0u;
    c->priority = (uint8_t)(e->priority_duration >> 8);
    c->duration = (uint8_t)(e->priority_duration & 0xffu);
    c->current_sound = sound_index;
    c->script_index = e->script_index;
    c->script_pc = 0u;

    if (e->dcs_command != 0u)
        (void)emit_word(s, (uint16_t)(e->dcs_command + ch), (int8_t)ch);

    out.played = e->dcs_command != 0u;
    out.channel = source_channel;
    out.duration = c->duration;
    return out;
}

wm_sound_result wm_sound_table_sound(wm_arcade_sound *s, uint16_t sound_number) {
    wm_sound_result none = {0};
    if (!s) return none;

    if ((sound_number & WM_SOUND_RANDOM_FLAG) == 0u)
        return wm_sound_triple_sound(s, sound_number);

    uint16_t group = (uint16_t)(sound_number ^ WM_SOUND_RANDOM_FLAG);
    if (!s->random_tables || group >= s->random_table_count) {
        ++s->invalid_calls;
        return none;
    }

    const wm_sound_random_table *t = &s->random_tables[group];
    if (!t->values || t->count == 0u) return none;
    uint32_t pick = 0u;
    if (!snd_rng_value(s, (uint32_t)t->count - 1u, &pick)) return none;
    uint16_t picked = t->values[pick];
    return wm_sound_triple_sound(s, picked);
}

wm_sound_result wm_sound_wrtable_sound(wm_arcade_sound *s,
                                       uint16_t lookup_index,
                                       uint8_t wrestler) {
    wm_sound_result none = {0};
    if (!s) return none;

    lookup_index &= 0x7fffu; /* source strips W_LOOKUP / bit 15 */
    const wm_sound_wrestler_matrix *m = &s->wrestler_matrix;
    if (!m->default_values || !m->master_values ||
        lookup_index >= m->value_count || wrestler >= m->wrestler_count) {
        ++s->invalid_calls;
        return none;
    }

    uint16_t value = m->master_values[(size_t)wrestler * m->value_count + lookup_index];
    if ((int16_t)value < 0) value = m->default_values[lookup_index];
    if (value == 0u) return none;

    /* The surviving DCSSOUND.ASM listing jumps directly to triple_sound here,
     * even though some table cells carry the 0x1000 random-table flag.  Keep
     * the exact listing behavior in this entry point; callers that need the
     * random-table dispatcher call wm_sound_table_sound explicitly. */
    return wm_sound_triple_sound(s, value);
}

wm_sound_announcer wm_sound_who_is_it(uint16_t sound_index) {
    if (sound_index < WM_SOUND_ANNOUNCER_START) return WM_SOUND_ANNOUNCER_NONE;
    if (sound_index < WM_SOUND_VINCE_END) return WM_SOUND_ANNOUNCER_VINCE;
    if (sound_index < WM_SOUND_RANDY_END) return WM_SOUND_ANNOUNCER_RANDY;
    if (sound_index < WM_SOUND_HOWARD_END) return WM_SOUND_ANNOUNCER_HOWARD;
    if (sound_index < WM_SOUND_MORE_JERRY) return WM_SOUND_ANNOUNCER_NONE;
    if (sound_index < WM_SOUND_TRIPLE_END) return WM_SOUND_ANNOUNCER_RANDY;
    return WM_SOUND_ANNOUNCER_NONE;
}

static void announcer_slots(wm_arcade_sound *s, wm_sound_announcer who,
                            uint8_t **channel, uint16_t **duration) {
    *channel = NULL;
    *duration = NULL;
    switch (who) {
        case WM_SOUND_ANNOUNCER_VINCE:
            *channel = &s->vince_channel;
            *duration = &s->vince_duration;
            break;
        case WM_SOUND_ANNOUNCER_RANDY:
            *channel = &s->randy_channel;
            *duration = &s->randy_duration;
            break;
        case WM_SOUND_ANNOUNCER_HOWARD:
            *channel = &s->howard_channel;
            *duration = &s->howard_duration;
            break;
        default:
            break;
    }
}

wm_sound_result wm_sound_announcer_sound(wm_arcade_sound *s,
                                         uint16_t sound_index) {
    wm_sound_result none = {0};
    if (!s || !s->sound_table || sound_index >= s->sound_table_count) return none;

    wm_sound_announcer who = wm_sound_who_is_it(sound_index);
    if (who == WM_SOUND_ANNOUNCER_NONE) return none;

    uint8_t *speaker_channel = NULL;
    uint16_t *speaker_duration = NULL;
    announcer_slots(s, who, &speaker_channel, &speaker_duration);
    if (!speaker_channel || !speaker_duration) return none;

    const wm_sound_entry *e = &s->sound_table[sound_index];
    uint8_t new_duration = (uint8_t)(e->priority_duration & 0xffu);
    uint8_t new_priority = (uint8_t)(e->priority_duration >> 8);

    if (*speaker_duration != 0u && *speaker_channel >= 1u &&
        *speaker_channel <= WM_SOUND_CHANNELS) {
        /* Existing speaker is cut off and restarted on the same source track. */
        uint8_t ch = (uint8_t)(*speaker_channel - 1u);
        wm_sound_channel *c = &s->channel[ch];
        c->active = true;
        c->priority = new_priority;
        c->duration = new_duration;
        c->current_sound = sound_index;
        c->script_index = e->script_index;
        c->script_pc = 0u;
        *speaker_duration = new_duration;
        if (e->dcs_command != 0u)
            (void)emit_word(s, (uint16_t)(e->dcs_command + ch), (int8_t)ch);
        return (wm_sound_result){e->dcs_command != 0u, *speaker_channel, new_duration};
    }

    wm_sound_result r = wm_sound_triple_sound(s, sound_index);
    *speaker_channel = r.channel;
    *speaker_duration = r.duration;
    return r;
}

void wm_sound_nosounds(wm_arcade_sound *s) {
    if (!s) return;
    if (s->sound_enabled) (void)wm_sound_sndsnd(s, 0u);
    for (uint8_t ch = 0; ch < WM_SOUND_CHANNELS; ++ch) channel_clear(s, ch);
    s->vince_channel = s->randy_channel = s->howard_channel = 0u;
    s->vince_duration = s->randy_duration = s->howard_duration = 0u;
    s->endless_channel = 0u;
    s->voice_wait_ticks = 0u;
}

void wm_sound_kill_all_channels(wm_arcade_sound *s) {
    if (!s) return;
    static const uint16_t stop_codes[WM_SOUND_CHANNELS] = {
        WM_SOUND_STOP_CH1, WM_SOUND_STOP_CH2, WM_SOUND_STOP_CH3, WM_SOUND_STOP_CH4
    };
    for (uint8_t ch = 0; ch < WM_SOUND_CHANNELS; ++ch) {
        channel_clear(s, ch);
        (void)wm_sound_sndsnd(s, stop_codes[ch]);
    }
    s->endless_channel = 0u;
}

void wm_sound_find_and_kill_endless(wm_arcade_sound *s) {
    if (!s || s->endless_channel < 1u || s->endless_channel > WM_SOUND_CHANNELS)
        return;
    uint8_t ch = (uint8_t)(s->endless_channel - 1u);
    channel_clear(s, ch);
    (void)wm_sound_sndsnd(s, (uint16_t)(WM_SOUND_STOP_CH1 + ch));
    s->endless_channel = 0u;
}

void wm_sound_reset_voice_queue(wm_arcade_sound *s) {
    if (!s) return;
    s->announce_read = s->announce_write = s->announce_count = 0u;
    s->voice_wait_ticks = 0u;
}

bool wm_sound_add_voice(wm_arcade_sound *s, uint16_t sound_index) {
    if (!s || sound_index >= s->sound_table_count) {
        if (s) ++s->invalid_calls;
        return false;
    }
    if (s->announce_count >= WM_SOUND_ANNOUNCE_QUEUE_CAPACITY) {
        ++s->dropped_voice_items;
        return false;
    }
    s->announce_queue[s->announce_write] = sound_index;
    s->announce_write = (uint8_t)((s->announce_write + 1u) %
                                  WM_SOUND_ANNOUNCE_QUEUE_CAPACITY);
    ++s->announce_count;
    return true;
}

bool wm_sound_if_silent_add_voice(wm_arcade_sound *s, uint16_t sound_index) {
    if (!s || s->announce_count != 0u) return false;
    wm_sound_announcer who = wm_sound_who_is_it(sound_index);
    if (who != WM_SOUND_ANNOUNCER_NONE) {
        uint8_t *channel = NULL;
        uint16_t *duration = NULL;
        announcer_slots(s, who, &channel, &duration);
        (void)channel;
        if (duration && *duration != 0u) return false;
    }
    return wm_sound_add_voice(s, sound_index);
}

void wm_sound_service_voice_queue(wm_arcade_sound *s) {
    if (!s || s->voice_wait_ticks != 0u) return;

    /* PRCSLP returns immediately for a zero-duration/no-op call, so consume
     * further queue entries on the same source tick until one actually waits. */
    unsigned guard = WM_SOUND_ANNOUNCE_QUEUE_CAPACITY;
    while (s->announce_count != 0u && guard-- != 0u) {
        uint16_t sound_index = s->announce_queue[s->announce_read];
        s->announce_read = (uint8_t)((s->announce_read + 1u) %
                                     WM_SOUND_ANNOUNCE_QUEUE_CAPACITY);
        --s->announce_count;

        wm_sound_find_and_kill_endless(s);
        wm_sound_result r;
        if (sound_index >= WM_SOUND_ANNOUNCER_START)
            r = wm_sound_announcer_sound(s, sound_index);
        else
            r = wm_sound_triple_sound(s, sound_index);

        if (r.duration != 0u) {
            s->voice_wait_ticks = r.duration;
            return;
        }
    }
}

void wm_sound_clear_speech_repeat(wm_arcade_sound *s) {
    if (!s) return;
    memset(s->last_voice, 0, sizeof(s->last_voice));
    s->which_last_voice = 0u;
    s->repeat_state = 0u;
    s->repeat_timeout = 0u;
}

static bool speech_seen(const wm_arcade_sound *s, uint16_t sound_index) {
    for (uint8_t i = 0; i < WM_SOUND_LAST_VOICE_SLOTS; ++i)
        if (s->last_voice[i] == sound_index) return true;
    return false;
}

static void speech_remember(wm_arcade_sound *s, uint16_t sound_index) {
    s->which_last_voice = (uint8_t)((s->which_last_voice + 1u) %
                                    WM_SOUND_LAST_VOICE_SLOTS);
    s->last_voice[s->which_last_voice] = sound_index;
}

static uint16_t personal_call(int16_t token, uint8_t wrestler,
                              wm_arcade_sound *s) {
    if (wrestler >= 9u) return 0u;
    switch (token) {
        case WM_SPEECH_GIVE_CREDIT:         return give_credit_to[wrestler];
        case WM_SPEECH_VERY_IMPRESSIVE:     return very_impressive_move[wrestler];
        case WM_SPEECH_IT_DOESNT_LOOK_GOOD: return doesnt_look_good_for[wrestler];
        case WM_SPEECH_R_IMPRESSIVE_MOVE:   return very_impressive_move_r[wrestler];
        case WM_SPEECH_GIDDUP_MODE:         return giddup_all[wrestler];
        case WM_SPEECH_REPEAT_MODE: {
            if (s->repeat_state == 0u) {
                s->repeat_state = 4u;
                s->repeat_timeout = 80u;
            } else {
                s->repeat_timeout = 80u;
            }
            --s->repeat_state;
            return ascending_table[wrestler][s->repeat_state & 3u];
        }
        default:
            return 0u;
    }
}

static uint16_t end_game_call(wm_arcade_sound *s) {
    if (!s->health || s->health_wrestler_count == 0u) return 0u;
    bool near_end = false;
    for (uint8_t i = 0; i < s->health_wrestler_count; ++i) {
        if (s->health(s->health_user, i) < 40) {
            near_end = true;
            break;
        }
    }
    if (!near_end) return 0u;
    uint32_t pick = 0u;
    if (!snd_rng_value(s,
            (uint32_t)(sizeof(special_last_stuff) / sizeof(special_last_stuff[0]) - 1u),
            &pick)) return 0u;
    return special_last_stuff[pick];
}

static void do_crowd_anyway(wm_arcade_sound *s, int16_t crowd_table_index) {
    if (!s || crowd_table_index < 0 || s->crowd_busy_ticks != 0u ||
        !s->crowd_tables || (uint16_t)crowd_table_index >= s->crowd_table_count)
        return;

    const wm_sound_crowd_table *table = &s->crowd_tables[crowd_table_index];
    if (!table->entries || table->count == 0u) return;
    uint32_t pick = 0u;
    if (!snd_rng_value(s, (uint32_t)table->count - 1u, &pick)) return;
    const wm_sound_crowd_entry *e = &table->entries[pick];

    (void)wm_sound_sndsnd(s, e->dcs_command);
    s->crowd_busy_ticks = e->duration;
    if (s->crowd) s->crowd(s->crowd_user, e->flags, e->random_parameter);
}

static bool add_speech_table(wm_arcade_sound *s,
                             uint16_t speech_table_index,
                             uint16_t chance_per_1000,
                             uint8_t attacking_wrestler,
                             bool unconditional) {
    if (!s || !s->speech_tables || speech_table_index >= s->speech_table_count)
        return false;

    const wm_sound_speech_table *table = &s->speech_tables[speech_table_index];
    if (!table->entries || table->entry_count == 0u || table->entry_words == 0u ||
        table->entry_words > WM_SOUND_MAX_SPEECH_ENTRY_WORDS) return false;

    if (table->reset_repeat != 0) s->repeat_state = 0u;
    do_crowd_anyway(s, table->crowd_table_index);

    if (s->repeat_state == 0u && !chance_passes(s, chance_per_1000)) return false;

    uint32_t entry_pick = 0u;
    if (!snd_rng_value(s, (uint32_t)table->entry_count - 1u, &entry_pick))
        return false;
    uint16_t entry = (uint16_t)entry_pick;
    const int16_t *words = table->entries + (size_t)entry * table->entry_words;
    int16_t first = words[0];
    uint16_t sound = 0u;

    if (first == WM_SPEECH_END_GAME_STUFF) {
        sound = end_game_call(s);
        if (sound == 0u) return false;
    } else if (first < 0) {
        sound = personal_call(first, attacking_wrestler, s);
        if (sound == 0u) return false;
    } else {
        sound = (uint16_t)first;
    }

    if (speech_seen(s, sound)) return false;
    speech_remember(s, sound);

    bool added = unconditional ? wm_sound_add_voice(s, sound)
                               : wm_sound_if_silent_add_voice(s, sound);
    if (!added) return false;

    for (uint8_t i = 1u; i < table->entry_words; ++i) {
        int16_t next = words[i];
        if (next == 0) break;
        uint16_t next_sound = next < 0 ? personal_call(next, attacking_wrestler, s)
                                       : (uint16_t)next;
        if (next_sound != 0u) (void)wm_sound_add_voice(s, next_sound);
    }
    return true;
}

bool wm_sound_add_to_queue(wm_arcade_sound *s,
                           uint16_t speech_table_index,
                           uint16_t chance_per_1000,
                           uint8_t attacking_wrestler) {
    return add_speech_table(s, speech_table_index, chance_per_1000,
                            attacking_wrestler, true);
}

bool wm_sound_add_table_if_silent(wm_arcade_sound *s,
                                  uint16_t speech_table_index,
                                  uint16_t chance_per_1000,
                                  uint8_t attacking_wrestler) {
    return add_speech_table(s, speech_table_index, chance_per_1000,
                            attacking_wrestler, false);
}

static bool schedule_pending(wm_arcade_sound *s, wm_sound_pending_kind kind,
                             uint16_t delay_ticks, uint16_t value,
                             uint16_t chance, uint8_t wrestler) {
    if (!s) return false;
    if (delay_ticks == 0u) {
        switch (kind) {
            case WM_SOUND_PENDING_SPEECH_IF_SILENT:
                return wm_sound_add_table_if_silent(s, value, chance, wrestler);
            case WM_SOUND_PENDING_SPEECH_ADD:
                return wm_sound_add_to_queue(s, value, chance, wrestler);
            case WM_SOUND_PENDING_VOICE_IF_SILENT:
                return wm_sound_if_silent_add_voice(s, value);
            case WM_SOUND_PENDING_VOICE_ADD:
                return wm_sound_add_voice(s, value);
            default:
                return false;
        }
    }
    for (uint8_t i = 0; i < WM_SOUND_PENDING_CALL_CAPACITY; ++i) {
        wm_sound_pending_call *p = &s->pending_calls[i];
        if (p->active) continue;
        p->active = true;
        p->kind = kind;
        p->ticks_left = delay_ticks;
        p->value = value;
        p->chance = chance;
        p->wrestler = wrestler;
        return true;
    }
    ++s->dropped_voice_items;
    return false;
}

bool wm_sound_schedule_speech_if_silent(wm_arcade_sound *s, uint16_t delay_ticks,
                                         wm_sound_speech_table_id table,
                                         uint16_t chance_per_1000,
                                         uint8_t wrestler) {
    if ((unsigned)table >= WM_SOUND_SPEECH_COUNT) {
        if (s) ++s->invalid_calls;
        return false;
    }
    return schedule_pending(s, WM_SOUND_PENDING_SPEECH_IF_SILENT,
                            delay_ticks, (uint16_t)table,
                            chance_per_1000, wrestler);
}

/* Direct translations of the CREATE/SLEEP wrappers in DCSSOUND.ASM. */
bool wm_sound_call_misses(wm_arcade_sound *s) {
    return wm_sound_schedule_speech_if_silent(s, 5u, WM_SOUND_SPEECH_MISSES, 350u, 0u);
}

bool wm_sound_call_special_move(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 10u, WM_SOUND_SPEECH_SPECIAL_MOVE, 550u, wrestler);
}

bool wm_sound_call_drop_kick(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 15u, WM_SOUND_SPEECH_DROP_KICK, 400u, wrestler);
}

bool wm_sound_call_face_hit(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 8u, WM_SOUND_SPEECH_FACE_HIT, 200u, wrestler);
}

bool wm_sound_call_mid_hit(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 8u, WM_SOUND_SPEECH_MID_HIT, 200u, wrestler);
}

bool wm_sound_call_average_move(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 10u, WM_SOUND_SPEECH_AVERAGE_MOVE, 500u, wrestler);
}

bool wm_sound_do_reversal(wm_arcade_sound *s, uint8_t wrestler) {
    if (!s || wrestler >= 9u || wrestler == 7u) {
        if (s) ++s->invalid_calls;
        return false;
    }
    wm_sound_find_and_kill_endless(s);
    return wm_sound_schedule_speech_if_silent(s, 0u, WM_SOUND_SPEECH_REVERSAL, 500u, wrestler);
}

bool wm_sound_call_miss_yoko(wm_arcade_sound *s) {
    return wm_sound_schedule_speech_if_silent(s, 10u, WM_SOUND_SPEECH_MISS_YOKO, 400u, 0u);
}

bool wm_sound_call_thrown_out(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 10u, WM_SOUND_SPEECH_THROWN_OUT, 500u, wrestler);
}

bool wm_sound_call_other_average(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 10u, WM_SOUND_SPEECH_OTHER_AVERAGE, 500u, wrestler);
}

bool wm_sound_call_nasty_move(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 10u, WM_SOUND_SPEECH_NASTY_MOVE, 500u, wrestler);
}

bool wm_sound_call_setup(wm_arcade_sound *s, uint8_t wrestler) {
    return wm_sound_schedule_speech_if_silent(s, 5u, WM_SOUND_SPEECH_SETUP_MOVE, 500u, wrestler);
}

static void service_pending_calls(wm_arcade_sound *s) {
    for (uint8_t i = 0; i < WM_SOUND_PENDING_CALL_CAPACITY; ++i) {
        wm_sound_pending_call *p = &s->pending_calls[i];
        if (!p->active) continue;
        if (p->ticks_left != 0u) {
            --p->ticks_left;
            if (p->ticks_left != 0u) continue;
        }
        switch (p->kind) {
            case WM_SOUND_PENDING_SPEECH_IF_SILENT:
                (void)wm_sound_add_table_if_silent(s, p->value, p->chance, p->wrestler);
                break;
            case WM_SOUND_PENDING_SPEECH_ADD:
                (void)wm_sound_add_to_queue(s, p->value, p->chance, p->wrestler);
                break;
            case WM_SOUND_PENDING_VOICE_IF_SILENT:
                (void)wm_sound_if_silent_add_voice(s, p->value);
                break;
            case WM_SOUND_PENDING_VOICE_ADD:
                (void)wm_sound_add_voice(s, p->value);
                break;
            default:
                ++s->invalid_calls;
                break;
        }
        memset(p, 0, sizeof(*p));
    }
}

static uint16_t volume_word(uint8_t value) {
    return (uint16_t)(((uint16_t)value << 8) | ((uint16_t)(~value) & 0xffu));
}

void wm_sound_set_lower_vol(wm_arcade_sound *s, int8_t source_channel,
                            uint8_t value) {
    if (!s || source_channel < -1 || source_channel >= (int8_t)WM_SOUND_CHANNELS)
        return;
    uint16_t selector = (uint16_t)(WM_SOUND_VOL_CH0 + source_channel);
    (void)wm_sound_sndsnd(s, selector);
    (void)wm_sound_sndsnd(s, volume_word(value));
    if (source_channel < 0) s->master_volume = value;
    else s->lower_volume[(uint8_t)source_channel] = value;
}

void wm_sound_set_volume(wm_arcade_sound *s, uint8_t value) {
    wm_sound_set_lower_vol(s, -1, value);
}

void wm_sound_begin_fade(wm_arcade_sound *s, int8_t source_channel,
                         uint16_t ticks) {
    if (!s || ticks == 0u || source_channel < -1 ||
        source_channel >= (int8_t)WM_SOUND_CHANNELS) return;
    uint8_t start = source_channel < 0 ? s->master_volume
                                      : s->lower_volume[(uint8_t)source_channel];
    s->fade_active = true;
    s->fade_channel = source_channel;
    s->fade_ticks_left = ticks;
    s->fade_accum_16_16 = (uint32_t)start << 16;
    s->fade_step_16_16 = s->fade_accum_16_16 / ticks;
}

static void tick_fade(wm_arcade_sound *s) {
    if (!s->fade_active) return;
    if (s->fade_ticks_left == 0u) {
        wm_sound_set_lower_vol(s, s->fade_channel, 0u);
        s->fade_active = false;
        return;
    }

    if (s->fade_accum_16_16 > s->fade_step_16_16)
        s->fade_accum_16_16 -= s->fade_step_16_16;
    else
        s->fade_accum_16_16 = 0u;

    wm_sound_set_lower_vol(s, s->fade_channel,
                           (uint8_t)(s->fade_accum_16_16 >> 16));
    --s->fade_ticks_left;
    if (s->fade_ticks_left == 0u) {
        wm_sound_set_lower_vol(s, s->fade_channel, 0u);
        s->fade_active = false;
    }
}

void wm_arcade_sound_tick(wm_arcade_sound *s) {
    if (!s) return;
    ++s->source_tick;

    if (s->vince_duration) --s->vince_duration;
    if (s->randy_duration) --s->randy_duration;
    if (s->howard_duration) --s->howard_duration;
    if (s->voice_wait_ticks) --s->voice_wait_ticks;
    if (s->crowd_busy_ticks) --s->crowd_busy_ticks;

    if (s->repeat_timeout) {
        --s->repeat_timeout;
        if (s->repeat_timeout == 0u) s->repeat_state = 0u;
    }

    for (uint8_t ch = 0; ch < WM_SOUND_CHANNELS; ++ch) {
        wm_sound_channel *c = &s->channel[ch];
        if (!c->active || c->duration == 0u) continue;
        --c->duration;
        if (c->duration == 0u) {
            if (c->script_index != WM_SOUND_NO_SCRIPT)
                (void)run_script(s, ch);
            else
                channel_clear(s, ch);
        }
    }

    tick_fade(s);
    service_pending_calls(s);
    wm_sound_service_voice_queue(s);
}

static wm_sound_result random_sound(wm_arcade_sound *s,
                                    const uint16_t *ids, size_t count) {
    wm_sound_result none = {0};
    if (!s || !ids || count == 0u) return none;
    uint32_t pick = 0u;
    if (!snd_rng_value(s, (uint32_t)count - 1u, &pick)) return none;
    return wm_sound_triple_sound(s, ids[pick]);
}

wm_sound_result wm_sound_ring_bell_start(wm_arcade_sound *s) {
    return wm_sound_triple_sound(s, 0x0b1u);
}

wm_sound_result wm_sound_walk(wm_arcade_sound *s) {
    wm_sound_result none = {0};
    if (!s || !s->hcount) {
        if (s) ++s->invalid_calls;
        return none;
    }
    /* WALK_SOUND reads @HCOUNT, shifts right once, and branches on carry;
     * therefore the selected footstep is exactly HCOUNT bit 0. */
    return wm_sound_triple_sound(s,
        (s->hcount(s->hcount_user) & 1u) ? 0x47u : 0x46u);
}

wm_sound_result wm_sound_small_run(wm_arcade_sound *s) {
    static const uint16_t ids[] = {0x0c0, 0x0c2, 0x0c0};
    return random_sound(s, ids, sizeof(ids) / sizeof(ids[0]));
}

wm_sound_result wm_sound_small_bounce(wm_arcade_sound *s) {
    static const uint16_t ids[] = {0x0c0, 0x0c2, 0x00d};
    return random_sound(s, ids, sizeof(ids) / sizeof(ids[0]));
}

wm_sound_result wm_sound_hit_the_mat(wm_arcade_sound *s) {
    /* Source first calls C1h, then a random 76/77/78; return the latter. */
    (void)wm_sound_triple_sound(s, 0x0c1u);
    static const uint16_t ids[] = {0x076, 0x077, 0x078};
    return random_sound(s, ids, sizeof(ids) / sizeof(ids[0]));
}

wm_sound_result wm_sound_block(wm_arcade_sound *s) {
    static const uint16_t ids[] = {0x004, 0x007, 0x008};
    return random_sound(s, ids, sizeof(ids) / sizeof(ids[0]));
}

wm_sound_result wm_sound_flame(wm_arcade_sound *s) {
    static const uint16_t ids[] = {0x099, 0x09a};
    return random_sound(s, ids, sizeof(ids) / sizeof(ids[0]));
}

wm_sound_result wm_sound_flame_hit(wm_arcade_sound *s) {
    static const uint16_t ids[] = {0x09d, 0x09e, 0x09f, 0x0a0, 0x0a1};
    return random_sound(s, ids, sizeof(ids) / sizeof(ids[0]));
}

wm_sound_result wm_sound_blocked(wm_arcade_sound *s, uint8_t wrestler) {
    static const uint16_t ids[9] = {
        0x236, 0x280, 0x23d, 0x23e, 0x284, 0x06a, 0x212, 0, 0x287
    };
    wm_sound_result none = {0};
    if (!s || wrestler >= 9u || ids[wrestler] == 0u) return none;
    /* DO_BLOCKED is gated by RNDPER 50 in the source. */
    if (!chance_passes(s, 50u)) return none;
    return wm_sound_triple_sound(s, ids[wrestler]);
}

wm_sound_result wm_sound_scream(wm_arcade_sound *s, uint8_t wrestler) {
    static const uint16_t ids[9][4] = {
        {0x265,0x266,0x262,0x263}, {0x268,0x269,0x26f,0x26c},
        {0x265,0x266,0x262,0x263}, {0x268,0x269,0x26f,0x26c},
        {0x265,0x266,0x262,0x263}, {0x265,0x266,0x262,0x263},
        {0x071,0x072,0x20a,0x20c}, {0,0,0,0},
        {0x268,0x269,0x26f,0x26c}
    };
    wm_sound_result none = {0};
    if (!s || wrestler >= 9u) return none;
    uint32_t pick = 0u;
    if (!snd_rng_value(s, 3u, &pick)) return none;
    uint16_t id = ids[wrestler][pick];
    return id ? wm_sound_triple_sound(s, id) : none;
}

wm_sound_result wm_sound_wail(wm_arcade_sound *s, uint8_t wrestler) {
    static const uint16_t ids[9] = {
        0x25f,0x270,0x20d,0x20d,0x20d,0x20d,0x20d,0,0x20d
    };
    wm_sound_result none = {0};
    if (!s || wrestler >= 9u || ids[wrestler] == 0u) return none;
    return wm_sound_triple_sound(s, ids[wrestler]);
}

wm_sound_result wm_sound_choke(wm_arcade_sound *s, uint8_t wrestler) {
    wm_sound_result none = {0};
    if (!s || wrestler >= 9u || wrestler == 7u) return none;
    wm_sound_find_and_kill_endless(s);
    wm_sound_result r = wm_sound_triple_sound(s, 0x21au);
    if (r.played) s->endless_channel = r.channel;
    return r;
}

wm_sound_result wm_sound_nono(wm_arcade_sound *s, uint8_t wrestler) {
    static const uint16_t ids[9] = {
        0x23c,0x281,0x23c,0x23c,0x23c,0x23c,0x219,0,0x23c
    };
    wm_sound_result none = {0};
    if (!s || wrestler >= 9u || ids[wrestler] == 0u) return none;
    wm_sound_find_and_kill_endless(s);
    wm_sound_result r = wm_sound_triple_sound(s, ids[wrestler]);
    if (r.played) s->endless_channel = r.channel;
    return r;
}

wm_sound_result wm_sound_razor_rug_speech(wm_arcade_sound *s,
                                          uint16_t repeat_count) {
    static const uint16_t ids[] = {0x27d,0x27c,0x27b,0x27a};
    wm_sound_result none = {0};
    if (!s || repeat_count == 0u) return none;
    uint16_t n = (uint16_t)(repeat_count - 1u);
    if (n >= 4u) return none;
    return wm_sound_triple_sound(s, ids[n]);
}

wm_sound_result wm_sound_doink_slam(wm_arcade_sound *s,
                                    uint16_t repeat_count) {
    wm_sound_result none = {0};
    if (!s) return none;
    if (repeat_count == 0u || repeat_count == 1u)
        return wm_sound_triple_sound(s, 0x218u);
    uint16_t n = (uint16_t)(repeat_count - 2u);
    static const uint16_t ids[] = {0x215,0x216,0x217};
    if (n >= 3u) return none;
    return wm_sound_triple_sound(s, ids[n]);
}

wm_sound_result wm_sound_bone_break(wm_arcade_sound *s) {
    static const uint16_t ids[] = {0x01d,0x09b,0x098};
    return random_sound(s, ids, sizeof(ids) / sizeof(ids[0]));
}
