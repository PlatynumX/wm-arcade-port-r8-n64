#ifndef WM_ARCADE_SOUND_H
#define WM_ARCADE_SOUND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Portable game-side translation of WWF WrestleMania DCSSOUND.ASM.
 *
 * This module intentionally does not emulate the DCS DSP.  The arcade main
 * CPU sent 16-bit host commands to the DCS board; the N64 port preserves that
 * command boundary and sends the same 16-bit values to its platform backend.
 */

#define WM_SOUND_CHANNELS 4u
#define WM_SOUND_ANNOUNCE_QUEUE_CAPACITY 20u
#define WM_SOUND_LAST_VOICE_SLOTS 4u /* LAST_VOICE is 64 bits = four words. */
#define WM_SOUND_DEFLT 0x8000u
#define WM_SOUND_RANDOM_FLAG 0x1000u
#define WM_SOUND_NO_SCRIPT 0xffffu
#define WM_SOUND_MAX_SPEECH_ENTRY_WORDS 4u
#define WM_SOUND_PENDING_CALL_CAPACITY 16u

/* DCSSOUND.ASM tune script command byte (low byte of [parameter,command]). */
enum {
    WM_SOUND_SCRIPT_SEND   = 0x01,
    WM_SOUND_SCRIPT_SETPRI = 0x02,
    WM_SOUND_SCRIPT_END    = 0x03,
    WM_SOUND_SCRIPT_SLEEP  = 0x04,
    WM_SOUND_SCRIPT_SEND2  = 0x05
};

/* Source priority values after the source's `srl 8` extraction. */
enum {
    WM_SOUND_PRI_ROBO    = 1,
    WM_SOUND_PRI_MAT1    = 4,
    WM_SOUND_PRI_WOOSH   = 8,
    WM_SOUND_PRI_ATTKV   = 12,
    WM_SOUND_PRI_REACV   = 14,
    WM_SOUND_PRI_LOSMACK = 15,
    WM_SOUND_PRI_SMACK   = 16,
    WM_SOUND_PRI_MAT2    = 20,
    WM_SOUND_PRI_WSPCH   = 24,
    WM_SOUND_PRI_SYSTEM1 = 36,
    WM_SOUND_PRI_SYSTEM2 = 40,
    WM_SOUND_PRI_SYSTEM3 = 44,
    WM_SOUND_PRI_ANNCER  = 100
};

typedef struct {
    uint16_t priority_duration; /* source word: high byte priority, low byte ticks */
    uint16_t dcs_command;       /* source channel-1 DCS call; channels add 0..3 */
    uint16_t script_index;      /* WM_SOUND_NO_SCRIPT for ordinary table calls */
    uint16_t flags;
} wm_sound_entry;

typedef struct {
    /* Exact source encoding: each command word is [parameter, command].
     * SEND2 consumes one extra word; its low byte contains the P2 code. */
    const uint16_t *words;
    uint16_t word_count;
} wm_sound_script;

typedef struct {
    uint8_t priority;
    uint16_t duration;
    uint16_t current_sound; /* triple-table index */
    uint16_t script_index;
    uint16_t script_pc;
    bool active;
} wm_sound_channel;

typedef struct {
    const uint16_t *values; /* selected triple-table ids (header/max removed) */
    uint16_t count;
} wm_sound_random_table;

typedef struct {
    const uint16_t *default_values;
    const uint16_t *master_values; /* wrestler-major flattened matrix */
    uint16_t value_count;
    uint8_t wrestler_count;
} wm_sound_wrestler_matrix;

typedef enum {
    WM_SOUND_ANNOUNCER_NONE = 0,
    WM_SOUND_ANNOUNCER_VINCE,
    WM_SOUND_ANNOUNCER_RANDY,
    WM_SOUND_ANNOUNCER_HOWARD
} wm_sound_announcer;

/* High-level ADD_TO_QUEUE speech table.  entry_count is the source header's
 * inclusive maximum + 1; entry_words is 1 for 0010h and 2 for 0020h tables. */
typedef struct {
    int16_t reset_repeat;
    int16_t crowd_table_index; /* -1 when source crowd pointer is zero */
    uint16_t entry_count;
    uint8_t entry_words;
    const int16_t *entries;
} wm_sound_speech_table;

typedef struct {
    uint16_t dcs_command;
    uint16_t duration;
    uint16_t flags;
    uint16_t random_parameter;
} wm_sound_crowd_entry;

/* Generated speech-table order is source order and intentionally stable. */
typedef enum {
    WM_SOUND_SPEECH_CLIMB_ROPES = 0,
    WM_SOUND_SPEECH_JUMP_ROPES,
    WM_SOUND_SPEECH_MISSES,
    WM_SOUND_SPEECH_SPECIAL_MOVE,
    WM_SOUND_SPEECH_DROP_KICK,
    WM_SOUND_SPEECH_FACE_HIT,
    WM_SOUND_SPEECH_MID_HIT,
    WM_SOUND_SPEECH_AVERAGE_MOVE,
    WM_SOUND_SPEECH_REVERSAL,
    WM_SOUND_SPEECH_MISS_YOKO,
    WM_SOUND_SPEECH_THROWN_OUT,
    WM_SOUND_SPEECH_OTHER_AVERAGE,
    WM_SOUND_SPEECH_NASTY_MOVE,
    WM_SOUND_SPEECH_SETUP_MOVE,
    WM_SOUND_SPEECH_SPECIAL_LAST_STUFF,
    WM_SOUND_SPEECH_MATCH_OVER,
    WM_SOUND_SPEECH_MATCH_OVER_DL,
    WM_SOUND_SPEECH_COUNT
} wm_sound_speech_table_id;

typedef enum {
    WM_SOUND_PENDING_NONE = 0,
    WM_SOUND_PENDING_SPEECH_IF_SILENT,
    WM_SOUND_PENDING_SPEECH_ADD,
    WM_SOUND_PENDING_VOICE_IF_SILENT,
    WM_SOUND_PENDING_VOICE_ADD,
    WM_SOUND_PENDING_RNDRNG0_VOICE_IF_SILENT,
    WM_SOUND_PENDING_GIDDUP
} wm_sound_pending_kind;

typedef struct {
    wm_sound_pending_kind kind;
    uint16_t ticks_left;
    uint16_t value;       /* speech-table index or triple-table id */
    uint16_t chance;      /* source RNDPER probability, 0..1000 */
    uint8_t wrestler;
    bool active;
} wm_sound_pending_call;

typedef struct {
    const wm_sound_crowd_entry *entries;
    uint16_t count;
} wm_sound_crowd_table;

/* source_channel is 0..3 when the translated scheduler selected a DCS voice,
 * or -2 for raw/control SNDSND words whose routing is owned by the command. */
typedef bool (*wm_sound_emit_fn)(void *user, uint16_t command, int8_t source_channel);
typedef uint32_t (*wm_sound_rng_fn)(void *user, uint32_t inclusive_max);
typedef uint32_t (*wm_sound_hcount_fn)(void *user);
typedef int (*wm_sound_health_fn)(void *user, uint8_t wrestler);
typedef void (*wm_sound_crowd_fn)(void *user,
                                  uint16_t flags,
                                  uint16_t random_parameter);

typedef struct {
    wm_sound_channel channel[WM_SOUND_CHANNELS];

    uint8_t vince_channel;  /* 1..4, zero when never assigned */
    uint16_t vince_duration;
    uint8_t randy_channel;
    uint16_t randy_duration;
    uint8_t howard_channel;
    uint16_t howard_duration;

    /* Source SOUNDSUP is zero when sound is allowed. */
    bool sound_enabled;
    bool doing_dcs_reset;
    uint8_t endless_channel; /* 1..4, 0 = none */

    uint16_t last_voice[WM_SOUND_LAST_VOICE_SLOTS];
    uint8_t which_last_voice;
    uint16_t repeat_state;
    uint16_t repeat_timeout;

    uint16_t announce_queue[WM_SOUND_ANNOUNCE_QUEUE_CAPACITY];
    uint8_t announce_read;
    uint8_t announce_write;
    uint8_t announce_count;
    uint16_t voice_wait_ticks;
    uint16_t crowd_busy_ticks;

    wm_sound_pending_call pending_calls[WM_SOUND_PENDING_CALL_CAPACITY];

    uint8_t master_volume;
    uint8_t lower_volume[WM_SOUND_CHANNELS];

    bool fade_active;
    int8_t fade_channel; /* -1 master, 0..3 logical channel */
    uint16_t fade_ticks_left;
    uint32_t fade_accum_16_16;
    uint32_t fade_step_16_16;

    uint32_t source_tick;
    uint32_t dropped_voice_items;
    uint32_t rejected_by_priority;
    uint32_t invalid_calls;

    const wm_sound_entry *sound_table;
    uint16_t sound_table_count;
    const wm_sound_script *scripts;
    uint16_t script_count;
    const wm_sound_random_table *random_tables;
    uint16_t random_table_count;
    wm_sound_wrestler_matrix wrestler_matrix;
    const wm_sound_speech_table *speech_tables;
    uint16_t speech_table_count;
    const wm_sound_crowd_table *crowd_tables;
    uint16_t crowd_table_count;

    wm_sound_emit_fn emit;
    void *emit_user;
    wm_sound_rng_fn rng;
    void *rng_user;
    wm_sound_hcount_fn hcount;
    void *hcount_user;
    wm_sound_health_fn health;
    void *health_user;
    uint8_t health_wrestler_count;
    wm_sound_crowd_fn crowd;
    void *crowd_user;
} wm_arcade_sound;

typedef struct {
    bool played;
    uint8_t channel; /* 1..4 when played */
    uint16_t duration;
} wm_sound_result;

void wm_arcade_sound_init(wm_arcade_sound *s,
                          wm_sound_emit_fn emit, void *emit_user,
                          wm_sound_rng_fn rng, void *rng_user);
void wm_arcade_sound_bind_tables(wm_arcade_sound *s,
                                 const wm_sound_entry *table, uint16_t table_count,
                                 const wm_sound_script *scripts, uint16_t script_count,
                                 const wm_sound_random_table *random_tables,
                                 uint16_t random_table_count,
                                 wm_sound_wrestler_matrix wrestler_matrix,
                                 const wm_sound_speech_table *speech_tables,
                                 uint16_t speech_table_count,
                                 const wm_sound_crowd_table *crowd_tables,
                                 uint16_t crowd_table_count);
void wm_arcade_sound_bind_hcount(wm_arcade_sound *s,
                                  wm_sound_hcount_fn hcount,
                                  void *hcount_user);
void wm_arcade_sound_bind_game_callbacks(wm_arcade_sound *s,
                                         wm_sound_health_fn health,
                                         void *health_user,
                                         uint8_t wrestler_count,
                                         wm_sound_crowd_fn crowd,
                                         void *crowd_user);
void wm_arcade_sound_reset(wm_arcade_sound *s);
void wm_arcade_sound_tick(wm_arcade_sound *s);
void wm_arcade_sound_set_enabled(wm_arcade_sound *s, bool enabled);

/* Central source entry points. */
bool wm_sound_sndsnd(wm_arcade_sound *s, uint16_t dcs_command);
wm_sound_result wm_sound_triple_sound(wm_arcade_sound *s, uint16_t sound_index);
wm_sound_result wm_sound_channel_sound(wm_arcade_sound *s,
                                       uint16_t sound_index,
                                       uint8_t source_channel);
wm_sound_result wm_sound_table_sound(wm_arcade_sound *s, uint16_t sound_number);
wm_sound_result wm_sound_wrtable_sound(wm_arcade_sound *s,
                                       uint16_t lookup_index,
                                       uint8_t wrestler);
wm_sound_announcer wm_sound_who_is_it(uint16_t sound_index);
wm_sound_result wm_sound_announcer_sound(wm_arcade_sound *s,
                                         uint16_t sound_index);
void wm_sound_nosounds(wm_arcade_sound *s);
void wm_sound_kill_all_channels(wm_arcade_sound *s);
void wm_sound_find_and_kill_endless(wm_arcade_sound *s);

/* Exact low-level announcer queue calls: queue entries are raw triple-table ids. */
bool wm_sound_add_voice(wm_arcade_sound *s, uint16_t sound_index);
bool wm_sound_if_silent_add_voice(wm_arcade_sound *s, uint16_t sound_index);
void wm_sound_reset_voice_queue(wm_arcade_sound *s);
void wm_sound_service_voice_queue(wm_arcade_sound *s);
void wm_sound_clear_speech_repeat(wm_arcade_sound *s);

/* High-level ADD_TO_QUEUE / ADD_IF_SILENT table selector. */
bool wm_sound_add_to_queue(wm_arcade_sound *s,
                           uint16_t speech_table_index,
                           uint16_t chance_per_1000,
                           uint8_t attacking_wrestler);
bool wm_sound_add_table_if_silent(wm_arcade_sound *s,
                                  uint16_t speech_table_index,
                                  uint16_t chance_per_1000,
                                  uint8_t attacking_wrestler);

/* Source process wrappers. These preserve the original PRCSLP delay and
 * RNDPER chance before entering ADD_IF_SILENT. Wrestler is the source
 * WRESTLERNUM captured when the process is created. */
bool wm_sound_schedule_speech_if_silent(wm_arcade_sound *s, uint16_t delay_ticks,
                                         wm_sound_speech_table_id table,
                                         uint16_t chance_per_1000,
                                         uint8_t wrestler);
bool wm_sound_call_misses(wm_arcade_sound *s);
bool wm_sound_call_special_move(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_drop_kick(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_face_hit(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_mid_hit(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_average_move(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_do_reversal(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_miss_yoko(wm_arcade_sound *s);
bool wm_sound_call_thrown_out(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_other_average(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_nasty_move(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_call_setup(wm_arcade_sound *s, uint8_t wrestler);
bool wm_sound_maybe_bounce_rope(wm_arcade_sound *s);
bool wm_sound_maybe_high_risk(wm_arcade_sound *s);
bool wm_sound_maybe_tough_enough(wm_arcade_sound *s);
wm_sound_result wm_sound_maybe_shocking(wm_arcade_sound *s);
bool wm_sound_maybe_giddup(wm_arcade_sound *s, uint8_t wrestler);

/* DCS volume protocol. Source channel is -1 for master or 0..3. */
void wm_sound_set_lower_vol(wm_arcade_sound *s, int8_t source_channel,
                            uint8_t value);
void wm_sound_set_volume(wm_arcade_sound *s, uint8_t value);
void wm_sound_begin_fade(wm_arcade_sound *s, int8_t source_channel,
                         uint16_t ticks);

/* Process-equivalent source helpers. */
wm_sound_result wm_sound_ring_bell_start(wm_arcade_sound *s);
wm_sound_result wm_sound_walk(wm_arcade_sound *s);
wm_sound_result wm_sound_small_run(wm_arcade_sound *s);
wm_sound_result wm_sound_small_bounce(wm_arcade_sound *s);
wm_sound_result wm_sound_hit_the_mat(wm_arcade_sound *s);
wm_sound_result wm_sound_block(wm_arcade_sound *s);
wm_sound_result wm_sound_flame(wm_arcade_sound *s);
wm_sound_result wm_sound_flame_hit(wm_arcade_sound *s);
wm_sound_result wm_sound_blocked(wm_arcade_sound *s, uint8_t wrestler);
wm_sound_result wm_sound_scream(wm_arcade_sound *s, uint8_t wrestler);
wm_sound_result wm_sound_wail(wm_arcade_sound *s, uint8_t wrestler);
wm_sound_result wm_sound_choke(wm_arcade_sound *s, uint8_t wrestler);
wm_sound_result wm_sound_nono(wm_arcade_sound *s, uint8_t wrestler);
wm_sound_result wm_sound_razor_rug_speech(wm_arcade_sound *s,
                                          uint16_t repeat_count);
wm_sound_result wm_sound_doink_slam(wm_arcade_sound *s,
                                    uint16_t repeat_count);
wm_sound_result wm_sound_bone_break(wm_arcade_sound *s);

#endif
